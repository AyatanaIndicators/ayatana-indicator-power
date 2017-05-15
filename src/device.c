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

#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include "device.h"

struct _IndicatorPowerDevicePrivate
{
  UpDeviceKind kind;
  UpDeviceState state;
  gchar * object_path;
  gdouble percentage;
  time_t time;

  /* Timestamp of when when we first noticed that upower couldn't estimate
     the time-remaining field for this device, or 0 if not applicable.
     This is used when generating the time-remaining string. */
  GTimer * inestimable;
  gboolean power_supply;
};

/* Properties */
/* Enum for the properties so that they can be quickly found and looked up. */
enum {
  PROP_0,
  PROP_KIND,
  PROP_STATE,
  PROP_OBJECT_PATH,
  PROP_PERCENTAGE,
  PROP_TIME,
  PROP_POWER_SUPPLY,
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
G_DEFINE_TYPE (IndicatorPowerDevice, indicator_power_device, G_TYPE_OBJECT)
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

  properties[PROP_POWER_SUPPLY] = g_param_spec_boolean (INDICATOR_POWER_DEVICE_POWER_SUPPLY,
                                                        "power supply",
                                                        "The device's power supply",
                                                        FALSE,
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
  priv->power_supply = FALSE;

  self->priv = priv;
}

static void
indicator_power_device_dispose (GObject *object)
{
  IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(object);
  IndicatorPowerDevicePrivate * p = self->priv;

  g_clear_pointer (&p->inestimable, g_timer_destroy);

  G_OBJECT_CLASS (indicator_power_device_parent_class)->dispose (object);
}

static void
indicator_power_device_finalize (GObject *object)
{
  IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(object);
  IndicatorPowerDevicePrivate * priv = self->priv;

  g_clear_pointer (&priv->object_path, g_free);

  G_OBJECT_CLASS (indicator_power_device_parent_class)->finalize (object);
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
        g_value_set_uint64 (value, (guint64)priv->time);
        break;

      case PROP_POWER_SUPPLY:
        g_value_set_boolean (value, priv->power_supply);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(o, prop_id, pspec);
        break;
    }
}

