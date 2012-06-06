/*

A simple Device structure used internally by indicator-power

Copyright 2012 Canonical Ltd.

Authors:
    Charles Kerr <charles.kerr@canonical.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 3.0 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 3.0 for more details.

You should have received a copy of the GNU General Public
License along with this library. If not, see
<http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <glib/gi18n.h>

#include "device.h"

struct _IndicatorPowerDevicePrivate
{
  UpDeviceKind kind;
  UpDeviceState state;
  gchar * object_path;
  gdouble percentage;
  time_t time;
};

#define INDICATOR_POWER_DEVICE_GET_PRIVATE(o) (INDICATOR_POWER_DEVICE(o)->priv)

/* Properties */
/* Enum for the properties so that they can be quickly found and looked up. */
enum {
  PROP_0,
  PROP_KIND,
  PROP_STATE,
  PROP_OBJECT_PATH,
  PROP_PERCENTAGE,
  PROP_TIME,
  N_PROPERTIES
};

static GParamSpec * properties[N_PROPERTIES];

/* GObject stuff */
static void indicator_power_device_class_init (IndicatorPowerDeviceClass *klass);
static void indicator_power_device_init       (IndicatorPowerDevice *self);
static void indicator_power_device_dispose    (GObject *object);
static void indicator_power_device_finalize   (GObject *object);
static void set_property (GObject*, guint prop_id, const GValue*, GParamSpec* );
static void get_property (GObject*, guint prop_id,       GValue*, GParamSpec* );

/* LCOV_EXCL_START */
G_DEFINE_TYPE (IndicatorPowerDevice, indicator_power_device, G_TYPE_OBJECT);
/* LCOV_EXCL_STOP */

static void
indicator_power_device_class_init (IndicatorPowerDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (IndicatorPowerDevicePrivate));

  object_class->dispose = indicator_power_device_dispose;
  object_class->finalize = indicator_power_device_finalize;
  object_class->set_property = set_property;
  object_class->get_property = get_property;

  properties[PROP_KIND] = g_param_spec_int (INDICATOR_POWER_DEVICE_KIND,
                                            "kind",
                                            "The device's UpDeviceKind",
                                            UP_DEVICE_KIND_UNKNOWN, UP_DEVICE_KIND_LAST,
                                            UP_DEVICE_KIND_UNKNOWN,
                                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_STATE] = g_param_spec_int (INDICATOR_POWER_DEVICE_STATE,
                                             "state",
                                             "The device's UpDeviceState",
                                             UP_DEVICE_STATE_UNKNOWN, UP_DEVICE_STATE_LAST,
                                             UP_DEVICE_STATE_UNKNOWN,
                                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_OBJECT_PATH] = g_param_spec_string (INDICATOR_POWER_DEVICE_OBJECT_PATH,
                                                      "object path",
                                                      "The device's DBus object path",
                                                      NULL,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PERCENTAGE] = g_param_spec_double (INDICATOR_POWER_DEVICE_PERCENTAGE,
                                                     "percentage",
                                                     "percent charged",
                                                     0.0, 100.0,
                                                     0.0,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_TIME] = g_param_spec_uint64 (INDICATOR_POWER_DEVICE_TIME,
                                               "time",
                                               "time left",
                                               0, G_MAXUINT64,
                                               0,
                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* Initialize an instance */
static void
indicator_power_device_init (IndicatorPowerDevice *self)
{
  IndicatorPowerDevicePrivate * priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (self, INDICATOR_POWER_DEVICE_TYPE,
                                            IndicatorPowerDevicePrivate);
  priv->kind = UP_DEVICE_KIND_UNKNOWN;
  priv->state = UP_DEVICE_STATE_UNKNOWN;
  priv->object_path = NULL;
  priv->percentage = 0.0;
  priv->time = 0;

  self->priv = priv;
}

static void
indicator_power_device_dispose (GObject *object)
{
  G_OBJECT_CLASS (indicator_power_device_parent_class)->dispose (object);
}

static void
indicator_power_device_finalize (GObject *object)
{
  IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(object);
  IndicatorPowerDevicePrivate * priv = self->priv;

  g_clear_pointer (&priv->object_path, g_free);
}

/***
****
***/

static void
get_property (GObject * o, guint  prop_id, GValue * value, GParamSpec * pspec)
{
  IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(o);
  IndicatorPowerDevicePrivate * priv = self->priv;

  switch (prop_id)
    {
      case PROP_KIND:
        g_value_set_int (value, priv->kind);
        break;

      case PROP_STATE:
        g_value_set_int (value, priv->state);
        break;

      case PROP_OBJECT_PATH:
        g_value_set_string (value, priv->object_path);
        break;

      case PROP_PERCENTAGE:
        g_value_set_double (value, priv->percentage);
        break;

      case PROP_TIME:
        g_value_set_uint64 (value, priv->time);
        break;
    }
}

static void
set_property (GObject * o, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(o);
  IndicatorPowerDevicePrivate * priv = self->priv;

  switch (prop_id)
    {
      case PROP_KIND:
        priv->kind = g_value_get_int (value);
        break;

      case PROP_STATE:
        priv->state = g_value_get_int (value);
        break;

      case PROP_OBJECT_PATH:
        g_free (priv->object_path);
        priv->object_path = g_value_dup_string (value);
        break;

      case PROP_PERCENTAGE:
        priv->percentage = g_value_get_double (value);
        break;

      case PROP_TIME:
        priv->time = g_value_get_uint64(value);
        break;
    }
}

/***
****  Accessors
***/

UpDeviceKind
indicator_power_device_get_kind  (const IndicatorPowerDevice * device)
{
  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_KIND_UNKNOWN);
  /* LCOV_EXCL_STOP */

  return device->priv->kind;
}

