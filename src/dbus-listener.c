/*

Listens on DBus for Power changes and passes them to an IndicatorPower

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

#include "dbus-listener.h"
#include "device.h"

struct _IndicatorPowerDbusListenerPrivate
{
  GCancellable * cancellable;
  GDBusProxy * proxy;
  guint watcher_id;
};

#define INDICATOR_POWER_DBUS_LISTENER_GET_PRIVATE(o) (INDICATOR_POWER_DBUS_LISTENER(o)->priv)

/* Signals */
enum {
  SIGNAL_DEVICES,
  SIGNAL_LAST
};
static guint signals[SIGNAL_LAST] = { 0 };


/* GObject stuff */
static void indicator_power_dbus_listener_class_init (IndicatorPowerDbusListenerClass *klass);
static void indicator_power_dbus_listener_init       (IndicatorPowerDbusListener *self);
static void indicator_power_dbus_listener_dispose    (GObject *object);
static void indicator_power_dbus_listener_finalize   (GObject *object);

static void gsd_appeared_callback (GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);

/* LCOV_EXCL_START */
G_DEFINE_TYPE (IndicatorPowerDbusListener, indicator_power_dbus_listener, G_TYPE_OBJECT);
/* LCOV_EXCL_STOP */

static void
indicator_power_dbus_listener_class_init (IndicatorPowerDbusListenerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (IndicatorPowerDbusListenerPrivate));

  /* methods */
  object_class->dispose = indicator_power_dbus_listener_dispose;
  object_class->finalize = indicator_power_dbus_listener_finalize;

  /* signals */
  signals[SIGNAL_DEVICES] = g_signal_new (INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED,
                                          G_TYPE_FROM_CLASS(klass), 0,
                                          G_STRUCT_OFFSET (IndicatorPowerDbusListenerClass, devices_enumerated),
                                          NULL, NULL,
                                          g_cclosure_marshal_VOID__POINTER,
                                          G_TYPE_NONE, 1, G_TYPE_POINTER, G_TYPE_NONE);
}

/* Initialize an instance */
static void
indicator_power_dbus_listener_init (IndicatorPowerDbusListener *self)
{
  IndicatorPowerDbusListenerPrivate * priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                      INDICATOR_POWER_DBUS_LISTENER_TYPE,
                                      IndicatorPowerDbusListenerPrivate);

  priv->cancellable = g_cancellable_new ();

  priv->watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                       GSD_SERVICE,
                                       G_BUS_NAME_WATCHER_FLAGS_NONE,
                                       gsd_appeared_callback,
                                       NULL,
                                       self,
                                       NULL);

  self->priv = priv;
}

static void
indicator_power_dbus_listener_dispose (GObject *object)
{
  IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(object);
  IndicatorPowerDbusListenerPrivate * priv = self->priv;

  g_clear_object (&priv->proxy);
  g_clear_object (&priv->cancellable);

  G_OBJECT_CLASS (indicator_power_dbus_listener_parent_class)->dispose (object);
}

static void
indicator_power_dbus_listener_finalize (GObject *object)
{
  G_OBJECT_CLASS (indicator_power_dbus_listener_parent_class)->finalize (object);
}

/***
****
***/

static void
get_devices_cb (GObject      * source_object,
                GAsyncResult * res,
                gpointer       user_data)
{
  GError *error;
  GSList * devices = NULL;
  GVariant * devices_container;
  IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER (user_data);

  /* build an array of IndicatorPowerDevices from the DBus response */
  error = NULL;
  devices_container = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (devices_container == NULL)
    {
      g_message ("Couldn't get devices: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      gsize i;
      GVariant * devices_variant = g_variant_get_child_value (devices_container, 0);
      const int device_count = devices_variant ? g_variant_n_children (devices_variant) : 0;
      
      for (i=0; i<device_count; i++)
        {
          GVariant * v = g_variant_get_child_value (devices_variant, i);
          devices = g_slist_prepend (devices, indicator_power_device_new_from_variant (v));
          g_variant_unref (v);
        }

      devices = g_slist_reverse (devices);

      g_variant_unref (devices_variant);
      g_variant_unref (devices_container);
    }

  g_signal_emit (self, signals[SIGNAL_DEVICES], (GQuark)0, devices);
  g_slist_free_full (devices, g_object_unref);
}

static void
request_device_list (IndicatorPowerDbusListener * self)
{
  g_dbus_proxy_call (self->priv->proxy,
                     "GetDevices",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     self->priv->cancellable,
                     get_devices_cb,
                     self);
}

static void
receive_properties_changed (GDBusProxy *proxy                  G_GNUC_UNUSED,
                            GVariant   *changed_properties     G_GNUC_UNUSED,
                            GStrv       invalidated_properties G_GNUC_UNUSED,
                            gpointer    user_data)
{
  request_device_list (INDICATOR_POWER_DBUS_LISTENER(user_data));
}


static void
service_proxy_cb (GObject      *object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
  GError * error = NULL;
  IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(user_data);
  IndicatorPowerDbusListenerPrivate * priv = self->priv;

  priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error != NULL)
    {
      g_error ("Error creating proxy: %s", error->message);
      g_error_free (error);
      return;
    }

  /* we want to change the primary device changes */
  g_signal_connect (priv->proxy, "g-properties-changed",
                    G_CALLBACK (receive_properties_changed), user_data);

  /* get the initial state */
  request_device_list (self);
}

static void
gsd_appeared_callback (GDBusConnection *connection,
                       const gchar     *name,
                       const gchar     *name_owner,
                       gpointer         user_data)
{
  IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(user_data);
  IndicatorPowerDbusListenerPrivate * priv = self->priv;

  g_dbus_proxy_new (connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                    NULL,
                    name,
                    GSD_POWER_DBUS_PATH,
                    GSD_POWER_DBUS_INTERFACE,
                    priv->cancellable,
                    service_proxy_cb,
                    self);
}
