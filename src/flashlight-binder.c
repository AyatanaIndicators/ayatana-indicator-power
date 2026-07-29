/*
 * Copyright 2026 The UBports project
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Torch backend that goes through the AIDL camera provider HAL, for devices
 * whose sysfs node cannot drive the LED on its own.  setTorchMode() is
 * specified to work with no camera session open.  Some flash drivers expose a
 * sysfs node that only toggles the LED and leaves the duty and the flash
 * safety timeout unprogrammed, so the LED lights and then goes out again by
 * itself; the HAL sets all three and the LED stays on.
 *
 * Selected with "FlashlightBackend: camera-hal" in the deviceinfo yaml.
 */

#include "flashlight.h"

#include <gbinder.h>
#include <glib.h>

#define TORCH_BINDER_DEV    "/dev/binder"
#define TORCH_PROVIDER_NAME "android.hardware.camera.provider.ICameraProvider/internal/0"
#define TORCH_PROVIDER_IFACE "android.hardware.camera.provider.ICameraProvider"
#define TORCH_DEVICE_IFACE  "android.hardware.camera.device.ICameraDevice"

/* AIDL transaction codes are FIRST_CALL_TRANSACTION plus the declaration index. */
#define PROVIDER_GET_CAMERA_ID_LIST   3
#define PROVIDER_GET_DEVICE_INTERFACE 4
#define DEVICE_SET_TORCH_MODE         7

static GBinderServiceManager* torch_sm = NULL;
static GBinderClient* torch_provider = NULL;
static GBinderClient* torch_device = NULL;
static char** torch_camera_ids = NULL;
static gboolean torch_probed = FALSE;

/* Every AIDL reply starts with an exception code; 0 is success. */
static gboolean
reply_ok(GBinderReader* reader)
{
    gint32 exception = 0;

    return gbinder_reader_read_int32(reader, &exception) && exception == 0;
}

static char**
read_camera_ids(void)
{
    GBinderLocalRequest* req;
    GBinderRemoteReply* reply;
    GPtrArray* ids = NULL;
    int status = 0;

    req = gbinder_client_new_request(torch_provider);
    reply = gbinder_client_transact_sync_reply(torch_provider,
                                               PROVIDER_GET_CAMERA_ID_LIST, req, &status);
    gbinder_local_request_unref(req);

    if (reply)
    {
        GBinderReader reader;
        gint32 count = 0;

        gbinder_remote_reply_init_reader(reply, &reader);

        if (reply_ok(&reader) && gbinder_reader_read_int32(&reader, &count) && count > 0)
        {
            gint32 i;

            ids = g_ptr_array_new();

            for (i = 0; i < count; i++)
            {
                char* name = gbinder_reader_read_string16(&reader);

                if (!name)
                    break;

                g_ptr_array_add(ids, name);
            }

            g_ptr_array_add(ids, NULL);
        }

        gbinder_remote_reply_unref(reply);
    }

    return ids ? (char**) g_ptr_array_free(ids, FALSE) : NULL;
}

static GBinderClient*
camera_device_client(const char* name)
{
    GBinderLocalRequest* req;
    GBinderRemoteReply* reply;
    GBinderWriter writer;
    GBinderClient* client = NULL;
    int status = 0;

    req = gbinder_client_new_request(torch_provider);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_string16(&writer, name);
    reply = gbinder_client_transact_sync_reply(torch_provider,
                                               PROVIDER_GET_DEVICE_INTERFACE, req, &status);
    gbinder_local_request_unref(req);

    if (reply)
    {
        GBinderReader reader;

        gbinder_remote_reply_init_reader(reply, &reader);

        if (reply_ok(&reader))
        {
            GBinderRemoteObject* obj = gbinder_reader_read_object(&reader);

            if (obj)
            {
                client = gbinder_client_new(obj, TORCH_DEVICE_IFACE);
                gbinder_remote_object_unref(obj);
            }
        }

        gbinder_remote_reply_unref(reply);
    }

    return client;
}

static gboolean
set_torch_mode(GBinderClient* device, int enable)
{
    GBinderLocalRequest* req;
    GBinderRemoteReply* reply;
    GBinderWriter writer;
    gboolean ok = FALSE;
    int status = 0;

    req = gbinder_client_new_request(device);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, enable != 0);
    reply = gbinder_client_transact_sync_reply(device, DEVICE_SET_TORCH_MODE, req, &status);
    gbinder_local_request_unref(req);

    if (reply)
    {
        GBinderReader reader;

        gbinder_remote_reply_init_reader(reply, &reader);
        ok = reply_ok(&reader);
        gbinder_remote_reply_unref(reply);
    }

    return ok;
}

static void
torch_probe(void)
{
    GBinderRemoteObject* remote;
    int status = 0;

    torch_probed = TRUE;

    torch_sm = gbinder_servicemanager_new(TORCH_BINDER_DEV);

    if (!torch_sm)
    {
        g_warning("flashlight: cannot open %s", TORCH_BINDER_DEV);
        return;
    }

    /* Autoreleased, so it must not be unreffed here. */
    remote = gbinder_servicemanager_get_service_sync(torch_sm, TORCH_PROVIDER_NAME,
                                                    &status);

    if (!remote)
    {
        g_warning("flashlight: %s is not registered", TORCH_PROVIDER_NAME);
        g_clear_pointer(&torch_sm, gbinder_servicemanager_unref);
        return;
    }

    torch_provider = gbinder_client_new(remote, TORCH_PROVIDER_IFACE);
    torch_camera_ids = read_camera_ids();

    if (!torch_camera_ids)
    {
        g_warning("flashlight: camera provider returned no cameras");
        g_clear_pointer(&torch_provider, gbinder_client_unref);
        g_clear_pointer(&torch_sm, gbinder_servicemanager_unref);
    }
}

int
flashlight_camera_hal_supported()
{
    if (!torch_probed)
        torch_probe();

    return torch_camera_ids != NULL;
}

int
flashlight_camera_hal_set(int enable)
{
    char** name;

    if (!flashlight_camera_hal_supported())
        return 0;

    if (torch_device && set_torch_mode(torch_device, enable))
        return 1;

    /* Only one camera owns the flash, and which one is not known up front. */
    g_clear_pointer(&torch_device, gbinder_client_unref);

    for (name = torch_camera_ids; *name; name++)
    {
        GBinderClient* device = camera_device_client(*name);

        if (!device)
            continue;

        if (set_torch_mode(device, enable))
        {
            torch_device = device;
            return 1;
        }

        gbinder_client_unref(device);
    }

    return 0;
}
