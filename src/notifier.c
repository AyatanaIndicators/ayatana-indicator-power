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
  LAST_PROP
};

static GParamSpec * properties[LAST_PROP];

static int n_notifiers = 0;

struct _IndicatorPowerNotifierPrivate
{
  IndicatorPowerDeviceProvider * device_provider;
  IndicatorPowerDevice * primary_device;
  gdouble battery_level;
  time_t time_remaining;
  NotifyNotification* notify_notification;
};

typedef IndicatorPowerNotifierPrivate priv_t;

/***
****
***/

static void
emit_critical_signal(IndicatorPowerNotifier * self G_GNUC_UNUSED)
{
  g_message("FIXME %s %s", G_STRFUNC, G_STRLOC);
}

static void
emit_hide_signal(IndicatorPowerNotifier * self G_GNUC_UNUSED)
{
  g_message("FIXME %s %s", G_STRFUNC, G_STRLOC);
}

static void
emit_show_signal(IndicatorPowerNotifier * self G_GNUC_UNUSED)
{
  g_message("FIXME %s %s", G_STRFUNC, G_STRLOC);
}

static void
notification_clear(IndicatorPowerNotifier * self)
{
  priv_t * p = self->priv;

  if (p->notify_notification != NULL)
    {
      notify_notification_clear_actions(p->notify_notification);
      g_signal_handlers_disconnect_by_data(p->notify_notification, self);
      g_clear_object(&p->notify_notification);
      emit_hide_signal(self);
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
  if (p->notify_notification != NULL)
    {
      notification_clear (self);
    }

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
      emit_show_signal(self);
    }

  g_free (body);
}

static void
on_battery_level_changed(IndicatorPowerNotifier * self        G_GNUC_UNUSED,
                         IndicatorPowerDevice   * device,
                         gdouble                  old_value,
                         gdouble                  new_value)
{
  static const double critical_level = 2.0;
  static const double very_low_level = 5.0;
  static const double low_level = 48.0;

  if (indicator_power_device_get_state(device) != UP_DEVICE_STATE_DISCHARGING)
    return;

g_message ("%s - %s - %f - %f", G_STRFUNC, indicator_power_device_get_object_path(device), old_value, new_value);

  if ((old_value > critical_level) && (new_value <= critical_level))
    {
      emit_critical_signal(self);
    }
  else if ((old_value > very_low_level) && (new_value <= very_low_level))
    {
      notification_show(self, device);
    }
  else if ((old_value > low_level) && (new_value <= low_level))
    {
      notification_show(self, device);
    }
}

static void
on_devices_changed(IndicatorPowerNotifier * self)
{
  priv_t * p = self->priv;
  GList * devices;

  // find the primary device
  devices = indicator_power_device_provider_get_devices(p->device_provider);
  g_clear_object(&p->primary_device);
  p->primary_device = indicator_power_service_choose_primary_device (devices);
  g_list_free_full (devices, (GDestroyNotify)g_object_unref);

  if (p->primary_device != NULL)
    {
      // test for battery level change
      const gdouble old_level = (int)(p->battery_level*1000) ? p->battery_level : 100.0;
      const gdouble new_level = indicator_power_device_get_percentage(p->primary_device);
      if ((int)(old_level*1000) != (int)(new_level*1000))
          on_battery_level_changed (self, p->primary_device, old_level, new_level);

      p->battery_level = new_level;
      p->time_remaining = indicator_power_device_get_time (p->primary_device);
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

  indicator_power_notifier_set_device_provider(self, NULL);
  notification_clear(self);

  G_OBJECT_CLASS (indicator_power_notifier_parent_class)->dispose (o);
}

static void
my_finalize (GObject * o G_GNUC_UNUSED)
{
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

  //p->battery_levels = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_variant_unref);

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
    "device-provider",
    "Device Provider",
    "Source for power devices",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

/***
****  Public API
***/

IndicatorPowerNotifier *
indicator_power_notifier_new (IndicatorPowerDeviceProvider * device_provider)
{
  GObject * o = g_object_new (INDICATOR_TYPE_POWER_NOTIFIER,
                              "device-provider", device_provider,
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
      g_clear_object(&p->primary_device);
    }

  if (dp != NULL)
    {
      p->device_provider = g_object_ref(dp);

      g_signal_connect_swapped(p->device_provider, "devices-changed",
                               G_CALLBACK(on_devices_changed), self);

      on_devices_changed(self);
    }
}
