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

#include "dbus-battery-info.h"
#include "dbus-shared.h"
#include "device.h"
#include "device-provider.h"
#include "notifier.h"
#include "service.h"

#include <libnotify/notify.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

G_DEFINE_TYPE(IndicatorPowerNotifier,
              indicator_power_notifier,
              G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_DEVICE_PROVIDER,
  PROP_IS_WARNING,
  PROP_POWER_LEVEL,
  LAST_PROP
};

#define DEVICE_PROVIDER_NAME "device-provider"
#define IS_WARNING_NAME "is-warning"
#define POWER_LEVEL_NAME "power-level"

static GParamSpec * properties[LAST_PROP];

static int n_notifiers = 0;

typedef enum
{
  POWER_LEVEL_OK,
  POWER_LEVEL_LOW,
  POWER_LEVEL_VERY_LOW,
  POWER_LEVEL_CRITICAL
}
PowerLevel;

struct _IndicatorPowerNotifierPrivate
{
  IndicatorPowerDeviceProvider * device_provider;

  IndicatorPowerDevice * battery;
  NotifyNotification* notify_notification;
  gboolean is_warning;
  PowerLevel power_level;

  DbusBattery * dbus_battery;
  GBinding * is_warning_binding;
  GBinding * power_level_binding;
  GDBusConnection * bus;
};

typedef IndicatorPowerNotifierPrivate priv_t;

/***
****
***/

/* implemented here rather than my_set_property() to guard from public use
   because this is a read-only property */
static void
set_is_warning_property (IndicatorPowerNotifier * self, gboolean is_warning)
{
  priv_t * p = self->priv;

  if (p->is_warning != is_warning)
    {
      p->is_warning = is_warning;

      g_object_notify_by_pspec (G_OBJECT(self), properties[PROP_IS_WARNING]);
    }
}

/* implemented here rather than my_set_property() to guard from public use
   because this is a read-only property */
static void
set_power_level_property (IndicatorPowerNotifier * self, PowerLevel power_level)
{
  priv_t * p = self->priv;

  if (p->power_level != power_level)
    {
      p->power_level = power_level;

      g_object_notify_by_pspec (G_OBJECT(self), properties[PROP_POWER_LEVEL]);
    }
}

/***
****
***/

static void
notification_clear (IndicatorPowerNotifier * self)
{
  priv_t * p = self->priv;

  if (p->notify_notification != NULL)
    {
      set_is_warning_property (self, FALSE);

      notify_notification_clear_actions(p->notify_notification);
      g_signal_handlers_disconnect_by_data(p->notify_notification, self);
      g_clear_object(&p->notify_notification);

    }
}

static void
on_notification_clicked(NotifyNotification * notify_notification G_GNUC_UNUSED,
                        char * action G_GNUC_UNUSED,
                        gpointer gself G_GNUC_UNUSED)
{
  /* no-op */
}

static void
notification_show(IndicatorPowerNotifier * self,
                  IndicatorPowerDevice   * device)
                   
{
  priv_t * p = self->priv;
  char * body;
  NotifyNotification * nn;

  // if there's already a notification, tear it down
  notification_clear (self);

  // create the notification
  body = g_strdup_printf(_("%d%% charge remaining"), (int)indicator_power_device_get_percentage(device));
  p->notify_notification = nn = notify_notification_new(_("Battery Low"), body, NULL);
  notify_notification_set_hint(nn, "x-canonical-snap-decisions", g_variant_new_boolean(TRUE));
  notify_notification_set_hint(nn, "x-canonical-private-button-tint", g_variant_new_boolean(TRUE));
  notify_notification_add_action(nn, "OK", _("OK"), on_notification_clicked, self, NULL);
  g_signal_connect_swapped(nn, "closed", G_CALLBACK(notification_clear), self);

  // show the notification
  GError* error = NULL;
  notify_notification_show(nn, &error);
  if (error != NULL)
    {
      g_critical("Unable to show snap decision for '%s': %s", body, error->message);
      g_error_free(error);
    }
  else
    {
      set_is_warning_property (self, TRUE);
    }

  g_free (body);
}

