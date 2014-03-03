/*
 * Copyright 2014 Canonical Ltd.
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
 *     Yuan-Chen Cheng <yc.cheng@canonical.com>
 */

#include <gudev/gudev.h>

#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "ib-brightness-powerd-control.h"

GDBusProxy*
powerd_get_proxy(void)
{
    GError *error = NULL;
    GDBusProxy* powerd_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                G_DBUS_PROXY_FLAGS_NONE,
                NULL,
                "com.canonical.powerd",
                "/com/canonical/powerd",
                "com.canonical.powerd",
                NULL,
                &error);

    if (error != NULL) {
        g_error_free (error);
        g_debug ("could not connect to powerd: %s", error->message);
        return NULL;
    }
    return powerd_proxy;
}


static gboolean
getBrightnessParams(GDBusProxy* powerd_proxy, int *min, int *max, int *dflt, gboolean *ab_supported)
{
    GVariant *ret = NULL;
    GError *error = NULL;

    ret = g_dbus_proxy_call_sync(powerd_proxy,
            "getBrightnessParams",
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1, NULL, &error);
    if (!ret) {
        g_warning("getBrightnessParams failed: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    g_variant_get(ret, "((iiib))", min, max, dflt, ab_supported);
    g_variant_unref(ret);
    return TRUE;
}

static gboolean setUserBrightness(GDBusProxy* powerd_proxy, int brightness)
{
    GVariant *ret = NULL;
    GError *error = NULL;

    ret = g_dbus_proxy_call_sync(powerd_proxy,
            "setUserBrightness",
            g_variant_new("(i)", brightness),
            G_DBUS_CALL_FLAGS_NONE,
            -1, NULL, &error);
    if (!ret) {
        g_warning("setUserBrightness failed: %s", error->message);
        g_error_free(error);
        return FALSE;
    } else {
        g_variant_unref(ret);
        return TRUE;
    }
}

struct _IbBrightnessPowerdControl
{
    gboolean inited;

    GDBusProxy *powerd_proxy;

    int min;
    int max;
    int dflt;
    gboolean ab_supported;

    int current;
};


static void ib_brightness_init(IbBrightnessPowerdControl *control)
{
    gboolean ret = getBrightnessParams(control->powerd_proxy, &(control->min),
        &(control->max), &(control->dflt), &(control->ab_supported));
    if (! ret) return;

    ib_brightness_powerd_control_set_value(control, control->max);

    control->inited = TRUE;
}

IbBrightnessPowerdControl*
ib_brightness_powerd_control_new (GDBusProxy* powerd_proxy)
{
    IbBrightnessPowerdControl *control;

    control = g_new0 (IbBrightnessPowerdControl, 1);
    control->inited = FALSE;
    control->powerd_proxy = powerd_proxy;

    ib_brightness_init(control);

    return control;
}

void
ib_brightness_powerd_control_set_value (IbBrightnessPowerdControl* self, gint value)
{
    gboolean ret;
    if (! self->inited) return;
    if (value > self->max || value < self->min) return;
    ret = setUserBrightness(self->powerd_proxy, value);
    if (ret)
    {
        self->current = value;
    }
}

gint
ib_brightness_powerd_control_get_value (IbBrightnessPowerdControl* self)
{
    if (! self->inited) return 0;
    return self->current;
}

gint
ib_brightness_powerd_control_get_max_value (IbBrightnessPowerdControl* self)
{
    if (! self->inited) return 0;
    return self->max;
}

void
ib_brightness_powerd_control_free (IbBrightnessPowerdControl *self)
{
    g_object_unref (self->powerd_proxy);
    g_free (self);
}

