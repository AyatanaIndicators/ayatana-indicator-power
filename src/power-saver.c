/*
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
  gboolean battery_saver;
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

static void set_battery_saver(IndicatorPowerSaver * self, gboolean enabled)
{
  if (enabled == self->priv->battery_saver) return;
  self->priv->battery_saver = enabled;
  emit_changed(self);
}

static void refresh_battery_saver_done(GObject * source, GAsyncResult * res, gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  GVariant * v = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, NULL);
  if (v)
    {
      gboolean b = FALSE;
      g_variant_get(v, "(b)", &b);
      set_battery_saver(self, b);
      g_variant_unref(v);
    }
  g_object_unref(self);
}

static void refresh(IndicatorPowerSaver * self)
{
  g_dbus_proxy_call(self->priv->proxy, "GetBatterySaver", NULL,
                    G_DBUS_CALL_FLAGS_NONE, -1, self->priv->cancellable,
                    refresh_battery_saver_done, g_object_ref(self));
}

static void on_proxy_signal(GDBusProxy * proxy G_GNUC_UNUSED,
                            const gchar * sender G_GNUC_UNUSED,
                            const gchar * name,
                            GVariant * params,
                            gpointer ud)
{
  if (g_strcmp0(name, "BatterySaverChanged") == 0)
    {
      gboolean b = FALSE;
      g_variant_get(params, "(b)", &b);
      set_battery_saver(INDICATOR_POWER_SAVER(ud), b);
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
  refresh(self);
  g_object_unref(self);
}

static void on_name_appeared(GDBusConnection * conn G_GNUC_UNUSED,
                             const gchar * name G_GNUC_UNUSED,
                             const gchar * owner G_GNUC_UNUSED,
                             gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  if (!self->priv->service_available)
    {
      self->priv->service_available = TRUE;
      emit_changed(self);
    }
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
      refresh(self);
    }
}

static void on_name_vanished(GDBusConnection * conn G_GNUC_UNUSED,
                             const gchar * name G_GNUC_UNUSED,
                             gpointer ud)
{
  IndicatorPowerSaver * self = INDICATOR_POWER_SAVER(ud);
  g_clear_object(&self->priv->proxy);
  if (self->priv->service_available)
    {
      self->priv->service_available = FALSE;
      emit_changed(self);
    }
}

gboolean indicator_power_saver_service_available(IndicatorPowerSaver * self)
{ return self->priv->service_available; }

gboolean indicator_power_saver_get_battery_saver(IndicatorPowerSaver * self)
{ return self->priv->battery_saver; }

void indicator_power_saver_set_battery_saver(IndicatorPowerSaver * self,
                                             gboolean enabled)
{
  if (!self->priv->proxy) return;
  g_dbus_proxy_call(self->priv->proxy, "SetBatterySaver",
                    g_variant_new("(b)", enabled),
                    G_DBUS_CALL_FLAGS_NONE, -1,
                    self->priv->cancellable, NULL, NULL);
}

static void indicator_power_saver_init(IndicatorPowerSaver * self)
{
  self->priv = indicator_power_saver_get_instance_private(self);
  self->priv->cancellable = g_cancellable_new();

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

static void indicator_power_saver_class_init(IndicatorPowerSaverClass * klass)
{
  GObjectClass * oc = G_OBJECT_CLASS(klass);
  oc->dispose = indicator_power_saver_dispose;

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
