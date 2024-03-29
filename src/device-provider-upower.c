/*
 * Copyright 2013 Canonical Ltd.
 * Copyright 2023 Robert Tari
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 *   Robert Tari <robert@tari.in>
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

#include "device.h"
#include "device-provider.h"
#include "device-provider-upower.h"

#define BUS_NAME "org.freedesktop.UPower"

#define MGR_IFACE "org.freedesktop.UPower"
#define MGR_PATH  "/org/freedesktop/UPower"

#define DISPLAY_DEVICE_PATH "/org/freedesktop/UPower/devices/DisplayDevice"

/***
****  private struct
***/

typedef struct
{
  GDBusConnection * bus;
  GCancellable * cancellable;

  /* dbus object path --> IndicatorPowerDevice */
  GHashTable * devices;

  /* a hashset of paths whose devices need to be refreshed */
  GHashTable * queued_paths;

  /* when this timer fires, the queued_paths will be refreshed */
  guint queued_paths_timer;

  GSList* subscriptions;

  guint name_tag;
}
IndicatorPowerDeviceProviderUPowerPrivate;

typedef IndicatorPowerDeviceProviderUPowerPrivate priv_t;

#define get_priv(o) ((priv_t*)indicator_power_device_provider_upower_get_instance_private(o))


/***
****  GObject boilerplate
***/

static void indicator_power_device_provider_interface_init (
                                IndicatorPowerDeviceProviderInterface * iface);

G_DEFINE_TYPE_WITH_CODE (
  IndicatorPowerDeviceProviderUPower,
  indicator_power_device_provider_upower,
  G_TYPE_OBJECT,
  G_ADD_PRIVATE(IndicatorPowerDeviceProviderUPower)
  G_IMPLEMENT_INTERFACE (INDICATOR_TYPE_POWER_DEVICE_PROVIDER,
                         indicator_power_device_provider_interface_init))

/***
****  UPOWER DBUS
***/

struct device_get_all_data
{
  char * path;
  IndicatorPowerDeviceProviderUPower * self;
};

static void
emit_devices_changed (IndicatorPowerDeviceProviderUPower * self)
{
  indicator_power_device_provider_emit_devices_changed (INDICATOR_POWER_DEVICE_PROVIDER (self));
}

static void
on_get_all_response (GObject * o, GAsyncResult * res, gpointer gdata)
{
  struct device_get_all_data * data = gdata;
  GError * error;
  GVariant * response;

  error = NULL;
  response = g_dbus_connection_call_finish (G_DBUS_CONNECTION(o), res, &error);
  if (error != NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("Error getting properties for UPower device '%s': %s",
                   data->path, error->message);

      g_error_free (error);
    }
  else
    {
      guint32 kind = 0;
      gchar *model;
      guint32 state = 0;
      gdouble percentage = 0;
      gint64 time_to_empty = 0;
      gint64 time_to_full = 0;
      gint64 time;
      gboolean power_supply = FALSE;
      IndicatorPowerDevice * device;
      priv_t * p = get_priv(data->self);
      GVariant * dict = g_variant_get_child_value (response, 0);

      g_variant_lookup (dict, "Type", "u", &kind);
      g_variant_lookup (dict, "Model", "s", &model);
      g_variant_lookup (dict, "State", "u", &state);
      g_variant_lookup (dict, "Percentage", "d", &percentage);
      g_variant_lookup (dict, "TimeToEmpty", "x", &time_to_empty);
      g_variant_lookup (dict, "TimeToFull", "x", &time_to_full);
      g_variant_lookup (dict, "PowerSupply", "b", &power_supply);
      time = time_to_empty ? time_to_empty : time_to_full;

      if ((device = g_hash_table_lookup (p->devices, data->path)))
        {
          g_object_set (device, INDICATOR_POWER_DEVICE_KIND, (gint)kind,
                                INDICATOR_POWER_DEVICE_MODEL, model,
                                INDICATOR_POWER_DEVICE_STATE, (gint)state,
                                INDICATOR_POWER_DEVICE_OBJECT_PATH, data->path,
                                INDICATOR_POWER_DEVICE_PERCENTAGE, percentage,
                                INDICATOR_POWER_DEVICE_TIME, time,
                                INDICATOR_POWER_DEVICE_POWER_SUPPLY, power_supply,
                                NULL);
        }
      else
        {
          device = indicator_power_device_new (data->path,
                                               kind,
                                               model,
                                               percentage,
                                               state,
                                               (time_t)time,
                                               power_supply);

          g_hash_table_insert (p->devices,
                               g_strdup (data->path),
                               g_object_ref (device));

          g_object_unref (device);
        }

      emit_devices_changed (data->self);
      g_variant_unref (dict);
      g_variant_unref (response);
    }

  g_free (data->path);
  g_slice_free (struct device_get_all_data, data);
}

