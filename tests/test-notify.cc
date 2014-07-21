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


#include "glib-fixture.h"

#include "dbus-shared.h"
#include "device.h"
#include "notifier.h"

#include <gtest/gtest.h>

#include <libdbustest/dbus-test.h>

#include <libnotify/notify.h>

#include <glib.h>
#include <gio/gio.h>

/***
****
***/

class NotifyFixture: public GlibFixture
{
private:

  typedef GlibFixture super;

  static constexpr char const * NOTIFY_BUSNAME   {"org.freedesktop.Notifications"};
  static constexpr char const * NOTIFY_INTERFACE {"org.freedesktop.Notifications"};
  static constexpr char const * NOTIFY_PATH      {"/org/freedesktop/Notifications"};

  DbusTestService * service = nullptr;
  DbusTestDbusMock * mock = nullptr;
  DbusTestDbusMockObject * obj = nullptr;

protected:

  GDBusConnection * bus = nullptr;

  static constexpr int NOTIFY_ID {1234};

  static constexpr int NOTIFICATION_CLOSED_EXPIRED   {1};
  static constexpr int NOTIFICATION_CLOSED_DISMISSED {2};
  static constexpr int NOTIFICATION_CLOSED_API       {3};
  static constexpr int NOTIFICATION_CLOSED_UNDEFINED {4};

  static constexpr char const * APP_NAME {"indicator-power-service"};

  static constexpr char const * METHOD_NOTIFY {"Notify"};
  static constexpr char const * METHOD_GET_CAPS {"GetCapabilities"};
  static constexpr char const * METHOD_GET_INFO {"GetServerInformation"};

  static constexpr char const * HINT_TIMEOUT {"x-canonical-snap-decisions-timeout"};

protected:

  void SetUp()
  {
    super::SetUp();

    // init DBusMock / dbus-test-runner

    service = dbus_test_service_new(nullptr);

    GError * error = nullptr;
    mock = dbus_test_dbus_mock_new(NOTIFY_BUSNAME);
    obj = dbus_test_dbus_mock_get_object(mock, NOTIFY_PATH, NOTIFY_INTERFACE, &error);
    g_assert_no_error (error);
    
    dbus_test_dbus_mock_object_add_method(mock, obj, METHOD_GET_INFO,
                                          nullptr,
                                          G_VARIANT_TYPE("(ssss)"),
                                          "ret = ('mock-notify', 'test vendor', '1.0', '1.1')", // python
                                          &error);
    g_assert_no_error (error);

    auto python_str = g_strdup_printf ("ret = %d", NOTIFY_ID);
    dbus_test_dbus_mock_object_add_method(mock, obj, METHOD_NOTIFY,
                                          G_VARIANT_TYPE("(susssasa{sv}i)"),
                                          G_VARIANT_TYPE_UINT32,
                                          python_str,
                                          &error);
    g_free (python_str);
    g_assert_no_error (error);

    dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
    dbus_test_service_start_tasks(service);

    bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    g_dbus_connection_set_exit_on_close(bus, FALSE);
    g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);

    notify_init(APP_NAME);
  }

  virtual void TearDown()
  {
    notify_uninit();

    g_clear_object(&mock);
    g_clear_object(&service);
    g_object_unref(bus);

    // wait a little while for the scaffolding to shut down,
    // but don't block on it forever...
    unsigned int cleartry = 0;
    while ((bus != nullptr) && (cleartry < 50))
      {
        g_usleep(100000);
        while (g_main_pending())
          g_main_iteration(true);
        cleartry++;
      }

    super::TearDown();
  }
};

/***
****
***/

// simple test to confirm the NotifyFixture plumbing all works
TEST_F(NotifyFixture, HelloWorld)
{
}


// scaffolding to listen for PropertyChanged signals and remember them
namespace
{
  enum
  {
    FIELD_POWER_LEVEL = (1<<0),
    FIELD_IS_WARNING  = (1<<1)
  };

  struct ChangedParams
  {
    int32_t power_level = POWER_LEVEL_OK;
    bool is_warning = false;
    uint32_t fields = 0;
  };