UpDeviceState
indicator_power_device_get_state (const IndicatorPowerDevice * device)
{
  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_STATE_UNKNOWN);
  /* LCOV_EXCL_STOP */

  return device->priv->state;
}

const gchar *
indicator_power_device_get_object_path (const IndicatorPowerDevice * device)
{
  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);
  /* LCOV_EXCL_STOP */

  return device->priv->object_path;
}

gdouble
indicator_power_device_get_percentage (const IndicatorPowerDevice * device)
{
  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), 0.0);
  /* LCOV_EXCL_STOP */

  return device->priv->percentage;
}

time_t
indicator_power_device_get_time (const IndicatorPowerDevice * device)
{
  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), (time_t)0);
  /* LCOV_EXCL_STOP */

  return device->priv->time;
}

/***
****
****
***/

/* taken from GSD's power plugin, (c) Richard Hughes and licensed GPL >=2 */
static const gchar *
gpm_upower_get_device_icon_suffix (gdouble percentage)
{
  if (percentage < 10) return "caution";
  if (percentage < 30) return "low";
  if (percentage < 60) return "good";
  return "full";
}

/* taken from GSD's power plugin, (c) Richard Hughes and licensed GPL >=2 */
static const gchar *
gpm_upower_get_device_icon_index (gdouble percentage)
{
  if (percentage < 10) return "000";
  if (percentage < 30) return "020";
  if (percentage < 50) return "040";
  if (percentage < 70) return "060";
  if (percentage < 90) return "080";
  return "100";
}

