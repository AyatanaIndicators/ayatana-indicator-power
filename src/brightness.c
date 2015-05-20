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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "brightness.h"
#include "dbus-powerd.h"

#include <gio/gio.h>

#define SCHEMA_NAME "com.ubuntu.touch.system"
#define KEY_AUTO "auto-brightness"
#define KEY_AUTO_SUPPORTED "auto-brightness-supported"
#define KEY_BRIGHTNESS "brightness"
#define KEY_NEED_DEFAULT "brightness-needs-hardware-default"

enum
{
  PROP_0,
  PROP_PERCENTAGE,
  PROP_AUTO,
  PROP_AUTO_SUPPORTED,
  LAST_PROP
};

static GParamSpec* properties[LAST_PROP];

typedef struct
{
  GDBusConnection * system_bus;
  GCancellable * cancellable;

  GSettings * settings;

  DbusPowerd * powerd_proxy;
  char * powerd_name_owner;

  double percentage;

  /* powerd brightness params */
  gint powerd_dim;
  gint powerd_min;
  gint powerd_max;
  gint powerd_default_value;
  gboolean powerd_ab_supported;
  gboolean have_powerd_params;
}
IndicatorPowerBrightnessPrivate;

typedef IndicatorPowerBrightnessPrivate priv_t;

G_DEFINE_TYPE_WITH_PRIVATE(IndicatorPowerBrightness,
                           indicator_power_brightness,
                           G_TYPE_OBJECT)

#define get_priv(o) ((priv_t*)indicator_power_brightness_get_instance_private(o))

/***
****  GObject virtual functions
***/

static void
my_get_property(GObject     * o,
                guint         property_id,
                GValue      * value,
                GParamSpec  * pspec)
{
  IndicatorPowerBrightness * self = INDICATOR_POWER_BRIGHTNESS(o);
  priv_t * p = get_priv(self);

  switch (property_id)
    {
      case PROP_PERCENTAGE:
        g_value_set_double(value, indicator_power_brightness_get_percentage(self));
        break;

      case PROP_AUTO:
        g_value_set_boolean(value, p->settings ? g_settings_get_boolean(p->settings, KEY_AUTO) : FALSE);
        break;

      case PROP_AUTO_SUPPORTED:
        g_value_set_boolean(value, p->powerd_ab_supported);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(o, property_id, pspec);
    }
}

static void
my_set_property(GObject       * o,
                guint           property_id,
                const GValue  * value,
                GParamSpec    * pspec)
{
  IndicatorPowerBrightness * self = INDICATOR_POWER_BRIGHTNESS(o);
  priv_t * p = get_priv(self);

  switch (property_id)
    {
      case PROP_PERCENTAGE:
        indicator_power_brightness_set_percentage(self, g_value_get_double(value));
        break;

      case PROP_AUTO:
        if (p->settings != NULL)
          g_settings_set_boolean (p->settings, KEY_AUTO, g_value_get_boolean(value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(o, property_id, pspec);
    }
}

static void
my_dispose(GObject * o)
{
  IndicatorPowerBrightness * self = INDICATOR_POWER_BRIGHTNESS(o);
  priv_t * p = get_priv(self);

  if (p->cancellable != NULL)
    {
      g_cancellable_cancel(p->cancellable);
      g_clear_object(&p->cancellable);
    }

  if (p->powerd_proxy != NULL)
    {
      g_signal_handlers_disconnect_by_data(p->powerd_proxy, o);
      g_clear_object(&p->powerd_proxy);
    }

  g_clear_object(&p->settings);
  g_clear_object(&p->system_bus);
  g_clear_pointer(&p->powerd_name_owner, g_free);

  G_OBJECT_CLASS(indicator_power_brightness_parent_class)->dispose(o);
}

/***
****  Percentage <-> Brightness Int conversion helpers
***/

static gdouble
brightness_to_percentage(IndicatorPowerBrightness * self, int brightness)
{
  const priv_t * p;
  gdouble percentage;

  p = get_priv(self);
  if (p->have_powerd_params)
    {
      const int lo = p->powerd_min;
      const int hi = p->powerd_max;
      percentage = (brightness-lo) / (double)(hi-lo);
    }
  else
    {
      percentage = 0;
    }

  return percentage;
}

static int
percentage_to_brightness(IndicatorPowerBrightness * self, double percentage)
{
  const priv_t * p;
  int brightness;

  p = get_priv(self);
  if (p->have_powerd_params)
    {
      const int lo = p->powerd_min;
      const int hi = p->powerd_max;
      brightness = (int)(lo + (percentage*(hi-lo)));
    }
  else
    {
      brightness = 0;
    }

  return brightness;
}

/**
 * DBus Chatter: com.canonical.powerd
 *
 * This is used to get default value, and upper and lower bounds,
 * of the brightness setting
 */

static void set_brightness_global(IndicatorPowerBrightness*, int);
static void set_brightness_local(IndicatorPowerBrightness*, int);

static void
on_powerd_brightness_params_ready(GObject      * oproxy,
                                  GAsyncResult * res,
                                  gpointer       gself)
{
  GError * error;
  GVariant * v;

  v = NULL;
  error = NULL;
  if (dbus_powerd_call_get_brightness_params_finish(DBUS_POWERD(oproxy), &v, res, &error))
    {
      IndicatorPowerBrightness * self = INDICATOR_POWER_BRIGHTNESS(gself);
      priv_t * p = get_priv(self);
      const gboolean old_ab_supported = p->powerd_ab_supported;

      g_message("%s", g_variant_print(v, TRUE));

      p->have_powerd_params = TRUE;
      g_variant_get(v, "(iiiib)", &p->powerd_dim,
                                  &p->powerd_min,
                                  &p->powerd_max,
                                  &p->powerd_default_value,
                                  &p->powerd_ab_supported);
      g_debug("powerd brightness settings: dim=%d, min=%d, max=%d, default=%d, ab_supported=%d",
              p->powerd_dim,
              p->powerd_min,
              p->powerd_max,
              p->powerd_default_value,
              (int)p->powerd_ab_supported);
 
      if (old_ab_supported != p->powerd_ab_supported)
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AUTO_SUPPORTED]);

      if (p->settings != NULL)
        { 
          if (g_settings_get_boolean(p->settings, KEY_NEED_DEFAULT))
            {
              /* user's first session, so init the schema's default
                 brightness from powerd's hardware-specific params */
              g_debug("%s is true, so initializing brightness to powerd default '%d'", KEY_NEED_DEFAULT, p->powerd_default_value);
              set_brightness_global(self, p->powerd_default_value);
              g_settings_set_boolean(p->settings, KEY_NEED_DEFAULT, FALSE);
            }
          else
            {
              /* not the first time, so restore the previous session's brightness */
              set_brightness_global(self, g_settings_get_int(p->settings, KEY_BRIGHTNESS));
            }
        }

      /* cleanup */
      g_variant_unref(v);
    }
  else if (error != NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Unable to get system bus: %s", error->message);

      g_error_free(error);
    }
}