static void
update_device_from_object_path (IndicatorPowerDeviceProviderUPower * self,
                                const char                         * path)
{
  priv_t * p = get_priv(self);
  struct device_get_all_data * data;

  /* Symbolic composite item. Nice idea! But its composite rules
     differ from Design's so (for now) don't use it.
     https://wiki.ubuntu.com/Power#Handling_multiple_batteries */
  if (!g_strcmp0(path, DISPLAY_DEVICE_PATH))
    return;

  data = g_slice_new (struct device_get_all_data);
  data->path = g_strdup (path);
  data->self = self;

  g_dbus_connection_call(p->bus,
                         BUS_NAME,
                         path,
                         "org.freedesktop.DBus.Properties",
                         "GetAll",
                         g_variant_new ("(s)", "org.freedesktop.UPower.Device"),
                         G_VARIANT_TYPE("(a{sv})"),
                         G_DBUS_CALL_FLAGS_NO_AUTO_START,
                         -1, /* default timeout */
                         p->cancellable,
                         on_get_all_response,
                         data);
}

/*
 * UPower 0.99 added proper PropertyChanged signals, but before that
 * it MGR_IFACE emitted a DeviceChanged signal which didn't tell which
 * property changed, so all properties had to get refreshed w/GetAll().
 *
 * Changes often come in bursts, so this timer tries to fold them together
 * by waiting a small bit before making calling GetAll().
 */

/* rebuild all the devices listed in our queued_paths hashset */
static gboolean
on_queued_paths_timer(gpointer gself)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;
  GHashTableIter iter;
  gpointer path;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER (gself);
  p = get_priv(self);

  /* create new devices for all the queued paths */
  g_hash_table_iter_init (&iter, p->queued_paths);
  while (g_hash_table_iter_next (&iter, &path, NULL))
    update_device_from_object_path (self, path);

  /* cleanup */
  g_hash_table_remove_all (p->queued_paths);
  p->queued_paths_timer = 0;
  return G_SOURCE_REMOVE;
}

/* add the path to our queued_paths hashset and ensure the timer's running */
static void
refresh_device_soon (IndicatorPowerDeviceProviderUPower * self,
                     const char                         * object_path)
{
  // Android: Ignore batt_therm devices since they give wrong values
  if (g_str_has_suffix(object_path, "batt_therm"))
    return;
  priv_t * p = get_priv(self);

  g_hash_table_add (p->queued_paths, g_strdup (object_path));

  if (p->queued_paths_timer == 0)
    p->queued_paths_timer = g_timeout_add (500, on_queued_paths_timer, self);
}

/***
****
***/

static void
on_enumerate_devices_response(GObject       * bus,
                              GAsyncResult  * res,
                              gpointer        gself)
{
  GError* error;
  GVariant* v;

  error = NULL;
  v = g_dbus_connection_call_finish(G_DBUS_CONNECTION(bus), res, &error);
  if (v == NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("Unable to enumerate UPower devices: %s", error->message);
      g_error_free (error);
    }
  else if (g_variant_is_of_type(v, G_VARIANT_TYPE("(ao)")))
    {
      GVariant * ao;
      GVariantIter iter;
      const gchar * path;

      ao = g_variant_get_child_value(v, 0);
      g_variant_iter_init(&iter, ao);
      path = NULL;
      while(g_variant_iter_loop(&iter, "o", &path)) {
        // Android: Ignore batt_therm devices since they give wrong values
        if (!g_str_has_suffix(path, "batt_therm"))
          refresh_device_soon (gself, path);
      }

      g_variant_unref(ao);
    }

  g_clear_pointer(&v, g_variant_unref);
}

