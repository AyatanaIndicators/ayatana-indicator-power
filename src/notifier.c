/*
 * Copyright 2014-2016 Canonical Ltd.
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

#include "datafiles.h"
#include "dbus-accounts-sound.h"
#include "dbus-battery.h"
#include "dbus-shared.h"
#include "notifier.h"
#include "utils.h"
#include "sound-player.h"

#ifdef HAS_URLDISPATCHER
#include <lomiri-url-dispatcher.h>
#endif

#include <libnotify/notify.h>

#include <glib/gi18n.h>

#include <stdint.h> /* UINT32_MAX */

typedef enum
{
  POWER_LEVEL_CRITICAL,
  POWER_LEVEL_VERY_LOW,
  POWER_LEVEL_LOW,
  POWER_LEVEL_OK
}
PowerLevel;

/**
***  GObject Properties
**/

enum
{
  PROP_0,
  PROP_BATTERY,
  PROP_SOUND_PLAYER,
  LAST_PROP
};

#define PROP_BATTERY_NAME "battery"
#define PROP_SOUND_PLAYER_NAME "sound-player"

static GParamSpec * properties[LAST_PROP];

static int instance_count = 0;

/**
***
**/

typedef struct
{
  /* The battery we're currently watching.
     This may be a physical battery or it may be an aggregated
     battery from multiple batteries present on the device.
     See indicator_power_service_choose_primary_device() and
     bug #880881 */
  IndicatorPowerDevice * battery;
  PowerLevel power_level;
  gboolean discharging;

  NotifyNotification * notify_notification;

  GDBusConnection * bus;
  DbusBattery * dbus_battery; /* org.ayatana.indicator.power.Battery skeleton */

  gboolean caps_queried;
  gboolean actions_supported;

  IndicatorPowerSoundPlayer * sound_player;

  GCancellable * cancellable;
  DbusAccountsServiceSound * accounts_service_sound_proxy;
}
IndicatorPowerNotifierPrivate;

typedef IndicatorPowerNotifierPrivate priv_t;

G_DEFINE_TYPE_WITH_PRIVATE(IndicatorPowerNotifier,
                           indicator_power_notifier,
                           G_TYPE_OBJECT)

#define get_priv(o) ((priv_t*)indicator_power_notifier_get_instance_private(o))

/***
****
***/

static const char *
power_level_to_dbus_string (const PowerLevel power_level)
{
  switch (power_level)
    {
      case POWER_LEVEL_LOW:      return POWER_LEVEL_STR_LOW;
      case POWER_LEVEL_VERY_LOW: return POWER_LEVEL_STR_VERY_LOW;
      case POWER_LEVEL_CRITICAL: return POWER_LEVEL_STR_CRITICAL;
      default:                   return POWER_LEVEL_STR_OK;
    }
}

static PowerLevel
get_battery_power_level (IndicatorPowerDevice * battery)
{
  static const double percent_critical = 2.0;
  static const double percent_very_low = 5.0;
  static const double percent_low = 10.0;
  gdouble p;
  PowerLevel ret;

  g_return_val_if_fail(battery != NULL, POWER_LEVEL_OK);
  g_return_val_if_fail(indicator_power_device_get_kind(battery) == UP_DEVICE_KIND_BATTERY, POWER_LEVEL_OK);

  p = indicator_power_device_get_percentage(battery);

  if (p <= percent_critical)
    ret = POWER_LEVEL_CRITICAL;
  else if (p <= percent_very_low)
    ret = POWER_LEVEL_VERY_LOW;
  else if (p <= percent_low)
    ret = POWER_LEVEL_LOW;
  else
    ret = POWER_LEVEL_OK;

  return ret;
}

/***
****  Sounds
***/

static void
on_sound_proxy_ready (GObject      * source_object G_GNUC_UNUSED,
                      GAsyncResult * res,
                      gpointer       gself)
{
  GError * error;
  DbusAccountsServiceSound * proxy;

  error = NULL;
  proxy = dbus_accounts_service_sound_proxy_new_for_bus_finish (res, &error);
  if (error != NULL)
    {
      if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("%s Couldn't find accounts service sound proxy: %s", G_STRLOC, error->message);

      g_clear_error(&error);
    }
  else
    {
      IndicatorPowerNotifier * const self = INDICATOR_POWER_NOTIFIER(gself);
      priv_t * const p = get_priv (self);
      p->accounts_service_sound_proxy = proxy;
    }
}

static gboolean
silent_mode (IndicatorPowerNotifier * self)
{
  priv_t * const p = get_priv (self);

  return (p->accounts_service_sound_proxy != NULL)
      && (dbus_accounts_service_sound_get_silent_mode(p->accounts_service_sound_proxy));
}