static void
on_powerd_name_owner_changed(GDBusProxy * powerd_proxy,
                             GParamSpec * pspec         G_GNUC_UNUSED,
                             gpointer     gself)
{
  priv_t * p = get_priv(INDICATOR_POWER_BRIGHTNESS(gself));
  gchar * owner = g_dbus_proxy_get_name_owner(powerd_proxy);

  if (g_strcmp0(p->powerd_name_owner, owner))
    {
      p->have_powerd_params = FALSE;

      if (owner != NULL)
        {
          dbus_powerd_call_get_brightness_params(DBUS_POWERD(powerd_proxy),
                                                 p->cancellable,
                                                 on_powerd_brightness_params_ready,
                                                 gself);
        }

      g_free(p->powerd_name_owner);
      p->powerd_name_owner = owner;
  }
}

static void
on_powerd_brightness_changed(DbusPowerd * powerd_proxy,
                             GParamSpec * pspec         G_GNUC_UNUSED,
                             gpointer     gself)
{
  set_brightness_local(gself, dbus_powerd_get_brightness(powerd_proxy));
}

static void
on_powerd_proxy_ready(GObject      * source_object G_GNUC_UNUSED,
                      GAsyncResult * res,
                      gpointer       gself)
{
  GError * error;
  DbusPowerd * powerd_proxy;

  error = NULL;
  powerd_proxy = dbus_powerd_proxy_new_for_bus_finish(res, &error);

  if (powerd_proxy != NULL)
    {
      priv_t * p;
      p = get_priv(INDICATOR_POWER_BRIGHTNESS(gself));

      /* keep a handle to the system bus */
      g_clear_object(&p->system_bus);
      p->system_bus = g_object_ref(g_dbus_proxy_get_connection(G_DBUS_PROXY(powerd_proxy)));

      /* keep the proxy and listen to owner changes */
      p->powerd_proxy = powerd_proxy;
      g_signal_connect(p->powerd_proxy, "notify::g-name-owner",
                       G_CALLBACK(on_powerd_name_owner_changed), gself);
      g_signal_connect(p->powerd_proxy, "notify::brightness",
                       G_CALLBACK(on_powerd_brightness_changed), gself);
      on_powerd_name_owner_changed(G_DBUS_PROXY(powerd_proxy), NULL, gself);
    }
  else if (error != NULL)
    {
      if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Unable to get powerd proxy: %s", error->message);

      g_error_free(error);
    }
}

/**
 * DBus Chatter: com.canonical.Unity.Screen
 *
 * Used to set the backlight brightness via setUserBrightness
 */

/* setUserBrightness doesn't return anything,
   so this function is just to check for bus error messages */