static void
on_device_properties_changed(GDBusConnection * connection     G_GNUC_UNUSED,
                             const gchar     * sender_name    G_GNUC_UNUSED,
                             const gchar     * object_path,
                             const gchar     * interface_name G_GNUC_UNUSED,
                             const gchar     * signal_name    G_GNUC_UNUSED,
                             GVariant        * parameters,
                             gpointer          gself)
{
  // Android: Ignore batt_therm devices since they give wrong values
  if (g_str_has_suffix(object_path, "batt_therm"))
    return;
  IndicatorPowerDeviceProviderUPower* self;
  priv_t* p;
  IndicatorPowerDevice* device;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(gself);
  p = get_priv(self);

  device = g_hash_table_lookup(p->devices, object_path);
  if (device == NULL) /* unlikely, but let's handle it */
    {
      refresh_device_soon (self, object_path);
    }
  else if ((parameters != NULL) && g_variant_n_children(parameters)>=2)
    {
      gboolean changed = FALSE;
      GVariant* dict;
      GVariantIter iter;
      gchar* key;
      GVariant* value;

      dict = g_variant_get_child_value(parameters, 1);
      g_variant_iter_init(&iter, dict);
      while (g_variant_iter_next(&iter, "{sv}", &key, &value))
        {
          if (!g_strcmp0(key, "TimeToFull") || !g_strcmp0(key, "TimeToEmpty"))
            {
              const gint64 i = g_variant_get_int64(value);
              if (i != 0)
                {
                  g_object_set(device,
                               INDICATOR_POWER_DEVICE_TIME, (guint64)i,
                               NULL);
                  changed = TRUE;
                }
            }
          else if (!g_strcmp0(key, "Percentage"))
            {
              const gdouble d = g_variant_get_double(value);
              g_object_set(device,
                           INDICATOR_POWER_DEVICE_PERCENTAGE, d,
                           NULL);
              changed = TRUE;
            }
          else if (!g_strcmp0(key, "Type"))
            {
              const guint32 u = g_variant_get_uint32(value);
              g_object_set(device,
                           INDICATOR_POWER_DEVICE_KIND, (gint)u,
                           NULL);
              changed = TRUE;
            }
          else if (!g_strcmp0(key, "Model"))
            {
              const gchar *s = g_variant_get_string(value, NULL);
              g_object_set(device,
                           INDICATOR_POWER_DEVICE_MODEL, s,
                           NULL);
              changed = TRUE;
            }
          else if (!g_strcmp0(key, "State"))
            {
              const guint32 u = g_variant_get_uint32(value);
              g_object_set(device,
                           INDICATOR_POWER_DEVICE_STATE, (gint)u,
                           NULL);
              changed = TRUE;
            }
        }
      g_variant_unref(dict);

      if (changed)
        emit_devices_changed(self);
    }
}

static const gchar*
get_path_from_nth_child(GVariant* parameters, gsize i)
{
  const gchar* path = NULL;

  if ((parameters != NULL) && g_variant_n_children(parameters)>i)
    {
      GVariant* v = g_variant_get_child_value(parameters, i);
      if (g_variant_is_of_type(v, G_VARIANT_TYPE_STRING) || /* UPower < 0.99 */
          g_variant_is_of_type(v, G_VARIANT_TYPE_OBJECT_PATH)) /* and >= 0.99 */
        {
          path = g_variant_get_string(v, NULL);
        }
      g_variant_unref(v);
    }

  return path;
}

static void
on_upower_signal(GDBusConnection * connection     G_GNUC_UNUSED,
                 const gchar     * sender_name    G_GNUC_UNUSED,
                 const gchar     * object_path    G_GNUC_UNUSED,
                 const gchar     * interface_name G_GNUC_UNUSED,
                 const gchar     * signal_name,
                 GVariant        * parameters,
                 gpointer          gself)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(gself);
  p = get_priv(self);

  if (!g_strcmp0(signal_name, "DeviceAdded"))
    {
      refresh_device_soon (self, get_path_from_nth_child(parameters, 0));
    }
  else if (!g_strcmp0(signal_name, "DeviceRemoved"))
    {
      const char* device_path = get_path_from_nth_child(parameters, 0);
      g_hash_table_remove(p->devices, device_path);
      g_hash_table_remove(p->queued_paths, device_path);
      emit_devices_changed(self);
    }
  else if (!g_strcmp0(signal_name, "DeviceChanged")) /* UPower < 0.99 */
    {
      refresh_device_soon (self, get_path_from_nth_child(parameters, 0));
    }
  else if (!g_strcmp0(signal_name, "Resuming")) /* UPower < 0.99 */
    {
      GHashTableIter iter;
      gpointer device_path = NULL;
      g_debug("Resumed from hibernate/sleep; queueing all devices for a refresh");
      g_hash_table_iter_init (&iter, p->devices);
      while (g_hash_table_iter_next (&iter, &device_path, NULL))
        refresh_device_soon (self, device_path);
    }
}

