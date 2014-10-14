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

#include "dbus-shared.h"
#include "device-provider-mock.h"
#include "device-provider-upower.h"
#include "dbus-testing.h"
#include "service.h"
#include "testing.h"

#include <glib-object.h>

/**
***  GObject Properties
**/

enum
{
  PROP_0,
  PROP_BUS,
  PROP_SERVICE,
  LAST_PROP
};

static GParamSpec * properties[LAST_PROP];

/**
***
**/

typedef struct
{
  GDBusConnection * bus;
  DbusTesting * skeleton;
  IndicatorPowerService * service;
  IndicatorPowerDevice * battery_mock;
  gpointer provider_mock;
  gpointer provider_upower;
}
IndicatorPowerTestingPrivate;

typedef IndicatorPowerTestingPrivate priv_t;

G_DEFINE_TYPE_WITH_PRIVATE(IndicatorPowerTesting,
                           indicator_power_testing,
                           G_TYPE_OBJECT)

#define get_priv(o) ((priv_t*)indicator_power_testing_get_instance_private(o))

/***
****
***/

static void
update_device_provider (IndicatorPowerTesting * self)
{
  priv_t * const p = get_priv(self);
  IndicatorPowerDeviceProvider * device_provider;

  device_provider = dbus_testing_get_mock_battery_enabled(p->skeleton)
                  ? p->provider_mock
                  : p->provider_upower;
  indicator_power_service_set_device_provider(p->service, device_provider);
}

static void
on_mock_battery_enabled_changed(DbusTesting           * skeleton  G_GNUC_UNUSED,
                                GParamSpec            * pspec     G_GNUC_UNUSED,
                                IndicatorPowerTesting * self)
{
  update_device_provider (self);
}

static void
on_mock_battery_level_changed(DbusTesting           * skeleton,
                              GParamSpec            * pspec     G_GNUC_UNUSED,
                              IndicatorPowerTesting * self)
{
  g_object_set(get_priv(self)->battery_mock,
               INDICATOR_POWER_DEVICE_PERCENTAGE, (gdouble)dbus_testing_get_mock_battery_level(skeleton),
               NULL);
}

static void
on_mock_battery_state_changed(DbusTesting           * skeleton,
                              GParamSpec            * pspec    G_GNUC_UNUSED,
                              IndicatorPowerTesting * self)
{
  const gchar* state_str = dbus_testing_get_mock_battery_state(skeleton);
  UpDeviceState state;

  if (!g_strcmp0(state_str, "charging"))
    {
      state = UP_DEVICE_STATE_CHARGING;
    }
  else if (!g_strcmp0(state_str, "discharging"))
    {
      state = UP_DEVICE_STATE_DISCHARGING;
    }
  else
    {
      g_warning("%s unsupported state: '%s'", G_STRLOC, state_str);
      state = UP_DEVICE_STATE_UNKNOWN;
    }

  g_object_set(get_priv(self)->battery_mock,
               INDICATOR_POWER_DEVICE_STATE, (gint)state,
               NULL);
}

static void
on_mock_battery_minutes_left_changed(DbusTesting           * skeleton,
                                     GParamSpec            * pspec    G_GNUC_UNUSED,
                                     IndicatorPowerTesting * self)
{
  g_object_set(get_priv(self)->battery_mock,
               INDICATOR_POWER_DEVICE_TIME, (guint64)dbus_testing_get_mock_battery_minutes_left(skeleton),
               NULL);
}

static void
on_bus_changed(IndicatorPowerService * service,
               GParamSpec            * spec     G_GNUC_UNUSED,
               IndicatorPowerTesting * self)
{
  GObject * bus = NULL;
  g_object_get(service, "bus", &bus, NULL);  
  indicator_power_testing_set_bus(self, G_DBUS_CONNECTION(bus));
  g_clear_object(&bus);
}

/***
****  GObject virtual functions
***/

static void
my_get_property(GObject     * o,
                guint         property_id,
                GValue      * value,
                GParamSpec  * pspec)
{
  IndicatorPowerTesting * const self = INDICATOR_POWER_TESTING (o);
  priv_t * const p = get_priv(self);

  switch (property_id)
    {
      case PROP_BUS:
        g_value_set_object(value, p->bus);
        break;

      case PROP_SERVICE:
        g_value_set_object(value, p->service);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(o, property_id, pspec);
    }
}

