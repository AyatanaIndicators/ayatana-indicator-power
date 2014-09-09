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

#include "ib-brightness-uscreen-control.h"

static gboolean getBrightnessParams(GDBusProxy* powerd_proxy, int *dim, int *min,
    int *max, int *dflt, gboolean *ab_supported);

GDBusProxy*
uscreen_get_proxy(brightness_params_t *params)
{
    GError *error = NULL;
    gboolean ret;

    g_return_val_if_fail (params != NULL, NULL);

    /* For now we still need to obtain the brigthness params from powerd */
    GDBusProxy* powerd_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                G_DBUS_PROXY_FLAGS_NONE,
                NULL,
                "com.canonical.powerd",
                "/com/canonical/powerd",
                "com.canonical.powerd",
                NULL,
                &error);

    if (error != NULL)
    {
        g_debug ("could not connect to powerd: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    ret = getBrightnessParams(powerd_proxy, &(params->dim), &(params->min),
        &(params->max), &(params->dflt), &(params->ab_supported));

    if (! ret)
    {
        g_debug ("can't get brightness parameters from powerd");
        g_object_unref (powerd_proxy);
        return NULL;
    }

    g_clear_object (&powerd_proxy);

    GDBusProxy* uscreen_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                G_DBUS_PROXY_FLAGS_NONE,
                NULL,
                "com.canonical.Unity.Screen",
                "/com/canonical/Unity/Screen",
                "com.canonical.Unity.Screen",
                NULL,
                &error);

    if (error != NULL)
    {
        g_debug ("could not connect to unity screen: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    return uscreen_proxy;
}


static gboolean
getBrightnessParams(GDBusProxy* powerd_proxy, int *dim, int *min, int *max, int *dflt, gboolean *ab_supported)
{
    GVariant *ret = NULL;
    GError *error = NULL;

    ret = g_dbus_proxy_call_sync(powerd_proxy,
            "getBrightnessParams",
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            400, NULL, &error); // timeout: 400 ms
    if (!ret)
    {
        if (error != NULL)
        {
            if (!g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN))
            {
                g_warning("getBrightnessParams from powerd failed: %s", error->message);
            }
            g_error_free(error);
        }
        return FALSE;
    }

    g_variant_get(ret, "((iiiib))", dim, min, max, dflt, ab_supported);
    g_variant_unref(ret);
    return TRUE;
}

static gboolean setUserBrightness(GDBusProxy* uscreen_proxy, GCancellable *gcancel, int brightness)
{
    GVariant *ret = NULL;
    GError *error = NULL;

    ret = g_dbus_proxy_call_sync(uscreen_proxy,
            "setUserBrightness",
            g_variant_new("(i)", brightness),
            G_DBUS_CALL_FLAGS_NONE,
            -1, gcancel, &error);
    if (!ret) {
        g_warning("setUserBrightness via unity.screen failed: %s", error->message);
        g_error_free(error);
        return FALSE;
    } else {
        g_variant_unref(ret);
        return TRUE;
    }
}

struct _IbBrightnessUScreenControl
{
    GDBusProxy *uscreen_proxy;
    GCancellable *gcancel;

    int dim;
    int min;
    int max;
    int dflt; // defalut value
    gboolean ab_supported;

    int current;
};

IbBrightnessUscreenControl*
ib_brightness_uscreen_control_new (GDBusProxy* uscreen_proxy, brightness_params_t params)
{
    IbBrightnessUscreenControl *control;

    control = g_new0 (IbBrightnessUscreenControl, 1);
    control->uscreen_proxy = uscreen_proxy;
    control->gcancel = g_cancellable_new();

    control->dim = params.dim;
    control->min = params.min;
    control->max = params.max;
    control->dflt = params.dflt;
    control->ab_supported = params.ab_supported;

    // XXX: set the brightness value is the only way to sync the brightness value with
    // unity.screen, and we should set the user prefered / last set brightness value upon startup.
    // Before we have code to store last set brightness value or other mechanism, we set
    // it to default brightness that powerd proposed.
    ib_brightness_uscreen_control_set_value(control, control->dflt);

    return control;
}

void
ib_brightness_uscreen_control_set_value (IbBrightnessUscreenControl* self, gint value)
{
    gboolean ret;

    value = CLAMP(value, self->min, self->max);
    ret = setUserBrightness(self->uscreen_proxy, self->gcancel, value);
    if (ret)
    {
        self->current = value;
    }
}

gint
ib_brightness_uscreen_control_get_value (IbBrightnessUscreenControl* self)
{
    return self->current;
}

gint
ib_brightness_uscreen_control_get_max_value (IbBrightnessUscreenControl* self)
{
    return self->max;
}

void
ib_brightness_uscreen_control_free (IbBrightnessUscreenControl *self)
{
    g_cancellable_cancel (self->gcancel);
    g_object_unref (self->gcancel);
    g_object_unref (self->uscreen_proxy);
    g_free (self);
}

