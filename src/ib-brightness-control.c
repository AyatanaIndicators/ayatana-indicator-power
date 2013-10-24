/*
 * Copyright 2012 Canonical Ltd.
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
 *
 * Authors:
 *     Renato Araujo Oliveira Filho <renato@canonical.com>
 */

#include <gudev/gudev.h>

#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "ib-brightness-control.h"

struct _IbBrightnessControl
{
    gchar *path;
};

IbBrightnessControl*
ib_brightness_control_new (void)
{
    IbBrightnessControl *control;
    GUdevClient *client;
    gchar *path = NULL;
    GList *devices;

    // detect device
    client = g_udev_client_new (NULL);
    devices = g_udev_client_query_by_subsystem (client, "backlight");
    if (devices != NULL) {
        GList *device;
        const gchar *device_type;

        for (device = devices; device != NULL; device = device->next) {
            device_type = g_udev_device_get_sysfs_attr (device->data, "type");
            if ((g_strcmp0 (device_type, "firmware") == 0) ||
                (g_strcmp0 (device_type, "platform") == 0) ||
                (g_strcmp0 (device_type, "raw") == 0)) {
                path = g_strdup (g_udev_device_get_sysfs_path (device->data));
                g_debug ("found: %s", path);
                break;
            }
        }

        g_list_free_full (devices, g_object_unref);
    }
    else {
        g_warning ("Fail to query backlight devices.");
    }

    control = g_new0 (IbBrightnessControl, 1);
    control->path = path;

    g_object_unref (client);
    return control;
}

void
ib_brightness_control_set_value (IbBrightnessControl* self, gint value)
{
    gint fd;
    gchar *filename;
    gchar *svalue;
    gint length;
    gint err;

    if (self->path == NULL)
      return;

    filename = g_build_filename (self->path, "brightness", NULL);
    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        g_warning ("Fail to set brightness.");
        g_free (filename);
        return;
    }

    svalue = g_strdup_printf ("%i", value);
    length = strlen (svalue);

    err = errno;
    errno = 0;
    if (write (fd, svalue, length) != length) {
        g_warning ("Fail to write brightness information: %s", g_strerror(errno));
    }
    errno = err;

    close (fd);
    g_free (svalue);
    g_free (filename);
}

gint
ib_brightness_control_get_value_from_file (IbBrightnessControl *self, const gchar *file)
{
    GError *error;
    gchar *svalue;
    gint value;
    gchar *filename;

    if (self->path == NULL)
      return 0;

    svalue = NULL;
    error = NULL;
    filename = g_build_filename (self->path, file, NULL);
    g_file_get_contents (filename, &svalue, NULL, &error);
    if (error) {
        g_warning ("Fail to get brightness value: %s", error->message);
        value = -1;
        g_error_free (error);
    } else {
        value = atoi (svalue);
        g_free (svalue);
    }

    g_free (filename);

    return value;

}

gint
ib_brightness_control_get_value (IbBrightnessControl* self)
{
    return ib_brightness_control_get_value_from_file (self, "brightness");
}

gint
ib_brightness_control_get_max_value (IbBrightnessControl* self)
{
    return ib_brightness_control_get_value_from_file (self, "max_brightness");
}

void
ib_brightness_control_free (IbBrightnessControl *self)
{
    g_free (self->path);
    g_free (self);
}