static void
set_property (GObject * o, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(o);
  IndicatorPowerDevicePrivate * p = self->priv;

  switch (prop_id)
    {
      case PROP_KIND:
        p->kind = (UpDeviceKind) g_value_get_int (value);
        break;

      case PROP_STATE:
        p->state = (UpDeviceState) g_value_get_int (value);
        break;

      case PROP_OBJECT_PATH:
        g_free (p->object_path);
        p->object_path = g_value_dup_string (value);
        break;

      case PROP_PERCENTAGE:
        p->percentage = g_value_get_double (value);
        break;

      case PROP_TIME:
        p->time = (time_t) g_value_get_uint64(value);
        break;

      case PROP_POWER_SUPPLY:
        p->power_supply = g_value_get_boolean (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(o, prop_id, pspec);
        break;
    }

  /**
   * Check to see if the time-remaining value is estimable.
   * When it first becomes inestimable, kick off a timer because
   * we need to track that to generate the appropriate title text.
   */

  const gboolean is_inestimable = (p->time == 0)
                               && (p->state != UP_DEVICE_STATE_FULLY_CHARGED)
                               && (p->percentage > 0);

  if (!is_inestimable)
    {
      g_clear_pointer (&p->inestimable, g_timer_destroy);
    }
  else if (p->inestimable == NULL)
    {
      p->inestimable = g_timer_new ();
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

gboolean
indicator_power_device_get_power_supply (const IndicatorPowerDevice * device)
{
  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), FALSE);
  /* LCOV_EXCL_STOP */

  return device->priv->power_supply;
}

/***
****
****
***/

static const gchar *
get_device_icon_suffix (gdouble percentage)
{
  if (percentage >= 60) return "full";
  if (percentage >= 30) return "good";
  if (percentage >= 10) return "low";
  return "caution";
}

static const gchar *
get_closest_10_percent_percentage (gdouble percentage)
{
  if (percentage >= 95) return "100";
  if (percentage >= 85) return "090";
  if (percentage >= 75) return "080";
  if (percentage >= 65) return "070";
  if (percentage >= 55) return "060";
  if (percentage >= 45) return "050";
  if (percentage >= 35) return "040";
  if (percentage >= 21) return "030"; /* don't round down to 20: see bug #1388235 */
  if (percentage >= 15) return "020";
  if (percentage >=  5) return "010";
  return "000";
}

static const gchar *
get_fallback_device_icon_index (gdouble percentage)
{
  if (percentage >= 90) return "100";
  if (percentage >= 70) return "080";
  if (percentage >= 50) return "060";
  if (percentage >  20) return "040"; /* don't round down to 20: see bug #1559731 */
  if (percentage >= 10) return "020";
  return "000";
}

static const char *
device_kind_to_string (UpDeviceKind kind)
{
  switch (kind)
    {
      case UP_DEVICE_KIND_LINE_POWER: return "line-power";
      case UP_DEVICE_KIND_BATTERY: return "battery";
      case UP_DEVICE_KIND_UPS: return "ups";
      case UP_DEVICE_KIND_MONITOR: return "monitor";
      case UP_DEVICE_KIND_MOUSE: return "mouse";
      case UP_DEVICE_KIND_KEYBOARD: return "keyboard";
      case UP_DEVICE_KIND_PDA: return "pda";
      case UP_DEVICE_KIND_PHONE: return "phone";
      case UP_DEVICE_KIND_MEDIA_PLAYER: return "media-player";
      case UP_DEVICE_KIND_TABLET: return "tablet";
      case UP_DEVICE_KIND_COMPUTER: return "computer";
      default: return "unknown";
    }
}

/**
  indicator_power_device_get_icon_names:
  @device: #IndicatorPowerDevice from which to generate the icon names

  See also indicator_power_device_get_gicon().

  Return value: (array zero-terminated=1) (transfer full):
  A GStrv of icon names suitable for passing to g_themed_icon_new_from_names().
  Free with g_strfreev() when done.
*/
GStrv
indicator_power_device_get_icon_names (const IndicatorPowerDevice * device)
{
  const gchar *suffix_str;
  const gchar *index_str;
  const gchar *index_str_2;

  /* LCOV_EXCL_START */
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);
  /* LCOV_EXCL_STOP */

  gdouble percentage = indicator_power_device_get_percentage (device);
  const UpDeviceKind kind = indicator_power_device_get_kind (device);
  const UpDeviceState state = indicator_power_device_get_state (device);
  const gchar * kind_str = device_kind_to_string (kind);

  GPtrArray * names = g_ptr_array_new ();

  if (kind == UP_DEVICE_KIND_LINE_POWER)
    {
      g_ptr_array_add (names, g_strdup("ac-adapter-symbolic"));
      g_ptr_array_add (names, g_strdup("ac-adapter"));
    }
  else if (kind == UP_DEVICE_KIND_MONITOR)
    {
      g_ptr_array_add (names, g_strdup("gpm-monitor-symbolic"));
      g_ptr_array_add (names, g_strdup("gpm-monitor"));
    }
  else switch (state)
    {
      case UP_DEVICE_STATE_EMPTY:
        g_ptr_array_add (names, g_strdup_printf("%s-empty-symbolic", kind_str));
        g_ptr_array_add (names, g_strdup_printf("gpm-%s-empty", kind_str));
        g_ptr_array_add (names, g_strdup_printf("gpm-%s-000", kind_str));
        g_ptr_array_add (names, g_strdup_printf("%s-empty", kind_str));
        break;

      case UP_DEVICE_STATE_FULLY_CHARGED:
        g_ptr_array_add (names, g_strdup_printf("%s-full-charged-symbolic", kind_str));
        g_ptr_array_add (names, g_strdup_printf("%s-full-charging-symbolic", kind_str));
        g_ptr_array_add (names, g_strdup_printf("gpm-%s-full", kind_str));
        g_ptr_array_add (names, g_strdup_printf("gpm-%s-100", kind_str));
        g_ptr_array_add (names, g_strdup_printf("%s-full-charged", kind_str));
        g_ptr_array_add (names, g_strdup_printf("%s-full-charging", kind_str));
        break;

      case UP_DEVICE_STATE_CHARGING:
        suffix_str = get_device_icon_suffix (percentage);
        index_str = get_closest_10_percent_percentage (percentage);
        g_ptr_array_add (names, g_strdup_printf ("%s-%s-charging", kind_str, index_str));
        g_ptr_array_add (names, g_strdup_printf ("gpm-%s-%s-charging", kind_str, index_str));
        index_str_2 = get_fallback_device_icon_index (percentage);
        if (g_strcmp0 (index_str, index_str_2))
          {
            g_ptr_array_add (names, g_strdup_printf ("%s-%s-charging", kind_str, index_str_2));
            g_ptr_array_add (names, g_strdup_printf ("gpm-%s-%s-charging", kind_str, index_str_2));
          }
        g_ptr_array_add (names, g_strdup_printf ("%s-%s-charging-symbolic", kind_str, suffix_str));
        g_ptr_array_add (names, g_strdup_printf ("%s-%s-charging", kind_str, suffix_str));
        // NB: fallthrough to use foo-bar as a fallback for foo-bar-charging

      case UP_DEVICE_STATE_PENDING_CHARGE:
      case UP_DEVICE_STATE_DISCHARGING:
      case UP_DEVICE_STATE_PENDING_DISCHARGE:
      case UP_DEVICE_STATE_UNKNOWN: /* http://pad.lv/1470080 */
        suffix_str = get_device_icon_suffix (percentage);
        index_str = get_closest_10_percent_percentage (percentage);
        g_ptr_array_add (names, g_strdup_printf ("%s-%s", kind_str, index_str));
        g_ptr_array_add (names, g_strdup_printf ("gpm-%s-%s", kind_str, index_str));
        index_str_2 = get_fallback_device_icon_index (percentage);
        if (g_strcmp0 (index_str, index_str_2))
          {
            g_ptr_array_add (names, g_strdup_printf ("%s-%s", kind_str, index_str_2));
            g_ptr_array_add (names, g_strdup_printf ("gpm-%s-%s", kind_str, index_str_2));
          }
        g_ptr_array_add (names, g_strdup_printf ("%s-%s-symbolic", kind_str, suffix_str));
        g_ptr_array_add (names, g_strdup_printf ("%s-%s", kind_str, suffix_str));
        break;

      default:
        g_ptr_array_add (names, g_strdup_printf("%s-missing-symbolic", kind_str));
        g_ptr_array_add (names, g_strdup_printf("gpm-%s-missing", kind_str));
        g_ptr_array_add (names, g_strdup_printf("%s-missing", kind_str));
    }

    g_ptr_array_add (names, NULL); /* terminates the strv */
    return (GStrv) g_ptr_array_free (names, FALSE);
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
    case UP_DEVICE_KIND_UNKNOWN:
      /* TRANSLATORS: unknown device */
      text = _("Unknown");
      break;
    default:
      g_warning ("enum unrecognised: %i", kind);
      text = device_kind_to_string (kind);
    }

  return text;
}