/**
  indicator_power_device_get_icon_names:
  @device: #IndicatorPowerDevice from which to generate the icon names

  Based on GSD's power plugin, (c) Richard Hughes and licensed GPL >= 2.
  It differs in these ways:

  1. All charging batteries use the same icon regardless of progress.
  <https://bugs.launchpad.net/indicator-power/+bug/824629/comments/7>

  2. For discharging batteries, we decide whether or not to use the 'caution'
  icon based on whether or not we have <= 30 minutes remaining, rather than
  looking at the battery's percentage left.
  <https://bugs.launchpad.net/indicator-power/+bug/743823>

  See also #indicator_power_device_get_gicon.

  Return value: (array zero-terminated=1) (transfer full):
  A GStrv of icon names suitable for passing to g_themed_icon_new_from_names().
  Free with g_strfreev() when done.
*/
GStrv
indicator_power_device_get_icon_names (const IndicatorPowerDevice * device)
{
  gchar ** ret = NULL;
  const gchar *suffix_str;
  const gchar *index_str;

  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);
  /* LCOV_EXCL_STOP */

  gdouble percentage = indicator_power_device_get_percentage (device);
  const UpDeviceKind kind = indicator_power_device_get_kind (device);
  const UpDeviceState state = indicator_power_device_get_state (device);
  const gchar * kind_str = kind_str = up_device_kind_to_string (kind);

  /* get correct icon prefix */
  GString * filename = g_string_new (NULL);

  /* get the icon from some simple rules */
  if (kind == UP_DEVICE_KIND_LINE_POWER) {
    g_string_append (filename, "ac-adapter-symbolic;");
    g_string_append (filename, "ac-adapter;");
  } else if (kind == UP_DEVICE_KIND_MONITOR) {
    g_string_append (filename, "gpm-monitor-symbolic;");
    g_string_append (filename, "gpm-monitor;");
  } else switch (state) {
    case UP_DEVICE_STATE_EMPTY:
      g_string_append (filename, "battery-empty-symbolic;");
      g_string_append_printf (filename, "gpm-%s-empty;", kind_str);
      g_string_append_printf (filename, "gpm-%s-000;", kind_str);
      g_string_append (filename, "battery-empty;");
      break;
    case UP_DEVICE_STATE_FULLY_CHARGED:
      g_string_append (filename, "battery-full-charged-symbolic;");
      g_string_append (filename, "battery-full-charging-symbolic;");
      g_string_append_printf (filename, "gpm-%s-full;", kind_str);
      g_string_append_printf (filename, "gpm-%s-100;", kind_str);
      g_string_append (filename, "battery-full-charged;");
      g_string_append (filename, "battery-full-charging;");
      break;
    case UP_DEVICE_STATE_CHARGING:
    case UP_DEVICE_STATE_PENDING_CHARGE:
      /* When charging, always use the same icon regardless of percentage.
         <http://bugs.launchpad.net/indicator-power/+bug/824629> */
      percentage = 0;

      suffix_str = gpm_upower_get_device_icon_suffix (percentage);
      index_str = gpm_upower_get_device_icon_index (percentage);
      g_string_append_printf (filename, "battery-%s-charging-symbolic;", suffix_str);
      g_string_append_printf (filename, "gpm-%s-%s-charging;", kind_str, index_str);
      g_string_append_printf (filename, "battery-%s-charging;", suffix_str);
      break;
    case UP_DEVICE_STATE_DISCHARGING:
    case UP_DEVICE_STATE_PENDING_DISCHARGE: {
      /* Don't show the caution/red icons unless we have <=30 min left.
         <https://bugs.launchpad.net/indicator-power/+bug/743823>
         Themes use the caution color when the percentage is 0% or 20%,
         so if we have >30 min left, use 30% as the icon's percentage floor */
      if (indicator_power_device_get_time (device) > (30*60))
        percentage = MAX(percentage, 30);

      suffix_str = gpm_upower_get_device_icon_suffix (percentage);
      index_str = gpm_upower_get_device_icon_index (percentage);
      g_string_append_printf (filename, "battery-%s-symbolic;", suffix_str);
      g_string_append_printf (filename, "gpm-%s-%s;", kind_str, index_str);
      g_string_append_printf (filename, "battery-%s;", suffix_str);
      break;
    }
    default:
      g_string_append (filename, "battery-missing-symbolic;");
      g_string_append (filename, "gpm-battery-missing;");
      g_string_append (filename, "battery-missing;");
    }

    ret = g_strsplit (filename->str, ";", -1);
    g_string_free (filename, TRUE);
    return ret;
}

/**
  indicator_power_device_get_gicon:
  @device: #IndicatorPowerDevice to generate the icon names from

  A convenience function to call g_themed_icon_new_from_names()
  with the names returned by indicator_power_device_get_icon_names()

  Return value: (transfer full): A themed GIcon
*/
GIcon *
indicator_power_device_get_gicon (const IndicatorPowerDevice * device)
{
  GStrv names = indicator_power_device_get_icon_names (device);
  GIcon * icon = g_themed_icon_new_from_names (names, -1);
  g_strfreev (names);
  return icon;
}