  void on_battery_property_changed (GDBusConnection *connection G_GNUC_UNUSED,
                                    const gchar *sender_name G_GNUC_UNUSED,
                                    const gchar *object_path G_GNUC_UNUSED,
                                    const gchar *interface_name G_GNUC_UNUSED,
                                    const gchar *signal_name G_GNUC_UNUSED,
                                    GVariant *parameters,
                                    gpointer gchanged_params)
  {
    g_return_if_fail (g_variant_n_children (parameters) == 3);
    auto changed_properties = g_variant_get_child_value (parameters, 1);
    g_return_if_fail (g_variant_is_of_type (changed_properties, G_VARIANT_TYPE_DICTIONARY));
    auto changed_params = static_cast<ChangedParams*>(gchanged_params);

    gint32 power_level;
    if (g_variant_lookup (changed_properties, "PowerLevel", "i", &power_level, nullptr))
    {
      changed_params->power_level = power_level;
      changed_params->fields |= FIELD_POWER_LEVEL;
    }

    gboolean is_warning;
    if (g_variant_lookup (changed_properties, "IsWarning", "b", &is_warning, nullptr))
    {
      changed_params->is_warning = is_warning;
      changed_params->fields |= FIELD_IS_WARNING;
    }

    g_variant_unref (changed_properties);
  }
}

TEST_F(NotifyFixture, PercentageToLevel)
{
  auto battery = indicator_power_device_new ("/object/path",
                                             UP_DEVICE_KIND_BATTERY,
                                             50.0,
                                             UP_DEVICE_STATE_DISCHARGING,
                                             30);

  // confirm that the power levels trigger at the right percentages
  for (int i=100; i>=0; --i)
    {
      g_object_set (battery, INDICATOR_POWER_DEVICE_PERCENTAGE, (gdouble)i, nullptr);
      const auto level = indicator_power_notifier_get_power_level(battery);

       if (i <= 2)
         EXPECT_EQ (POWER_LEVEL_CRITICAL, level);
       else if (i <= 5)
         EXPECT_EQ (POWER_LEVEL_VERY_LOW, level);
       else if (i <= 10)
         EXPECT_EQ (POWER_LEVEL_LOW, level);
       else
         EXPECT_EQ (POWER_LEVEL_OK, level);
     }

  g_object_unref (battery);
}


TEST_F(NotifyFixture, LevelsDuringBatteryDrain)
{
  auto battery = indicator_power_device_new ("/object/path",
                                             UP_DEVICE_KIND_BATTERY,
                                             50.0,
                                             UP_DEVICE_STATE_DISCHARGING,
                                             30);

  // set up a notifier and give it the battery so changing the battery's
  // charge should show up on the bus.
  auto notifier = indicator_power_notifier_new ();
  indicator_power_notifier_set_battery (notifier, battery);
  indicator_power_notifier_set_bus (notifier, bus);
  wait_msec();

  ChangedParams changed_params;
  auto subscription_tag = g_dbus_connection_signal_subscribe (bus,
                                                              nullptr,
                                                              "org.freedesktop.DBus.Properties",
                                                              "PropertiesChanged",
                                                              BUS_PATH"/Battery",
                                                              nullptr,
                                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                                              on_battery_property_changed,
                                                              &changed_params,
                                                              nullptr);

  // confirm that draining the battery puts
  // the power_level change through its paces                                                              
  for (int i=100; i>=0; --i)
    {
      changed_params = ChangedParams();
      EXPECT_TRUE (changed_params.fields == 0);

      const auto old_level = indicator_power_notifier_get_power_level(battery);
      g_object_set (battery, INDICATOR_POWER_DEVICE_PERCENTAGE, (gdouble)i, nullptr);
      const auto new_level = indicator_power_notifier_get_power_level(battery);
      wait_msec();

      if (old_level == new_level)
        {
          EXPECT_EQ (0, changed_params.fields);
        }
      else
        {
          EXPECT_EQ (FIELD_POWER_LEVEL, changed_params.fields);
          EXPECT_EQ (new_level, changed_params.power_level);
        }
    }

  // cleanup
  g_dbus_connection_signal_unsubscribe (bus, subscription_tag);
  g_object_unref (notifier);
  g_object_unref (battery);
}