static void
on_set_uscreen_user_brightness_result(GObject      * system_bus,
                                      GAsyncResult * res,
                                      gpointer       gself        G_GNUC_UNUSED)
{
  GError * error;
  GVariant * v;

  error = NULL;
  v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(system_bus), res, &error);
  if (error != NULL)
    {
      if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Unable to call uscreen.setBrightness: %s", error->message);

      g_error_free(error);
    }

  g_clear_pointer(&v, g_variant_unref);
}

static void
set_uscreen_user_brightness(IndicatorPowerBrightness * self,
                            int                        value)
{
  priv_t * p = get_priv(self);

  g_dbus_connection_call(p->system_bus,
                         "com.canonical.Unity.Screen",
                         "/com/canonical/Unity/Screen",
                         "com.canonical.Unity.Screen",
                         "setUserBrightness",
                         g_variant_new("(i)", value),
                         NULL, /* no return args */
                         G_DBUS_CALL_FLAGS_NONE,
                         -1, /* default timeout */
                         p->cancellable,
                         on_set_uscreen_user_brightness_result,
                         self);
}

/***
****
***/

static void
set_brightness_local(IndicatorPowerBrightness * self, int brightness)
{
  priv_t * p = get_priv(self);
  p->percentage = brightness_to_percentage(self, brightness);
  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PERCENTAGE]);
}

static void
on_brightness_changed_in_schema(GSettings * settings,
                                gchar     * key,
                                gpointer    gself)
{
  set_brightness_local(INDICATOR_POWER_BRIGHTNESS(gself),
                       g_settings_get_int(settings, key));
}

static void
set_brightness_global(IndicatorPowerBrightness * self, int brightness)
{
  priv_t * p = get_priv(self);

  set_uscreen_user_brightness(self, brightness);

  if (p->settings != NULL)
    g_settings_set_int(p->settings, KEY_BRIGHTNESS, brightness);
  else
    set_brightness_local(self, brightness);
}

static void
on_auto_changed_in_schema(IndicatorPowerBrightness * self)
{
  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AUTO]);
}


/***
****  Instantiation
***/

static void
indicator_power_brightness_init(IndicatorPowerBrightness * self)
{
  priv_t * p;
  GSettingsSchema * schema;

  p = get_priv(self);
  p->cancellable = g_cancellable_new();

  schema = g_settings_schema_source_lookup(g_settings_schema_source_get_default(),
                                           SCHEMA_NAME,
                                           TRUE);

  /* "brightness" is only spec'ed for the phone profile,
     so fail gracefully & silently if we don't have the
     schema for it. */
  if (schema != NULL)
    {
      if (g_settings_schema_has_key(schema, KEY_BRIGHTNESS))
        {
          p->settings = g_settings_new(SCHEMA_NAME);
          g_signal_connect(p->settings, "changed::" KEY_BRIGHTNESS,
                           G_CALLBACK(on_brightness_changed_in_schema), self);
          g_signal_connect_swapped(p->settings, "changed::" KEY_AUTO,
                                   G_CALLBACK(on_auto_changed_in_schema), self);
        }
      g_settings_schema_unref(schema);
    }

  dbus_powerd_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                 G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                                 "com.canonical.powerd",
                                 "/com/canonical/powerd",
                                 p->cancellable,
                                 on_powerd_proxy_ready,
                                 self);
}

static void
indicator_power_brightness_class_init(IndicatorPowerBrightnessClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = my_dispose;
  object_class->get_property = my_get_property;
  object_class->set_property = my_set_property;

  properties[PROP_0] = NULL;

  properties[PROP_PERCENTAGE] = g_param_spec_double(
    "percentage",
    "Percentage",
    "Brightness percentage",
    0.0, /* minimum */
    1.0, /* maximum */
    0.8,
    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  properties[PROP_AUTO] = g_param_spec_boolean(
    "auto-brightness",
    "Auto-Brightness",
    "Automatically adjust brightness level",
    FALSE,
    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  properties[PROP_AUTO_SUPPORTED] = g_param_spec_boolean(
    "auto-brightness-supported",
    "Auto-Brightness Supported",
    "True if the device can automatically adjust brightness",
    FALSE,
    G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties(object_class, LAST_PROP, properties);
}

/***
****  Public API
***/

IndicatorPowerBrightness *
indicator_power_brightness_new(void)
{
  gpointer o = g_object_new(INDICATOR_TYPE_POWER_BRIGHTNESS, NULL);

  return INDICATOR_POWER_BRIGHTNESS(o);
}

void
indicator_power_brightness_set_percentage(IndicatorPowerBrightness * self,
                                          double                     percentage)
{
  g_return_if_fail(INDICATOR_IS_POWER_BRIGHTNESS(self));

  set_brightness_global(self, percentage_to_brightness(self, percentage));
}

double
indicator_power_brightness_get_percentage(IndicatorPowerBrightness * self)
{
  g_return_val_if_fail(INDICATOR_IS_POWER_BRIGHTNESS(self), 0.0);

  return get_priv(self)->percentage;
}