/* start listening for UPower events on the bus */
static void
on_bus_name_appeared(GDBusConnection * bus,
                     const gchar     * name         G_GNUC_UNUSED,
                     const gchar     * name_owner,
                     gpointer          gself)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;
  guint tag;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(gself);
  p = get_priv(self);
  p->bus = G_DBUS_CONNECTION(g_object_ref(bus));

  /* listen for signals from the boss */
  tag = g_dbus_connection_signal_subscribe(p->bus,
                                           name_owner,
                                           MGR_IFACE,
                                           NULL /*signal_name*/,
                                           MGR_PATH,
                                           NULL /*arg0*/,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           on_upower_signal,
                                           self,
                                           NULL);
  p->subscriptions = g_slist_prepend(p->subscriptions, GUINT_TO_POINTER(tag));

  /* listen for change events from the devices */
  tag = g_dbus_connection_signal_subscribe(p->bus,
                                           name_owner,
                                           "org.freedesktop.DBus.Properties",
                                           "PropertiesChanged",
                                           NULL /*object_path*/,
                                           "org.freedesktop.UPower.Device", /*arg0*/
                                           G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE,
                                           on_device_properties_changed,
                                           self,
                                           NULL);
  p->subscriptions = g_slist_prepend(p->subscriptions, GUINT_TO_POINTER(tag));

  /* rebuild our devices list */
  g_dbus_connection_call(p->bus,
                         BUS_NAME,
                         MGR_PATH,
                         MGR_IFACE,
                         "EnumerateDevices",
                         NULL,
                         G_VARIANT_TYPE("(ao)"),
                         G_DBUS_CALL_FLAGS_NO_AUTO_START,
                         -1, /* default timeout */
                         p->cancellable,
                         on_enumerate_devices_response,
                         self);
}

static void
on_bus_name_vanished(GDBusConnection * connection G_GNUC_UNUSED,
                     const gchar     * name       G_GNUC_UNUSED,
                     gpointer          gself)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;
  GSList * l;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(gself);
  p = get_priv(self);

  /* clear the devices */
  g_hash_table_remove_all(p->devices);
  g_hash_table_remove_all(p->queued_paths);
  if (p->queued_paths_timer != 0)
    {
      g_source_remove(p->queued_paths_timer);
      p->queued_paths_timer = 0;
    }
  emit_devices_changed (self);

  /* clear the bus subscriptions */
  for (l=p->subscriptions; l!=NULL; l=l->next)
    g_dbus_connection_signal_unsubscribe(p->bus, GPOINTER_TO_UINT(l->data));
  g_slist_free(p->subscriptions);
  p->subscriptions = NULL;

  /* clear the bus */
  g_clear_object(&p->bus);
}

/***
****  IndicatorPowerDeviceProvider virtual functions
***/

static GList *
my_get_devices(IndicatorPowerDeviceProvider * provider)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;
  GList * devices;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(provider);
  p = get_priv(self);

  devices = g_hash_table_get_values (p->devices);
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
  p = get_priv(self);

  if (p->cancellable != NULL)
    {
      g_cancellable_cancel (p->cancellable);

      g_clear_object (&p->cancellable);
    }

  if (p->queued_paths_timer != 0)
    {
      g_source_remove (p->queued_paths_timer);

      p->queued_paths_timer = 0;
    }

  if (p->name_tag != 0)
    {
      g_bus_unwatch_name(p->name_tag);
      on_bus_name_vanished(NULL, NULL, self);

      p->name_tag = 0;
    }

  G_OBJECT_CLASS (indicator_power_device_provider_upower_parent_class)->dispose(o);
}

static void
my_finalize (GObject * o)
{
  IndicatorPowerDeviceProviderUPower * self;
  priv_t * p;

  self = INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(o);
  p = get_priv(self);

  g_hash_table_destroy (p->devices);
  g_hash_table_destroy (p->queued_paths);

  G_OBJECT_CLASS (indicator_power_device_provider_upower_parent_class)->finalize (o);
}

/***
****  Instantiation
***/

static void
indicator_power_device_provider_upower_class_init (IndicatorPowerDeviceProviderUPowerClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = my_dispose;
  object_class->finalize = my_finalize;
}

static void
indicator_power_device_provider_interface_init (IndicatorPowerDeviceProviderInterface * iface)
{
  iface->get_devices = my_get_devices;
}

static void
indicator_power_device_provider_upower_init (IndicatorPowerDeviceProviderUPower * self)
{
  priv_t * p = get_priv(self);

  p->cancellable = g_cancellable_new();

  p->devices = g_hash_table_new_full(g_str_hash,
                                     g_str_equal,
                                     g_free,
                                     g_object_unref);

  p->queued_paths = g_hash_table_new_full(g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          NULL);

  p->name_tag = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
                                 BUS_NAME,
                                 G_BUS_NAME_WATCHER_FLAGS_NONE,
                                 on_bus_name_appeared,
                                 on_bus_name_vanished,
                                 self,
                                 NULL);
}

/***
****  Public API
***/

IndicatorPowerDeviceProvider *
indicator_power_device_provider_upower_new(void)
{
  gpointer o = g_object_new (INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER, NULL);

  return INDICATOR_POWER_DEVICE_PROVIDER (o);
}