static PowerLevel
get_power_level (const IndicatorPowerDevice * device)
{
  static const double percent_critical = 2.0;
  static const double percent_very_low = 5.0;
  static const double percent_low = 48.0;
  const gdouble p = indicator_power_device_get_percentage(device);
  PowerLevel ret;

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
  
static void
on_devices_changed(IndicatorPowerNotifier * self)
{
  priv_t * p = self->priv;
  GList * devices;
  IndicatorPowerDevice * primary;

  /* find the primary battery */
  devices = indicator_power_device_provider_get_devices (p->device_provider);
  primary = indicator_power_service_choose_primary_device (devices);
  g_clear_object (&p->battery);
  if ((primary != NULL) && (indicator_power_device_get_kind (primary) == UP_DEVICE_KIND_BATTERY))
    p->battery = g_object_ref (primary);
  g_clear_object(&primary);
  g_list_free_full (devices, (GDestroyNotify)g_object_unref);

  /* update our state based on the new primary device */
  if (p->battery == NULL)
    {
      /* if there's no primary battery, put everything in standby mode */
      set_power_level_property (self, POWER_LEVEL_OK);
      notification_clear(self);
    }
  else
    {
      const PowerLevel power_level = get_power_level (p->battery);

      if (p->power_level != power_level)
        {
          set_power_level_property (self, power_level);

          /* maybe update the notifications */
          if ((power_level == POWER_LEVEL_OK) ||
              (indicator_power_device_get_state(p->battery) != UP_DEVICE_STATE_DISCHARGING))
            {
              notification_clear (self);
            }
          else
            {
              notification_show (self, p->battery);
            }
        }
    }
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
  IndicatorPowerNotifier * self = INDICATOR_POWER_NOTIFIER (o);
  priv_t * p = self->priv;

  switch (property_id)
    {
      case PROP_DEVICE_PROVIDER:
        g_value_set_object (value, p->device_provider);
        break;

      case PROP_POWER_LEVEL:
        g_value_set_int (value, p->power_level);
        break;

      case PROP_IS_WARNING:
        g_value_set_boolean (value, p->is_warning);
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
  IndicatorPowerNotifier * self = INDICATOR_POWER_NOTIFIER (o);

  switch (property_id)
    {
      case PROP_DEVICE_PROVIDER:
        indicator_power_notifier_set_device_provider (self, g_value_get_object (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
    }
}

static void
my_dispose (GObject * o)
{
  IndicatorPowerNotifier * self = INDICATOR_POWER_NOTIFIER(o);
  priv_t * p = self->priv;
g_message ("%s %s dispose %p", G_STRLOC, G_STRFUNC, (void*)o);

  indicator_power_notifier_set_bus(self, NULL);
  notification_clear(self);
  indicator_power_notifier_set_device_provider (self, NULL);

  g_clear_pointer (&p->power_level_binding, g_binding_unbind);
  g_clear_pointer (&p->is_warning_binding, g_binding_unbind);
  g_clear_object (&p->dbus_battery);

  G_OBJECT_CLASS (indicator_power_notifier_parent_class)->dispose (o);
}

static void
my_finalize (GObject * o)
{
g_message ("%s %s finalize %p", G_STRLOC, G_STRFUNC, (void*)o);
  if (!--n_notifiers)
    notify_uninit();
}

/***
****  Instantiation
***/

static void
indicator_power_notifier_init (IndicatorPowerNotifier * self)
{
  priv_t * p = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_TYPE_POWER_NOTIFIER,
                                            IndicatorPowerNotifierPrivate);
  self->priv = p;
g_message ("%s %s init %p", G_STRLOC, G_STRFUNC, (void*)self);

  p->dbus_battery = dbus_battery_skeleton_new ();

  /* FIXME: own the busname and export the skeleton */

  p->is_warning_binding = g_object_bind_property (self,
                                                  IS_WARNING_NAME,
                                                  p->dbus_battery,
                                                  IS_WARNING_NAME,
                                                  G_BINDING_SYNC_CREATE);

  p->power_level_binding = g_object_bind_property (self,
                                                   POWER_LEVEL_NAME,
                                                   p->dbus_battery,
                                                   POWER_LEVEL_NAME,
                                                   G_BINDING_SYNC_CREATE);

  if (!n_notifiers++ && !notify_init("indicator-power-service"))
    g_critical("Unable to initialize libnotify! Notifications might not be shown.");
}

static void
indicator_power_notifier_class_init (IndicatorPowerNotifierClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = my_dispose;
  object_class->finalize = my_finalize;
  object_class->get_property = my_get_property;
  object_class->set_property = my_set_property;

  g_type_class_add_private (klass, sizeof (IndicatorPowerNotifierPrivate));

  properties[PROP_0] = NULL;

  properties[PROP_DEVICE_PROVIDER] = g_param_spec_object (
    DEVICE_PROVIDER_NAME,
    "Device Provider",
    "Source for power devices",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_POWER_LEVEL] = g_param_spec_int (
    POWER_LEVEL_NAME,
    "Power Level",
    "Power Level of the batteries",
    POWER_LEVEL_OK,
    POWER_LEVEL_CRITICAL,
    POWER_LEVEL_OK,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_IS_WARNING] = g_param_spec_boolean (
    IS_WARNING_NAME,
    "Is Warning",
    "Whether or not we're currently warning the user about a low battery",
    FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

/***
****  Public API
***/

IndicatorPowerNotifier *
indicator_power_notifier_new (IndicatorPowerDeviceProvider * device_provider)
{
  GObject * o = g_object_new (INDICATOR_TYPE_POWER_NOTIFIER,
                              DEVICE_PROVIDER_NAME, device_provider,
                              NULL);

  return INDICATOR_POWER_NOTIFIER (o);
}

void
indicator_power_notifier_set_device_provider(IndicatorPowerNotifier * self,
                                             IndicatorPowerDeviceProvider * dp)
{
  priv_t * p;

  g_return_if_fail(INDICATOR_IS_POWER_NOTIFIER(self));
  g_return_if_fail(!dp || INDICATOR_IS_POWER_DEVICE_PROVIDER(dp));
  p = self->priv;

  if (p->device_provider != NULL)
    {
      g_signal_handlers_disconnect_by_data(p->device_provider, self);
      g_clear_object(&p->device_provider);
      g_clear_object(&p->battery);
    }

  if (dp != NULL)
    {
      p->device_provider = g_object_ref(dp);

      g_signal_connect_swapped(p->device_provider, "devices-changed",
                               G_CALLBACK(on_devices_changed), self);

      on_devices_changed(self);
    }
}

void
indicator_power_notifier_set_bus (IndicatorPowerNotifier * self,
                                  GDBusConnection * bus)
{
  priv_t * p;

  g_return_if_fail(INDICATOR_IS_POWER_NOTIFIER(self));
  p = self->priv;

  if (p->bus != NULL)
    {
      if (p->dbus_battery != NULL)
        {
          g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON(p->dbus_battery));
        }

      g_clear_object (&p->bus);
    }

  if (bus != NULL)
    {
      GError * error;

      p->bus = g_object_ref (bus);

      error = NULL;
      g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(p->dbus_battery),
                                       bus,
                                       BUS_PATH"/Battery",
                                       &error);
      if (error != NULL)
        {
          g_warning ("Unable to export LowBattery properties: %s", error->message);
          g_error_free (error);
        }
    }
}