/***
****
***/

static void
get_timestring (guint64   time_secs,
                gchar   **short_timestring,
                gchar   **detailed_timestring)
{
  gint  hours;
  gint  minutes;

  /* Add 0.5 to do rounding */
  minutes = (int) ( ( time_secs / 60.0 ) + 0.5 );

  if (minutes == 0)
    {
      *short_timestring = g_strdup (_("Unknown time"));
      *detailed_timestring = g_strdup (_("Unknown time"));

      return;
    }

  if (minutes < 60)
    {
      *short_timestring = g_strdup_printf ("0:%.2i", minutes);
      *detailed_timestring = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, "%i minute",
                                              "%i minutes",
                                              minutes), minutes);
      return;
    }

  hours = minutes / 60;
  minutes = minutes % 60;

  *short_timestring = g_strdup_printf ("%i:%.2i", hours, minutes);

  if (minutes == 0)
    {
      *detailed_timestring = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, 
                                              "%i hour",
                                              "%i hours",
                                              hours), hours);
    }
  else
    {
      /* TRANSLATOR: "%i %s %i %s" are "%i hours %i minutes"
       * Swap order with "%2$s %2$i %1$s %1$i if needed */
      *detailed_timestring = g_strdup_printf (_("%i %s %i %s"),
                                              hours, g_dngettext (GETTEXT_PACKAGE, "hour", "hours", hours),
                                              minutes, g_dngettext (GETTEXT_PACKAGE, "minute", "minutes", minutes));
    }
}

static const gchar *
device_kind_to_localised_string (UpDeviceKind kind)
{
  const gchar *text = NULL;

  switch (kind) {
    case UP_DEVICE_KIND_LINE_POWER:
      /* TRANSLATORS: system power cord */
      text = _("AC Adapter");
      break;
    case UP_DEVICE_KIND_BATTERY:
      /* TRANSLATORS: laptop primary battery */
      text = _("Battery");
      break;
    case UP_DEVICE_KIND_UPS:
      /* TRANSLATORS: battery-backed AC power source */
      text = _("UPS");
      break;
    case UP_DEVICE_KIND_MONITOR:
      /* TRANSLATORS: a monitor is a device to measure voltage and current */
      text = _("Monitor");
      break;
    case UP_DEVICE_KIND_MOUSE:
      /* TRANSLATORS: wireless mice with internal batteries */
      text = _("Mouse");
      break;
    case UP_DEVICE_KIND_KEYBOARD:
      /* TRANSLATORS: wireless keyboard with internal battery */
      text = _("Keyboard");
      break;
    case UP_DEVICE_KIND_PDA:
      /* TRANSLATORS: portable device */
      text = _("PDA");
      break;
    case UP_DEVICE_KIND_PHONE:
      /* TRANSLATORS: cell phone (mobile...) */
      text = _("Cell phone");
      break;
    case UP_DEVICE_KIND_MEDIA_PLAYER:
      /* TRANSLATORS: media player, mp3 etc */
      text = _("Media player");
      break;
    case UP_DEVICE_KIND_TABLET:
      /* TRANSLATORS: tablet device */
      text = _("Tablet");
      break;
    case UP_DEVICE_KIND_COMPUTER:
      /* TRANSLATORS: tablet device */
      text = _("Computer");
      break;
    default:
      g_warning ("enum unrecognised: %i", kind);
      text = up_device_kind_to_string (kind);
    }

  return text;
}

