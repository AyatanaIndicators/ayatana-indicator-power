/*
 * Copyright © 2026 UBports Foundation.
 * Copyright © 2026 Volla Systeme GmbH.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 */

#include "power-saver.h"

#include <gio/gio.h>

#define BUS_NAME       "com.lomiri.Repowerd.PowerSaver"
#define OBJECT_PATH    "/com/lomiri/Repowerd/PowerSaver"
#define IFACE          "com.lomiri.Repowerd.PowerSaver"

struct _IndicatorPowerSaverPriv
{
  GDBusProxy   * proxy;
  GCancellable * cancellable;

  guint    watch_id;
  gboolean service_available;
  gboolean available;
  gchar  * device_name;
  GStrv    profiles;
  gchar  * active_profile;
  gboolean auto_on_battery;
};

enum
{
  SIGNAL_CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

/* G_DEFINE_TYPE_WITH_PRIVATE() expands to <TypeName>Private. */
typedef struct _IndicatorPowerSaverPriv IndicatorPowerSaverPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IndicatorPowerSaver, indicator_power_saver, G_TYPE_OBJECT)

static void emit_changed(IndicatorPowerSaver * self)
{
  g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
}

static gboolean strv_equal(GStrv a, GStrv b)
{
  guint la = a ? g_strv_length(a) : 0;
  guint lb = b ? g_strv_length(b) : 0;
  if (la != lb) return FALSE;
  for (guint i = 0; i < la; ++i)
    if (g_strcmp0(a[i], b[i]) != 0) return FALSE;
  return TRUE;
}

static void refresh_available_done(GObject * source, GAsyncResult * res, gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GError * err = NULL;
  GVariant * v = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &err);
  if (!v) { g_clear_error(&err); g_object_unref(self); return; }
  gboolean a = FALSE;
  g_variant_get(v, "(b)", &a);
  if (a != self->priv->available)
    { self->priv->available = a; emit_changed(self); }
  g_variant_unref(v);
  g_object_unref(self);
}

static void refresh_device_done(GObject * source, GAsyncResult * res, gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GError * err = NULL;
  GVariant * v = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &err);
  if (!v) { g_clear_error(&err); g_object_unref(self); return; }
  const gchar * s = NULL;
  g_variant_get(v, "(&s)", &s);
  if (g_strcmp0(s, self->priv->device_name) != 0)
    {
      g_free(self->priv->device_name);
      self->priv->device_name = g_strdup(s ? s : "");
      emit_changed(self);
    }
  g_variant_unref(v);
  g_object_unref(self);
}

static void refresh_profiles_done(GObject * source, GAsyncResult * res, gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GError * err = NULL;
  GVariant * v = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &err);
  if (!v) { g_clear_error(&err); g_object_unref(self); return; }

  GVariant * inner = g_variant_get_child_value(v, 0);
  GVariantIter it;
  g_variant_iter_init(&it, inner);
  GPtrArray * arr = g_ptr_array_new();
  gchar * s;
  while (g_variant_iter_next(&it, "&s", &s))
    g_ptr_array_add(arr, g_strdup(s));
  g_ptr_array_add(arr, NULL);
  GStrv next = (GStrv) g_ptr_array_free(arr, FALSE);

  if (!strv_equal(self->priv->profiles, next))
    {
      g_strfreev(self->priv->profiles);
      self->priv->profiles = next;
      emit_changed(self);
    }
  else
    {
      g_strfreev(next);
    }
  g_variant_unref(inner);
  g_variant_unref(v);
  g_object_unref(self);
}

static void refresh_active_done(GObject * source, GAsyncResult * res, gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GError * err = NULL;
  GVariant * v = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &err);
  if (!v) { g_clear_error(&err); g_object_unref(self); return; }
  const gchar * s = NULL;
  g_variant_get(v, "(&s)", &s);
  if (g_strcmp0(s, self->priv->active_profile) != 0)
    {
      g_free(self->priv->active_profile);
      self->priv->active_profile = g_strdup(s ? s : "");
      emit_changed(self);
    }
  g_variant_unref(v);
  g_object_unref(self);
}