/**
 * The '''brief time-remaining string''' for a component should be:
 *  * the time remaining for it to empty or fully charge,
 *    if estimable, in H:MM format; otherwise
 *  * “estimating…” if the time remaining has been inestimable for
 *    less than 30 seconds; otherwise
 *  * “unknown” if the time remaining has been inestimable for
 *    between 30 seconds and one minute; otherwise
 *  * the empty string.
 */
static char *
get_brief_time_remaining (const IndicatorPowerDevice * device)
{
  gchar * str = NULL;
  const IndicatorPowerDevicePrivate * p = device->priv;

  if (p->time > 0)
    {
      int minutes = p->time / 60;
      const int hours = minutes / 60;
      minutes %= 60;

      str = g_strdup_printf("%0d:%02d", hours, minutes);
    }
  else if (p->inestimable != NULL)
    {
      const double elapsed = g_timer_elapsed (p->inestimable, NULL);

      if (elapsed < 30)
        {
          str = g_strdup_printf (_("estimating…"));
        }
      else if (elapsed < 60)
        {
          str = g_strdup_printf (_("unknown"));
        }
    }

  return str;
}

/**
 * The '''expanded time-remaining string''' for a component should
 * be the same as the brief time-remaining string, except that if
 * the time is estimable:
 *  * if the component is charging, it should be “H:MM to charge”
 *  * if the component is discharging, it should be “H:MM left”.
 */
static char*
get_expanded_time_remaining (const IndicatorPowerDevice * device)
{
  char * str = NULL;
  const IndicatorPowerDevicePrivate * p = device->priv;

  if (p->time && ((p->state == UP_DEVICE_STATE_CHARGING) || (p->state == UP_DEVICE_STATE_DISCHARGING)))
    {
      int minutes = p->time / 60;
      const int hours = minutes / 60;
      minutes %= 60;

      if (p->state == UP_DEVICE_STATE_CHARGING)
        {
          /* TRANSLATORS: H:MM (hours, minutes) to charge the battery. Example: "1:30 to charge" */
          str = g_strdup_printf (_("%0d:%02d to charge"), hours, minutes);
        }
      else // discharging
        {
          /* TRANSLATORS: H:MM (hours, minutes) to discharge the battery. Example: "1:30 left"*/
          str = g_strdup_printf (_("%0d:%02d left"), hours, minutes);
        }
    }
  else
    {
      str = get_brief_time_remaining (device);
    }

  return str;
}