void
indicator_power_device_get_time_details (const IndicatorPowerDevice * device,
                                         gchar ** short_details,
                                         gchar ** details,
                                         gchar ** accessible_name)
{
  if (!INDICATOR_IS_POWER_DEVICE(device))
    {
      *short_details = NULL;
      *details = NULL;
      *accessible_name = NULL;
      g_warning ("%s: %p is not an IndicatorPowerDevice", G_STRFUNC, device);
      return;
    }

  const time_t time = indicator_power_device_get_time (device);
  const UpDeviceState state = indicator_power_device_get_state (device);
  const gdouble percentage = indicator_power_device_get_percentage (device);
  const UpDeviceKind kind = indicator_power_device_get_kind (device);
  const gchar * device_name = device_kind_to_localised_string (kind);

  if (time > 0)
    {
      gchar *short_timestring = NULL;
      gchar *detailed_timestring = NULL;

      get_timestring (time,
                      &short_timestring,
                      &detailed_timestring);

      if (state == UP_DEVICE_STATE_CHARGING)
        {
          /* TRANSLATORS: %2 is a time string, e.g. "1 hour 5 minutes" */
          *accessible_name = g_strdup_printf (_("%s (%s to charge (%.0lf%%))"), device_name, detailed_timestring, percentage);
          *details = g_strdup_printf (_("%s (%s to charge)"), device_name, short_timestring);
          *short_details = g_strdup_printf ("(%s)", short_timestring);
        }
      else if ((state == UP_DEVICE_STATE_DISCHARGING) && (time > (60*60*12)))
        {
          *accessible_name = g_strdup_printf (_("%s"), device_name);
          *details = g_strdup_printf (_("%s"), device_name);
          *short_details = g_strdup (short_timestring);
        }
      else
        {
          /* TRANSLATORS: %2 is a time string, e.g. "1 hour 5 minutes" */
          *accessible_name = g_strdup_printf (_("%s (%s left (%.0lf%%))"), device_name, detailed_timestring, percentage);
          *details = g_strdup_printf (_("%s (%s left)"), device_name, short_timestring);
          *short_details = g_strdup (short_timestring);
        }

      g_free (short_timestring);
      g_free (detailed_timestring);
    }
  else if (state == UP_DEVICE_STATE_FULLY_CHARGED)
    {
      *details = g_strdup_printf (_("%s (charged)"), device_name);
      *accessible_name = g_strdup (*details);
      *short_details = g_strdup ("");
    }
  else if (percentage > 0)
    {
      /* TRANSLATORS: %2 is a percentage value. Note: this string is only
       * used when we don't have a time value */
      *details = g_strdup_printf (_("%s (%.0lf%%)"), device_name, percentage);
      *accessible_name = g_strdup (*details);
      *short_details = g_strdup_printf (_("(%.0lf%%)"), percentage);
    }
  else if (kind == UP_DEVICE_KIND_LINE_POWER)
    {
      *details         = g_strdup (device_name);
      *accessible_name = g_strdup (device_name);
      *short_details   = g_strdup (device_name);
    }
  else
    {
      *details = g_strdup_printf (_("%s (not present)"), device_name);
      *accessible_name = g_strdup (*details);
      *short_details = g_strdup (_("(not present)"));
    }
}

/***
****  Instantiation
***/

IndicatorPowerDevice *
indicator_power_device_new (const gchar * object_path,
                            UpDeviceKind  kind,
                            gdouble percentage,
                            UpDeviceState state,
                            time_t timestamp)
{
  GObject * o = g_object_new (INDICATOR_POWER_DEVICE_TYPE,
    INDICATOR_POWER_DEVICE_KIND, kind,
    INDICATOR_POWER_DEVICE_STATE, state,
    INDICATOR_POWER_DEVICE_OBJECT_PATH, object_path,
    INDICATOR_POWER_DEVICE_PERCENTAGE, percentage,
    INDICATOR_POWER_DEVICE_TIME, (guint64)timestamp,
    NULL);
  return INDICATOR_POWER_DEVICE(o);
}

IndicatorPowerDevice *
indicator_power_device_new_from_variant (GVariant * v)
{
  UpDeviceKind kind = UP_DEVICE_KIND_UNKNOWN;
  UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
  const gchar * icon = NULL;
  const gchar * object_path = NULL;
  gdouble percentage = 0;
  guint64 time = 0;

  g_variant_get (v, "(&su&sdut)",
                 &object_path,
                 &kind,
                 &icon,
                 &percentage,
                 &state,
                 &time);

  return indicator_power_device_new (object_path,
                                     kind,
                                     percentage,
                                     state,
                                     time);
}
