/*
 * Copyright 2013 Canonical Ltd.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
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
 */

#include "config.h"

#include "dbus-upower.h"
#include "dbus-upower-device.h"
#include "device.h"
#include "device-provider.h"
#include "device-provider-upower.h"

#define BUS_NAME "org.freedesktop.UPower"
#define BUS_PATH "/org/freedesktop/UPower"

/***
****  private struct
***/

struct _IndicatorPowerDeviceProviderUPowerPriv
{
  DbusUPower * upower_proxy;
  GHashTable * devices;
  GCancellable * cancellable;
  guint timer;
};

typedef IndicatorPowerDeviceProviderUPowerPriv priv_t;

/***
****  GObject boilerplate
***/

static void indicator_power_device_provider_interface_init (
                                IndicatorPowerDeviceProviderInterface * iface);

G_DEFINE_TYPE_WITH_CODE (
  IndicatorPowerDeviceProviderUPower,
  indicator_power_device_provider_upower,
  G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (INDICATOR_TYPE_POWER_DEVICE_PROVIDER,
                         indicator_power_device_provider_interface_init));

/***
****  TIMER
***/

/*
 * Rebuilds are needed whenever upower devices are added, changed, or removed.
 * 
 * Events often come in batches. For example, unplugging a power cable
 * triggers a device-removed signal, and also a device-changed as the
 * battery's state changes to 'discharging'.
 *
 * We use a small timer here to fold upower events into a single
 * IndicatorPowerDeviceProvider devices-changed signal.
 */

static gboolean
on_timer (gpointer gself)
{
  IndicatorPowerDeviceProvider * provider;
  IndicatorPowerDeviceProviderUPower * provider_upower;

  provider = INDICATOR_POWER_DEVICE_PROVIDER (gself);
  indicator_power_device_provider_emit_devices_changed (provider);

  provider_upower = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER (gself);
  provider_upower->priv->timer = 0;
  return G_SOURCE_REMOVE;
}

static void
emit_devices_changed_soon (IndicatorPowerDeviceProviderUPower * self)
{
  if (self->priv->timer == 0)
    self->priv->timer = g_timeout_add_seconds (1, on_timer, self);
}

/***
****  UPOWER DBUS
***/

static void
on_upower_device_proxy_ready (GObject * o, GAsyncResult * res, gpointer gself)
{
  GError * err;
  DbusDevice * tmp;

  err = NULL;
  tmp = dbus_device_proxy_new_for_bus_finish (res, &err);
  if (err != NULL)
    {
      g_warning ("Unable to get UPower Device Proxy: %s", err->message);
      g_error_free (err);
    }
  else
    {
      IndicatorPowerDevice * device;
      IndicatorPowerDeviceProviderUPower * self;
      priv_t * p;

      self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER (gself);
      p = self->priv;

      const guint kind = dbus_device_get_type_ (tmp);
      const gdouble percentage = dbus_device_get_percentage (tmp);
      const guint state = dbus_device_get_state (tmp);
      const gint64 time_to_empty = dbus_device_get_time_to_empty (tmp);
      const gint64 time_to_full = dbus_device_get_time_to_full (tmp);
      const time_t time = time_to_empty ? time_to_empty : time_to_full;
      const char * path = g_dbus_proxy_get_object_path (G_DBUS_PROXY (tmp));

      device = indicator_power_device_new (path,
                                           kind,
                                           percentage,
                                           state,
                                           time);

      g_hash_table_insert (p->devices,
                           g_strdup (path),
                           g_object_ref (device));

      emit_devices_changed_soon (self);

      g_object_unref (device);
      g_object_unref (tmp);
    }
}

static void
update_device_from_object_path (IndicatorPowerDeviceProviderUPower * self,
                                const char                         * path)
{
  dbus_device_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 BUS_NAME,
                                 path,
                                 self->priv->cancellable,
                                 on_upower_device_proxy_ready,
                                 self);
}

static void
on_upower_device_enumerations_ready (GObject       * proxy,
                                     GAsyncResult  * res,
                                     gpointer        gself)
{
  GError * err;
  char ** object_paths;

  err = NULL;
  dbus_upower_call_enumerate_devices_finish (DBUS_UPOWER(proxy),
                                             &object_paths,
                                             res,
                                             &err);

  if (err != NULL)
    {
      g_warning ("Unable to get UPower devices: %s", err->message);
      g_error_free (err);
    }
  else
    {
      guint i;

      for (i=0; object_paths && object_paths[i]; i++)
        update_device_from_object_path (gself, object_paths[i]);

      g_strfreev (object_paths);
    }
}