static void
play_low_battery_sound (IndicatorPowerNotifier * self)
{
  const gchar * const key = LOW_BATTERY_SOUND;
  gchar * filename;
  priv_t * const p = get_priv(self);

  /* can't play? */
  g_return_if_fail (p->sound_player != NULL);

  /* won't play? */
  if (silent_mode(self))
    return;

  filename = datafile_find(DATAFILE_TYPE_SOUND, key);
  if (filename != NULL)
    {
      gchar * uri = g_filename_to_uri(filename, NULL, NULL);
      indicator_power_sound_player_play_uri (p->sound_player, uri);
      g_free(uri);
      g_free(filename);
    }
  else
    {
      g_warning("Unable to find '%s' in XDG data dirs", key);
    }
}

/***
****  Notifications
***/

static void
on_notify_notification_finalized (gpointer gself, GObject * dead)
{
  IndicatorPowerNotifier * const self = INDICATOR_POWER_NOTIFIER(gself);
  priv_t * const p = get_priv(self);
  g_return_if_fail ((void*)(p->notify_notification) == (void*)dead);
  p->notify_notification = NULL;
  dbus_battery_set_is_warning (p->dbus_battery, FALSE);
}

static void
notification_clear (IndicatorPowerNotifier * self)
{
  priv_t * const p = get_priv(self);
  NotifyNotification * nn;

  if ((nn = p->notify_notification))
    {
      GError * error = NULL;

      g_object_weak_unref(G_OBJECT(nn), on_notify_notification_finalized, self);

      if (!notify_notification_close(nn, &error))
        {
          g_warning("Unable to close notification: %s", error->message);
          g_error_free(error);
        }

      p->notify_notification = NULL;
      dbus_battery_set_is_warning (p->dbus_battery, FALSE);
    }
}

static void
on_battery_settings_clicked(NotifyNotification * nn        G_GNUC_UNUSED,
                            char               * action    G_GNUC_UNUSED,
                            gpointer             user_data G_GNUC_UNUSED)
{
  utils_handle_settings_request();
}

static void
on_dismiss_clicked(NotifyNotification * nn        G_GNUC_UNUSED,
                   char               * action    G_GNUC_UNUSED,
                   gpointer             user_data G_GNUC_UNUSED)
{
  /* no-op; libnotify warns if we have a NULL action callback */
}

static gboolean
are_actions_supported(IndicatorPowerNotifier * self)
{
  priv_t * const p = get_priv(self);

  if (!p->caps_queried)
    {
      gboolean actions_supported;
      GList * caps;
      GList * l;

      /* see if actions are supported */
      actions_supported = FALSE;
      caps = notify_get_server_caps();
      for (l=caps; l!=NULL && !actions_supported; l=l->next)
        if (!g_strcmp0(l->data, "actions"))
          actions_supported = TRUE;

      p->actions_supported = actions_supported;
      p->caps_queried = TRUE;

      g_list_free_full(caps, g_free);
    }

  return p->actions_supported;
}

static void
notification_show(IndicatorPowerNotifier * self)
{
  priv_t * const p = get_priv(self);
  gdouble pct;
  const char * title;
  char * body;
  GStrv icon_names;
  const char * icon_name;
  NotifyNotification * nn;
  GError * error;
  const PowerLevel power_level = get_battery_power_level(p->battery);

  notification_clear(self);

  g_return_if_fail(power_level != POWER_LEVEL_OK);

  /* create the notification */
  title = power_level == POWER_LEVEL_LOW
        ? _("Battery Low")
        : _("Battery Critical");
  pct = indicator_power_device_get_percentage(p->battery);
  body = g_strdup_printf(_("%.0f%% charge remaining"), pct);
  icon_names = indicator_power_device_get_icon_names(p->battery);
  if (icon_names && *icon_names)
    icon_name = icon_names[0];
  else
    icon_name = NULL;
  nn = notify_notification_new(title, body, icon_name);
  g_strfreev (icon_names);
  g_free (body);

  if (are_actions_supported(self))
    {
      notify_notification_set_hint(nn, "x-canonical-snap-decisions", g_variant_new_string("true"));
      notify_notification_set_hint(nn, "x-canonical-non-shaped-icon", g_variant_new_string("true"));
      notify_notification_set_hint(nn, "x-canonical-private-affirmative-tint", g_variant_new_string("true"));
      notify_notification_set_hint(nn, "x-canonical-snap-decisions-timeout", g_variant_new_int32(INT32_MAX));
      notify_notification_set_timeout(nn, NOTIFY_EXPIRES_NEVER);
      notify_notification_add_action(nn, "dismiss", _("OK"), on_dismiss_clicked, NULL, NULL);
      notify_notification_add_action(nn, "settings", _("Battery settings"), on_battery_settings_clicked, NULL, NULL);
    }

  /* if we can show it, keep it */
  error = NULL;
  if (notify_notification_show(nn, &error))
    {
      p->notify_notification = nn;
      g_signal_connect(nn, "closed", G_CALLBACK(g_object_unref), NULL);
      g_object_weak_ref(G_OBJECT(nn), on_notify_notification_finalized, self);
      dbus_battery_set_is_warning (p->dbus_battery, TRUE);
    }
  else
    {
      g_critical("Unable to show snap decision for '%s': %s", body, error->message);
      g_error_free(error);
      g_object_unref(nn);
    }
}

