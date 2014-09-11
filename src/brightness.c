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

#include <gio/gio.h>

#define SCHEMA_NAME "com.ubuntu.touch.system"
#define KEY_BRIGHTNESS "brightness"
#define KEY_NEED_DEFAULT "brightness-needs-a-default"

enum
{
  PROP_0,
  PROP_PERCENTAGE,
  LAST_PROP
};

static GParamSpec* properties[LAST_PROP];

typedef struct
{
  GDBusConnection * system_bus;
  GCancellable * cancellable;

  GSettings * settings;

  guint powerd_name_tag;

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

  switch (property_id)
    {
      case PROP_PERCENTAGE:
        g_value_set_double(value, indicator_power_brightness_get_percentage(self));
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

  switch (property_id)
    {
      case PROP_PERCENTAGE:
        indicator_power_brightness_set_percentage(self, g_value_get_double(value));
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

  if (p->powerd_name_tag)
    {
      g_bus_unwatch_name(p->powerd_name_tag);
      p->powerd_name_tag = 0;
    }

  g_clear_object(&p->settings);
  g_clear_object(&p->system_bus);

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

/***
****  DBus Chatter: com.canonical.powerd
***/

static void set_brightness_global(IndicatorPowerBrightness*, int);

static void
on_powerd_brightness_params_ready(GObject      * source,
                                  GAsyncResult * res,
                                  gpointer       gself)
{
  GError * error;
  GVariant * v;

  error = NULL;
  v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &error);
  if (v == NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Unable to get system bus: %s", error->message);

      g_error_free(error);
    }
  else
    {
      IndicatorPowerBrightness * self = INDICATOR_POWER_BRIGHTNESS(gself);
      priv_t * p = get_priv(self);

      p->have_powerd_params = TRUE;
      g_variant_get(v, "((iiiib))", &p->powerd_dim,
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


      if (p->settings != NULL)
        { 
          if (g_settings_get_boolean(p->settings, KEY_NEED_DEFAULT))
            {
              /* user's first session, so init the schema's default
                 brightness from powerd's hardware-specific params */
              g_message("%s is true, so setting brightness to powerd default '%d'", KEY_NEED_DEFAULT, p->powerd_default_value);
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
}

static void
call_powerd_get_brightness_params(IndicatorPowerBrightness * self)
{
  priv_t * p = get_priv(self);

  g_dbus_connection_call(p->system_bus,
                         "com.canonical.powerd",
                         "/com/canonical/powerd",
                         "com.canonical.powerd",
                         "getBrightnessParams",
                         NULL,
                         G_VARIANT_TYPE("((iiiib))"),
                         G_DBUS_CALL_FLAGS_NONE,
                         -1, /* default timeout */
                         p->cancellable,
                         on_powerd_brightness_params_ready,
                         self);
}

static void
on_powerd_appeared(GDBusConnection * connection,
                   const gchar     * bus_name     G_GNUC_UNUSED,
                   const gchar     * name_owner   G_GNUC_UNUSED,
                   gpointer          gself)
{
  IndicatorPowerBrightness * self = INDICATOR_POWER_BRIGHTNESS(gself);
  priv_t * p = get_priv(self);

  /* keep a handle to the system bus */
  g_clear_object(&p->system_bus);
  p->system_bus = g_object_ref(connection);
 
  /* update our cache of powerd's brightness params */
  call_powerd_get_brightness_params(self);
}

static void
on_powerd_vanished(GDBusConnection * connection  G_GNUC_UNUSED,
                   const gchar     * bus_name    G_GNUC_UNUSED,
                   gpointer          gself)
{
  priv_t * p = get_priv(INDICATOR_POWER_BRIGHTNESS(gself));

  p->have_powerd_params = FALSE;
}

/***
****  DBus Chatter: com.canonical.Unity.Screen
***/

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

void
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
  g_message("%s setting brightness property percentage to %.2f", G_STRFUNC, p->percentage);
  g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PERCENTAGE]);
}

static void
on_brightness_changed_in_schema(GSettings * settings,
                                gchar     * key,
                                gpointer    gself)
{
  g_message("%s schema changed; updating our local property", G_STRFUNC);
  set_brightness_local(INDICATOR_POWER_BRIGHTNESS(gself),
                       g_settings_get_int(settings, key));
}

static void
set_brightness_global(IndicatorPowerBrightness * self, int brightness)
{
  priv_t * p = get_priv(self);

  g_message("%s setting uscreen and local to %d", G_STRFUNC, brightness);

  set_uscreen_user_brightness(self, brightness);

  if (p->settings != NULL)
    g_settings_set_int(p->settings, KEY_BRIGHTNESS, brightness);
  else
    set_brightness_local(self, brightness);
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
        }
      g_settings_schema_unref(schema);
    }

  p->powerd_name_tag = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
                                        "com.canonical.powerd",
                                        G_BUS_NAME_WATCHER_FLAGS_NONE,
                                        on_powerd_appeared,
                                        on_powerd_vanished,
                                        self,
                                        NULL);
}

static void
indicator_power_brightness_class_init(IndicatorPowerBrightnessClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = my_dispose;
  object_class->get_property = my_get_property;
  object_class->set_property = my_set_property;

  properties[PROP_0] = NULL;

  properties[PROP_PERCENTAGE] = g_param_spec_double("percentage",
                                                    "Percentage",
                                                    "Brightness percentage",
                                                    0.0, /* minimum */
                                                    1.0, /* maximum */
                                                    0.8,
                                                    G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

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

  g_debug("%s called; current value is %.2f, desired value is %.2f",
          G_STRFUNC, get_priv(self)->percentage, percentage);

  set_brightness_global(self, percentage_to_brightness(self, percentage));
}

double
indicator_power_brightness_get_percentage(IndicatorPowerBrightness * self)
{
  g_return_val_if_fail(INDICATOR_IS_POWER_BRIGHTNESS(self), 0.0);

  return get_priv(self)->percentage;
}