/**
 * The '''accessible time-remaining string''' for a component
 * should be the same as the expanded time-remaining string,
 * except the H:MM time should be rendered as “''H'' hours ''M'' minutes”,
 * or just as “''M'' minutes” if the time is less than one hour.
 */
static char *
get_accessible_time_remaining (const IndicatorPowerDevice * device)
{
  char * str = NULL;
  const IndicatorPowerDevicePrivate * p = device->priv;

  if (p->time && ((p->state == UP_DEVICE_STATE_CHARGING) || (p->state == UP_DEVICE_STATE_DISCHARGING)))
    {
      guint minutes = (guint)p->time / 60u;
      const guint hours = minutes / 60u;
      minutes %= 60;

      if (p->state == UP_DEVICE_STATE_CHARGING)
        {
          if (hours > 0)
            {
              /* TRANSLATORS: "X (hour,hours) Y (minute,minutes) to charge" the battery.
                 Example: "1 hour 10 minutes to charge" */
              str = g_strdup_printf (_("%d %s %d %s to charge"),
                          hours, g_dngettext (NULL, "hour", "hours", hours),
                          minutes, g_dngettext (NULL, "minute", "minutes", minutes));
           }
         else
           {
              /* TRANSLATORS: "Y (minute,minutes) to charge" the battery.
                 Example: "59 minutes to charge" */
              str = g_strdup_printf (_("%d %s to charge"),
                          minutes, g_dngettext (NULL, "minute", "minutes", minutes));
           }
        }
      else // discharging
        {
          if (hours > 0)
            {
              /* TRANSLATORS: "X (hour,hours) Y (minute,minutes) left" until the battery's empty.
                 Example: "1 hour 10 minutes left" */
              str = g_strdup_printf (_("%d %s %d %s left"),
                          hours, g_dngettext (NULL, "hour", "hours", hours),
                          minutes, g_dngettext (NULL, "minute", "minutes", minutes));
            }
          else
            {
              /* TRANSLATORS: "Y (minute,minutes) left" until the battery's empty.
                 Example: "59 minutes left" */
              str = g_strdup_printf (_("%d %s left"),
                          minutes, g_dngettext (NULL, "minute", "minutes", minutes));
            }
        }
    }
  else
    {
      str = get_brief_time_remaining (device);
    }

  return str;
}

/**
 * The time is relevant for a device if either (a) the component is charging,
 * or (b) the component is discharging and the estimated time is less than
 * 24 hours. (A time greater than 24 hours is probably a mistaken calculation.)
 */
static gboolean
time_is_relevant (const IndicatorPowerDevice * device)
{
  const IndicatorPowerDevicePrivate * p = device->priv;

  if (p->state == UP_DEVICE_STATE_CHARGING)
    return TRUE;

  if ((p->state == UP_DEVICE_STATE_DISCHARGING) && (p->time<(24*60*60)))
    return TRUE;

  return FALSE;
}

/**
 * The menu item for each chargeable component should consist of ...
 * Text representing the name of the component (“Battery”, “Mouse”,
 * “UPS”, “Alejandra’s iPod”, etc) and the charge status in brackets:
 *
 *  * “X (charged)” if it is fully charged and not discharging;
 *  * “X (expanded time-remaining string)” if it is charging,
 *    or discharging with less than 24 hours left;
 *  * “X” if it is discharging with 24 hours or more left. 
 *
 * The accessible label for the menu item should be the same as the
 * visible label, except with the accessible time-remaining string
 * instead of the expanded time-remaining string. 
 */
static char *
get_menuitem_text (const IndicatorPowerDevice * device,
                   gboolean                     accessible)
{
  char * str = NULL;
  const IndicatorPowerDevicePrivate * p = device->priv;
  const char * kind_str = device_kind_to_localised_string (p->kind);

  if (p->state == UP_DEVICE_STATE_FULLY_CHARGED)
    {
      /* TRANSLATORS: example: "battery (charged)" */
      str = g_strdup_printf (_("%s (charged)"), kind_str);
    }
  else
    {
      char * time_str = NULL;

      if (time_is_relevant (device))
        {
          if (accessible)
            time_str = get_accessible_time_remaining (device);
          else
            time_str = get_expanded_time_remaining (device);
        }

      if (time_str && *time_str)
        {
          /* TRANSLATORS: example: "battery (time remaining)" */
          str = g_strdup_printf (_("%s (%s)"), kind_str, time_str);
        }
      else
        {
          str = g_strdup (kind_str);
        }

      g_free (time_str);
    }

  return str;
}