static void refresh_auto_done(GObject * source, GAsyncResult * res, gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GError * err = NULL;
  GVariant * v = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &err);
  if (!v) { g_clear_error(&err); g_object_unref(self); return; }
  gboolean b = FALSE;
  g_variant_get(v, "(b)", &b);
  if (b != self->priv->auto_on_battery)
    { self->priv->auto_on_battery = b; emit_changed(self); }
  g_variant_unref(v);
  g_object_unref(self);
}

static void refresh_all(IndicatorPowerSaver * self)
{
  if (!self->priv->proxy) return;
  g_dbus_proxy_call(self->priv->proxy, "IsAvailable", NULL,
                    G_DBUS_CALL_FLAGS_NONE, -1, self->priv->cancellable,
                    refresh_available_done, g_object_ref(self));
  g_dbus_proxy_call(self->priv->proxy, "GetDeviceName", NULL,
                    G_DBUS_CALL_FLAGS_NONE, -1, self->priv->cancellable,
                    refresh_device_done, g_object_ref(self));
  g_dbus_proxy_call(self->priv->proxy, "ListProfiles", NULL,
                    G_DBUS_CALL_FLAGS_NONE, -1, self->priv->cancellable,
                    refresh_profiles_done, g_object_ref(self));
  g_dbus_proxy_call(self->priv->proxy, "GetActiveProfile", NULL,
                    G_DBUS_CALL_FLAGS_NONE, -1, self->priv->cancellable,
                    refresh_active_done, g_object_ref(self));
  g_dbus_proxy_call(self->priv->proxy, "GetAutoOnBattery", NULL,
                    G_DBUS_CALL_FLAGS_NONE, -1, self->priv->cancellable,
                    refresh_auto_done, g_object_ref(self));
}

static void on_proxy_signal(GDBusProxy * proxy G_GNUC_UNUSED,
                            const gchar * sender G_GNUC_UNUSED,
                            const gchar * name,
                            GVariant * params,
                            gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  if (g_strcmp0(name, "ActiveProfileChanged") == 0)
    {
      const gchar * s = NULL;
      g_variant_get(params, "(&s)", &s);
      if (g_strcmp0(s, self->priv->active_profile) != 0)
        {
          g_free(self->priv->active_profile);
          self->priv->active_profile = g_strdup(s ? s : "");
          emit_changed(self);
        }
    }
  else if (g_strcmp0(name, "AutoOnBatteryChanged") == 0)
    {
      gboolean b = FALSE;
      g_variant_get(params, "(b)", &b);
      if (b != self->priv->auto_on_battery)
        { self->priv->auto_on_battery = b; emit_changed(self); }
    }
}

static void on_proxy_ready(GObject * source G_GNUC_UNUSED,
                           GAsyncResult * res,
                           gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GError * err = NULL;
  GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &err);
  if (!proxy)
    {
      if (err && !g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("PowerSaver: proxy create failed: %s", err->message);
      g_clear_error(&err);
      g_object_unref(self);
      return;
    }

  self->priv->proxy = proxy;
  g_signal_connect(proxy, "g-signal", G_CALLBACK(on_proxy_signal), self);
  refresh_all(self);
  g_object_unref(self);
}

static void on_name_appeared(GDBusConnection * conn G_GNUC_UNUSED,
                             const gchar * name G_GNUC_UNUSED,
                             const gchar * owner G_GNUC_UNUSED,
                             gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  if (!self->priv->service_available)
    { self->priv->service_available = TRUE; emit_changed(self); }
  if (!self->priv->proxy)
    {
      g_dbus_proxy_new_for_bus(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
        NULL,
        BUS_NAME, OBJECT_PATH, IFACE,
        self->priv->cancellable,
        on_proxy_ready,
        g_object_ref(self));
    }
  else
    {
      refresh_all(self);
    }
}