/***
****
***/

static void
on_battery_property_changed (IndicatorPowerNotifier * self)
{
  priv_t * p;
  PowerLevel old_power_level;
  PowerLevel new_power_level;
  gboolean old_discharging;
  gboolean new_discharging;

  g_return_if_fail(INDICATOR_IS_POWER_NOTIFIER(self));
  p = get_priv (self);
  g_return_if_fail(INDICATOR_IS_POWER_DEVICE(p->battery));

  old_power_level = p->power_level;
  new_power_level = get_battery_power_level (p->battery);

  old_discharging = p->discharging;
  new_discharging = indicator_power_device_get_state(p->battery) == UP_DEVICE_STATE_DISCHARGING;

  /* pop up a 'low battery' notification if either:
     a) it's already discharging, and its PowerLevel worsens, OR
     b) it's already got a bad PowerLevel and its state becomes 'discharging */
  if ((new_discharging && (old_power_level > new_power_level)) ||
      ((new_power_level != POWER_LEVEL_OK) && new_discharging && !old_discharging))
    {
      notification_show (self);
      play_low_battery_sound (self);
    }
  else if (!new_discharging || (new_power_level == POWER_LEVEL_OK))
    {
      notification_clear (self);
    }

  dbus_battery_set_power_level (p->dbus_battery, power_level_to_dbus_string (new_power_level));
  p->power_level = new_power_level;
  p->discharging = new_discharging;
}

/***
****  GObject virtual functions
***/

static void
my_get_property (GObject     * o,
                 guint         property_id,
                 GValue      * value,
                 GParamSpec  * pspec)
{
  IndicatorPowerNotifier * const self = INDICATOR_POWER_NOTIFIER (o);
  priv_t * const p = get_priv (self);

  switch (property_id)
    {
      case PROP_BATTERY:
        g_value_set_object (value, p->battery);
        break;

      case PROP_SOUND_PLAYER:
        g_value_set_object (value, p->sound_player);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
    }
}