char *
indicator_power_device_get_readable_text (const IndicatorPowerDevice * device)
{
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);

  return get_menuitem_text (device, FALSE);
}

char *
indicator_power_device_get_accessible_text (const IndicatorPowerDevice * device)
{
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);

  return get_menuitem_text (device, TRUE);
}

/**
 * If the time is relevant and/or “Show Percentage in Menu Bar” is checked,
 * the icon should be followed by brackets.
 *
 * If the time is relevant, the brackets should contain the time-remaining
 * string for that component.
 *
 * If “Show Percentage in Menu Bar” is checked (as it should not be by default),
 * the brackets should contain the percentage charge for that device.
 *
 * If both conditions are true, the time and percentage should be separated by a space. 
 */
char*
indicator_power_device_get_readable_title (const IndicatorPowerDevice * device,
                                           gboolean                     want_time,
                                           gboolean                     want_percent)
{
  char * str = NULL;
  char * time_str = NULL;
  const IndicatorPowerDevicePrivate * p;

  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);

  p = device->priv;

  // if we can't provide time-remaining, turn off the time flag
  if (want_time && !time_is_relevant (device))
    want_time = FALSE;

  // if we can't provide percent, turn off the percent flag
  if (p->percentage < 0.01)
    want_percent = FALSE;

  // try to build the time-remaining string
  if (want_time)
    {
      time_str = get_brief_time_remaining (device);
      want_time = time_str && *time_str;
    }

  if (want_time && want_percent)
    {
      /* TRANSLATORS: after the icon, a time-remaining string + battery %. Example: "(0:59, 33%)" */
      str = g_strdup_printf (_("(%s, %.0lf%%)"), time_str, p->percentage);
    }
  else if (want_time)
    {
      /* TRANSLATORS: after the icon, a time-remaining string Example: "(0:59)" */
      str = g_strdup_printf (_("(%s)"), time_str);
    }
  else if (want_percent)
    {
      /* TRANSLATORS: after the icon, a battery %. Example: "(33%)" */
      str = g_strdup_printf (_("(%.0lf%%)"), p->percentage);
    }
  else
    {
      str = NULL;
    }

  g_free (time_str);
  return str;
}

/**
 * Regardless, the accessible name for the whole menu title should be the same
 * as the accessible name for that thing’s component inside the menu itself. 
 */
char *
indicator_power_device_get_accessible_title (const IndicatorPowerDevice * device,
                                             gboolean                     want_time G_GNUC_UNUSED,
                                             gboolean                     want_percent G_GNUC_UNUSED)
{
  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), NULL);

  return indicator_power_device_get_accessible_text (device);
}

/***
****  Instantiation
***/

IndicatorPowerDevice *
indicator_power_device_new (const gchar * object_path,
                            UpDeviceKind  kind,
                            gdouble percentage,
                            UpDeviceState state,
                            time_t timestamp,
                            gboolean power_supply)
{
  GObject * o = g_object_new (INDICATOR_POWER_DEVICE_TYPE,
    INDICATOR_POWER_DEVICE_KIND, kind,
    INDICATOR_POWER_DEVICE_STATE, state,
    INDICATOR_POWER_DEVICE_OBJECT_PATH, object_path,
    INDICATOR_POWER_DEVICE_PERCENTAGE, percentage,
    INDICATOR_POWER_DEVICE_TIME, (guint64)timestamp,
    INDICATOR_POWER_DEVICE_POWER_SUPPLY, power_supply,
    NULL);
  return INDICATOR_POWER_DEVICE(o);
}

IndicatorPowerDevice *
indicator_power_device_new_from_variant (GVariant * v)
{
  g_return_val_if_fail (g_variant_is_of_type (v, G_VARIANT_TYPE("(susdutb)")), NULL);

  UpDeviceKind kind = UP_DEVICE_KIND_UNKNOWN;
  UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
  const gchar * icon = NULL;
  const gchar * object_path = NULL;
  gdouble percentage = 0;
  guint64 time = 0;
  gboolean power_supply = FALSE;

  g_variant_get (v, "(&su&sdutb)",
                 &object_path,
                 &kind,
                 &icon,
                 &percentage,
                 &state,
                 &time,
                 &power_supply);

  return indicator_power_device_new (object_path,
                                     kind,
                                     percentage,
                                     state,
                                     (time_t)time,
                                     power_supply);
}