static void on_name_vanished(GDBusConnection * conn G_GNUC_UNUSED,
                             const gchar * name G_GNUC_UNUSED,
                             gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  g_clear_object(&self->priv->proxy);
  if (self->priv->service_available)
    { self->priv->service_available = FALSE; emit_changed(self); }
}

gboolean indicator_power_saver_service_available(IndicatorPowerSaver * self)
{ return self->priv->service_available; }

gboolean indicator_power_saver_is_available(IndicatorPowerSaver * self)
{ return self->priv->available; }

const gchar * indicator_power_saver_device_name(IndicatorPowerSaver * self)
{ return self->priv->device_name ? self->priv->device_name : ""; }

GStrv indicator_power_saver_profiles(IndicatorPowerSaver * self)
{ return self->priv->profiles; }

const gchar * indicator_power_saver_active_profile(IndicatorPowerSaver * self)
{ return self->priv->active_profile ? self->priv->active_profile : ""; }

gboolean indicator_power_saver_get_auto_on_battery(IndicatorPowerSaver * self)
{ return self->priv->auto_on_battery; }

void indicator_power_saver_set_active_profile(IndicatorPowerSaver * self,
                                              const gchar * profile)
{
  if (!self->priv->proxy) return;
  g_dbus_proxy_call(self->priv->proxy, "SetActiveProfile",
                    g_variant_new("(s)", profile ? profile : ""),
                    G_DBUS_CALL_FLAGS_NONE, -1,
                    self->priv->cancellable, NULL, NULL);
}

void indicator_power_saver_set_auto_on_battery(IndicatorPowerSaver * self,
                                               gboolean enabled)
{
  if (!self->priv->proxy) return;
  g_dbus_proxy_call(self->priv->proxy, "SetAutoOnBattery",
                    g_variant_new("(b)", enabled),
                    G_DBUS_CALL_FLAGS_NONE, -1,
                    self->priv->cancellable, NULL, NULL);
}

static void indicator_power_saver_init(IndicatorPowerSaver * self)
{
  self->priv = indicator_power_saver_get_instance_private(self);
  self->priv->cancellable = g_cancellable_new();
  self->priv->device_name = g_strdup("");
  self->priv->active_profile = g_strdup("");

  self->priv->watch_id = g_bus_watch_name(
    G_BUS_TYPE_SYSTEM,
    BUS_NAME,
    G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
    on_name_appeared,
    on_name_vanished,
    self,
    NULL);
}

static void indicator_power_saver_dispose(GObject * obj)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(obj);
  if (self->priv->cancellable)
    g_cancellable_cancel(self->priv->cancellable);
  if (self->priv->watch_id)
    { g_bus_unwatch_name(self->priv->watch_id); self->priv->watch_id = 0; }
  g_clear_object(&self->priv->proxy);
  g_clear_object(&self->priv->cancellable);
  G_OBJECT_CLASS(indicator_power_saver_parent_class)->dispose(obj);
}

static void indicator_power_saver_finalize(GObject * obj)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(obj);
  g_clear_pointer(&self->priv->device_name, g_free);
  g_clear_pointer(&self->priv->active_profile, g_free);
  g_clear_pointer(&self->priv->profiles, g_strfreev);
  G_OBJECT_CLASS(indicator_power_saver_parent_class)->finalize(obj);
}

static void indicator_power_saver_class_init(IndicatorPowerSaverClass * klass)
{
  GObjectClass * oc = G_OBJECT_CLASS(klass);
  oc->dispose = indicator_power_saver_dispose;
  oc->finalize = indicator_power_saver_finalize;

  signals[SIGNAL_CHANGED] =
    g_signal_new("changed",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0, NULL, NULL, NULL,
                 G_TYPE_NONE, 0);
}

IndicatorPowerSaver * indicator_power_saver_new(void)
{
  return g_object_new(INDICATOR_TYPE_POWER_SAVER, NULL);
}