static void
my_set_property(GObject       * o,
                guint           property_id,
                const GValue  * value,
                GParamSpec    * pspec)
{
  IndicatorPowerTesting * const self = INDICATOR_POWER_TESTING (o);
  priv_t * const p = get_priv(self);

  switch (property_id)
    {
      case PROP_BUS:
        indicator_power_testing_set_bus(self, G_DBUS_CONNECTION(g_value_get_object(value)));
        break;

      case PROP_SERVICE:
        g_assert(p->service == NULL); /* G_PARAM_CONSTRUCT_ONLY */
        p->service = g_value_dup_object(value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(o, property_id, pspec);
    }
}

static void
my_dispose(GObject * o)
{
  IndicatorPowerTesting * const self = INDICATOR_POWER_TESTING(o);
  priv_t * const p = get_priv (self);

  indicator_power_testing_set_bus(self, NULL);
  g_clear_object(&p->skeleton);
  g_clear_object(&p->provider_upower);
  g_clear_object(&p->provider_mock);
  g_clear_object(&p->battery_mock);
  g_clear_object(&p->service);

  G_OBJECT_CLASS (indicator_power_testing_parent_class)->dispose(o);
}

static void
my_finalize (GObject * o G_GNUC_UNUSED)
{
}

static void
my_constructed (GObject * o)
{
  IndicatorPowerTesting * const self = INDICATOR_POWER_TESTING(o);
  priv_t * const p = get_priv (self);

  g_assert(p->service != NULL); /* G_PARAM_CONSTRUCT_ONLY */
  g_signal_connect(p->service, "notify::bus", G_CALLBACK(on_bus_changed), o);
  on_bus_changed(p->service, NULL, self);
  update_device_provider(self);
}


/***
****  Instantiation
***/

static void
indicator_power_testing_init (IndicatorPowerTesting * self)
{
  priv_t * const p = get_priv (self);

  /* DBus Skeleton */

  p->skeleton = dbus_testing_skeleton_new();
  dbus_testing_set_mock_battery_level(p->skeleton, 50u);
  dbus_testing_set_mock_battery_state(p->skeleton, "discharging");
  dbus_testing_set_mock_battery_enabled(p->skeleton, FALSE);
  dbus_testing_set_mock_battery_minutes_left(p->skeleton, 30);

  g_signal_connect(p->skeleton, "notify::mock-battery-enabled",
                   G_CALLBACK(on_mock_battery_enabled_changed), self);
  g_signal_connect(p->skeleton, "notify::mock-battery-level",
                   G_CALLBACK(on_mock_battery_level_changed), self);
  g_signal_connect(p->skeleton, "notify::mock-battery-state",
                   G_CALLBACK(on_mock_battery_state_changed), self);
  g_signal_connect(p->skeleton, "notify::mock-battery-minutes-left",
                   G_CALLBACK(on_mock_battery_minutes_left_changed), self);

  /* Mock Battery */
  
  p->battery_mock = indicator_power_device_new("/some/path",
                                               UP_DEVICE_KIND_BATTERY,
                                               50.0,
                                               UP_DEVICE_STATE_DISCHARGING,
                                               60*30);


  /* Mock Provider */

  p->provider_mock = indicator_power_device_provider_mock_new();

  indicator_power_device_provider_add_device(INDICATOR_POWER_DEVICE_PROVIDER_MOCK(p->provider_mock),
                                             p->battery_mock);

  /* UPower Provider */

  p->provider_upower = indicator_power_device_provider_upower_new();
}

static void
indicator_power_testing_class_init (IndicatorPowerTestingClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = my_dispose;
  object_class->finalize = my_finalize;
  object_class->constructed = my_constructed;
  object_class->get_property = my_get_property;
  object_class->set_property = my_set_property;

  properties[PROP_BUS] = g_param_spec_object (
    "bus",
    "Bus",
    "The GDBusConnection on which to export menus",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_SERVICE] = g_param_spec_object (
    "service",
    "Servie",
    "The IndicatorPower Service",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

/***
****  Public API
***/

IndicatorPowerTesting *
indicator_power_testing_new (IndicatorPowerService * service)
{
  GObject * o = g_object_new (INDICATOR_TYPE_POWER_TESTING, "service", service, NULL);

  return INDICATOR_POWER_TESTING (o);
}

void
indicator_power_testing_set_bus(IndicatorPowerTesting * self,
                                GDBusConnection       * bus)
{
  priv_t * p;
  GDBusInterfaceSkeleton * skel;

  g_return_if_fail(INDICATOR_IS_POWER_TESTING(self));
  g_return_if_fail((bus == NULL) || G_IS_DBUS_CONNECTION(bus));

  p = get_priv (self);

  if (p->bus == bus)
    return;

  skel = G_DBUS_INTERFACE_SKELETON(p->skeleton);

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
                                            BUS_PATH"/Testing",
                                            &error))
        {
          g_warning ("Unable to export Testing properties: %s", error->message);
          g_error_free (error);
        }
    }
}