static void
on_upower_device_added (DbusUPower * unused G_GNUC_UNUSED,
                        const char * object_path,
                        gpointer     gself)
{
  update_device_from_object_path (gself, object_path);
}

static void
on_upower_device_changed (DbusUPower * unused G_GNUC_UNUSED,
                          const char * object_path,
                          gpointer     gself)
{
  update_device_from_object_path (gself, object_path);
}

static void
on_upower_device_removed (DbusUPower * unused G_GNUC_UNUSED,
                          const char * object_path,
                          gpointer     gself)
{
  IndicatorPowerDeviceProviderUPower * self;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER (gself);
  g_hash_table_remove (self->priv->devices, object_path);
  emit_devices_changed_soon (self);
}

static void
on_upower_proxy_ready (GObject        * source G_GNUC_UNUSED,
                       GAsyncResult   * res,
                       gpointer         gself)
{
  GError * err;
  DbusUPower * proxy;

  err = NULL;
  proxy = dbus_upower_proxy_new_for_bus_finish (res, &err);
  if (err != NULL)
    {
      g_warning ("Unable to get UPower proxy: %s", err->message);
      g_error_free (err);
    }
  else
    {
      IndicatorPowerDeviceProviderUPower * self;
      priv_t * p;

      self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER (gself);
      p = self->priv;

      p->upower_proxy = proxy;
      g_signal_connect (proxy, "device-changed",
                        G_CALLBACK (on_upower_device_changed), self);
      g_signal_connect (proxy, "device-added",
                        G_CALLBACK (on_upower_device_added), self);
      g_signal_connect (proxy, "device-removed",
                        G_CALLBACK (on_upower_device_removed), self);

      dbus_upower_call_enumerate_devices (p->upower_proxy,
                                          p->cancellable,
                                          on_upower_device_enumerations_ready,
                                          self);
    }
}

/***
****  IndicatorPowerDeviceProvider virtual functions
***/

static GList *
my_get_devices (IndicatorPowerDeviceProvider * provider)
{
  GList * devices;
  IndicatorPowerDeviceProviderUPower * self;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(provider);

  devices = g_hash_table_get_values (self->priv->devices);
  g_list_foreach (devices, (GFunc)g_object_ref, NULL);
  return devices;
}

/***
****  GObject virtual functions
***/

static void
my_dispose (GObject * o)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(o);
  p = self->priv;

  if (p->cancellable != NULL)
    {
      g_cancellable_cancel (p->cancellable);

      g_clear_object (&p->cancellable);
    }

  if (p->timer != 0)
    {
      g_source_remove (p->timer);

      p->timer = 0;
    }
  
  g_clear_object (&p->upower_proxy);

  g_clear_pointer (&p->devices, g_hash_table_destroy);

  G_OBJECT_CLASS (indicator_power_device_provider_upower_parent_class)->dispose (o);
}

/***
****  Instantiation
***/

static void
indicator_power_device_provider_upower_class_init (IndicatorPowerDeviceProviderUPowerClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = my_dispose;

  g_type_class_add_private (klass,
                            sizeof (IndicatorPowerDeviceProviderUPowerPriv));
}

static void
indicator_power_device_provider_interface_init (IndicatorPowerDeviceProviderInterface * iface)
{
  iface->get_devices = my_get_devices;
}

static void
indicator_power_device_provider_upower_init (IndicatorPowerDeviceProviderUPower * self)
{
  IndicatorPowerDeviceProviderUPowerPriv * p;

  p = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                   INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER,
                                   IndicatorPowerDeviceProviderUPowerPriv);

  self->priv = p;

  p->cancellable = g_cancellable_new ();

  p->devices = g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      g_free,
                                      g_object_unref);

  dbus_upower_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                 G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                                 BUS_NAME,
                                 BUS_PATH,
                                 p->cancellable,
                                 on_upower_proxy_ready,
                                 self);
}

/***
****  Public API
***/

IndicatorPowerDeviceProvider *
indicator_power_device_provider_upower_new (void)
{
  gpointer o = g_object_new (INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER, NULL);

  return INDICATOR_POWER_DEVICE_PROVIDER (o);
}