static void
my_set_property (GObject       * o,
                 guint           property_id,
                 const GValue  * value,
                 GParamSpec    * pspec)
{
  IndicatorPowerNotifier * const self = INDICATOR_POWER_NOTIFIER (o);

  switch (property_id)
    {
      case PROP_BATTERY:
        indicator_power_notifier_set_battery (self, g_value_get_object(value));
        break;

      case PROP_SOUND_PLAYER:
        indicator_power_notifier_set_sound_player (self, g_value_get_object(value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
    }
}

static void
my_dispose (GObject * o)
{
  IndicatorPowerNotifier * const self = INDICATOR_POWER_NOTIFIER(o);
  priv_t * const p = get_priv (self);

  if (p->cancellable != NULL)
    {
      g_cancellable_cancel(p->cancellable);
      g_clear_object(&p->cancellable);
    }

  indicator_power_notifier_set_bus (self, NULL);
  indicator_power_notifier_set_sound_player (self, NULL);
  notification_clear (self);
  indicator_power_notifier_set_battery (self, NULL);
  g_clear_object (&p->dbus_battery);
  g_clear_object (&p->accounts_service_sound_proxy);

  G_OBJECT_CLASS (indicator_power_notifier_parent_class)->dispose (o);
}

static void
my_finalize (GObject * o G_GNUC_UNUSED)
{
  /* FIXME: This is an awkward place to put this. 
     Ordinarily something like this would go in main(), but we need libnotify
     to clean itself up before shutting down the bus in the unit tests as well. */
  if (!--instance_count)
    notify_uninit();
}

/***
****  Instantiation
***/

static void
indicator_power_notifier_init (IndicatorPowerNotifier * self)
{
  priv_t * const p = get_priv (self);

  /* bind the read-only properties so they'll get pushed to the bus */

  p->dbus_battery = dbus_battery_skeleton_new ();

  p->power_level = POWER_LEVEL_OK;

  p->cancellable = g_cancellable_new();

  if (!instance_count++ && !notify_init("ayatana-indicator-power-service"))
    g_critical("Unable to initialize libnotify! Notifications might not be shown.");

  gchar* object_path = g_strdup_printf("/org/freedesktop/Accounts/User%lu", (gulong)getuid());
  dbus_accounts_service_sound_proxy_new_for_bus(
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
    "org.freedesktop.Accounts",
    object_path,
    p->cancellable,
    on_sound_proxy_ready,
    self);
  g_clear_pointer(&object_path, g_free);
}

static void
indicator_power_notifier_class_init (IndicatorPowerNotifierClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = my_dispose;
  object_class->finalize = my_finalize;
  object_class->get_property = my_get_property;
  object_class->set_property = my_set_property;

  properties[PROP_BATTERY] = g_param_spec_object (
    PROP_BATTERY_NAME,
    "Battery",
    "The current battery",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_SOUND_PLAYER] = g_param_spec_object (
    PROP_SOUND_PLAYER_NAME,
    "Sound Player",
    "The current battery",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

/***
****  Public API
***/

IndicatorPowerNotifier *
indicator_power_notifier_new (IndicatorPowerSoundPlayer * sound_player)
{
  GObject * o = g_object_new (INDICATOR_TYPE_POWER_NOTIFIER,
                              PROP_SOUND_PLAYER_NAME, sound_player,
                              NULL);

  return INDICATOR_POWER_NOTIFIER (o);
}

void
indicator_power_notifier_set_battery (IndicatorPowerNotifier * self,
                                      IndicatorPowerDevice   * battery)
{
  priv_t * p;

  g_return_if_fail(INDICATOR_IS_POWER_NOTIFIER(self));
  g_return_if_fail((battery == NULL) || INDICATOR_IS_POWER_DEVICE(battery));
  g_return_if_fail((battery == NULL) || (indicator_power_device_get_kind(battery) == UP_DEVICE_KIND_BATTERY));

  p = get_priv (self);

  if (p->battery == battery)
    return;

  if (p->battery != NULL)
    {
      g_signal_handlers_disconnect_by_data (p->battery, self);
      g_clear_object (&p->battery);
      dbus_battery_set_power_level (p->dbus_battery, power_level_to_dbus_string (POWER_LEVEL_OK));
      notification_clear (self);
    }

  if (battery != NULL)
    {
      p->battery = g_object_ref (battery);
      g_signal_connect_swapped (p->battery, "notify::"INDICATOR_POWER_DEVICE_PERCENTAGE,
                                G_CALLBACK(on_battery_property_changed), self);
      g_signal_connect_swapped (p->battery, "notify::"INDICATOR_POWER_DEVICE_STATE,
                                G_CALLBACK(on_battery_property_changed), self);
      on_battery_property_changed (self);
    }
}

void
indicator_power_notifier_set_sound_player (IndicatorPowerNotifier * self,
                                           IndicatorPowerSoundPlayer * sound_player)
{
  priv_t * p;

  g_return_if_fail(INDICATOR_IS_POWER_NOTIFIER(self));
  g_return_if_fail((sound_player == NULL) || INDICATOR_IS_POWER_SOUND_PLAYER(sound_player));

  p = get_priv (self);

  if (p->sound_player == sound_player)
    return;

  g_clear_object(&p->sound_player);

  if (sound_player != NULL)
      p->sound_player = g_object_ref(sound_player);
}

void
indicator_power_notifier_set_bus (IndicatorPowerNotifier * self,
                                  GDBusConnection        * bus)
{
  priv_t * p;
  GDBusInterfaceSkeleton * skel;

  g_return_if_fail(INDICATOR_IS_POWER_NOTIFIER(self));
  g_return_if_fail((bus == NULL) || G_IS_DBUS_CONNECTION(bus));

  p = get_priv (self);

  if (p->bus == bus)
    return;

  skel = G_DBUS_INTERFACE_SKELETON(p->dbus_battery);

  if (p->bus != NULL)
    {
      if (skel != NULL)
        g_dbus_interface_skeleton_unexport (skel);

      g_clear_object (&p->bus);
    }

  if (bus != NULL)
    {
      GError * error;

      p->bus = g_object_ref (bus);

      error = NULL;
      if (!g_dbus_interface_skeleton_export(skel,
                                            bus,
                                            BUS_PATH"/Battery",
                                            &error))
        {
          g_warning ("Unable to export LowBattery properties: %s", error->message);
          g_error_free (error);
        }
    }
}

const char *
indicator_power_notifier_get_power_level (IndicatorPowerDevice * battery)
{
  return power_level_to_dbus_string (get_battery_power_level (battery));
}
