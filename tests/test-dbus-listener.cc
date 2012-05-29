/*
Copyright 2012 Canonical Ltd.

Authors:
    Charles Kerr <charles.kerr@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>

#include "dbus-listener.h"
#include "device.h"

/***
****
***/

namespace
{
  void ensure_glib_initialized ()
  {
    static bool initialized = false;

    if (G_UNLIKELY(!initialized))
    {
      initialized = true;
      g_type_init();
    }
  }
}

/***
****
***/

class DbusListenerTest : public ::testing::Test
{
  protected:
    GSList * devices;
    GDBusConnection * connection;
    GMainLoop * mainloop;
    GTestDBus * bus;
    GDBusNodeInfo * gsd_power_introspection_data;
    GVariant * get_devices_retval;
    bool bus_acquired;
    bool name_acquired;
    int gsd_name_ownership_id;
    int gsd_power_registration_id;
    char * gsd_power_error_string;
  
  protected:

    static void
    on_bus_acquired (GDBusConnection *conn, const gchar * name, gpointer gself)
    {
      g_debug ("bus acquired: %s, connection is %p", name, conn);

      DbusListenerTest * test = static_cast<DbusListenerTest*> (gself);
      test->connection = conn;
      test->bus_acquired = true;
    }

    static void
    on_name_acquired (GDBusConnection * conn, const gchar * name, gpointer gself)
    {
      g_debug ("name acquired: %s, connection is %p", name, conn);

      DbusListenerTest * test = static_cast<DbusListenerTest*> (gself);
      test->name_acquired = true;
      g_main_loop_quit (test->mainloop);
    }

    static void
    on_name_lost (GDBusConnection * conn, const gchar * name, gpointer gself)
    {
      g_debug ("name lost: %s, connection is %p", name, conn);

      DbusListenerTest * test = static_cast<DbusListenerTest*> (gself);
      test->name_acquired = false;
      g_main_loop_quit (test->mainloop);
    }

  protected:

    static void
    on_devices_enumerated (IndicatorPowerDbusListener * l, GSList * devices, gpointer gself)
    {
      g_debug ("on_devices_enumerated");

      DbusListenerTest * test = static_cast<DbusListenerTest*> (gself);
      test->name_acquired = false;
      g_slist_foreach (devices, (GFunc)g_object_ref, NULL);
      test->devices = g_slist_copy (devices);
      g_main_loop_quit (test->mainloop);
    }

    static void
    gsd_power_handle_method_call (GDBusConnection       * connection,
                                  const gchar           * sender,
                                  const gchar           * object_path,
                                  const gchar           * interface_name,
                                  const gchar           * method_name,
                                  GVariant              * parameters,
                                  GDBusMethodInvocation * invocation,
                                  gpointer                gself)
    {
      DbusListenerTest * test = static_cast<DbusListenerTest*> (gself);
      g_assert (!g_strcmp0 (method_name, "GetDevices"));
      if (test->gsd_power_error_string != NULL) {
        const GQuark domain = g_quark_from_static_string ("mock-gsd-power");
        g_message ("returning an error");
        g_dbus_method_invocation_return_error_literal (invocation, domain, 0, test->gsd_power_error_string);
      } else {
        g_dbus_method_invocation_return_value (invocation, test->get_devices_retval);
      }
    }

  protected:

    virtual void SetUp()
    {
      bus_acquired = false;
      name_acquired = false;
      connection = NULL;
      devices = NULL;
      get_devices_retval = NULL;
      gsd_power_error_string = NULL;

      // bring up the test bus
      ensure_glib_initialized ();
      mainloop =  g_main_loop_new (NULL, FALSE);
      bus = g_test_dbus_new (G_TEST_DBUS_NONE);
      g_test_dbus_up (bus);

      // own org.gnome.SettingsDameon on this test bus
      gsd_name_ownership_id = g_bus_own_name (
        G_BUS_TYPE_SESSION,
        GSD_SERVICE,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        on_name_acquired,
        on_name_lost,
        this, NULL);
      ASSERT_FALSE (bus_acquired);
      ASSERT_FALSE (name_acquired);
      ASSERT_TRUE (connection == NULL);
      g_main_loop_run (mainloop);
      ASSERT_TRUE (bus_acquired);
      ASSERT_TRUE (name_acquired);
      ASSERT_TRUE (G_IS_DBUS_CONNECTION(connection));

      // parse the org.gnome.SettingsDaemon.Power interface
      const gchar introspection_xml[] =
        "<node>"
        "  <interface name='org.gnome.SettingsDaemon.Power'>"
        "    <property name='Icon' type='s' access='read'>"
        "    </property>"
        "    <property name='Tooltip' type='s' access='read'>"
        "    </property>"
        "    <method name='GetDevices'>"
        "      <arg name='devices' type='a(susdut)' direction='out' />"
        "    </method>"
        "  </interface>"
        "</node>";
      gsd_power_introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
      ASSERT_TRUE (gsd_power_introspection_data != NULL);

      // Set up a mock GSD.
      // All it really does is wait for calls to GetDevice and
      // returns the get_devices_retval variant
      const GDBusInterfaceVTable gsd_power_interface_vtable = {
        gsd_power_handle_method_call,
        NULL, /* GetProperty */
        NULL, /* SetProperty */
      };
      gsd_power_registration_id = g_dbus_connection_register_object (connection,
                                                                     GSD_POWER_DBUS_PATH,
                                                                     gsd_power_introspection_data->interfaces[0],
                                                                     &gsd_power_interface_vtable,
                                                                     this, NULL, NULL);
    }

    virtual void TearDown()
    {
      g_free (gsd_power_error_string);

      g_dbus_connection_unregister_object (connection, gsd_power_registration_id);

      g_dbus_node_info_unref (gsd_power_introspection_data);
      g_bus_unown_name (gsd_name_ownership_id);

      g_slist_free_full (devices, g_object_unref);
      g_test_dbus_down (bus);
      g_main_loop_unref (mainloop);
    }
};

/***
****
***/


TEST_F(DbusListenerTest, GSDHasPowerAndBattery)
{
  // build a GetDevices retval that shows an AC line and a discharging battery
  GVariantBuilder * builder = g_variant_builder_new (G_VARIANT_TYPE("a(susdut)"));
  g_variant_builder_add (builder, "(susdut)",
                         "/org/freedesktop/UPower/devices/line_power_AC",
                         UP_DEVICE_KIND_LINE_POWER,
                         ". GThemedIcon ac-adapter-symbolic ac-adapter ",
                         0.0,
                         UP_DEVICE_STATE_UNKNOWN,
                         0);
  g_variant_builder_add (builder, "(susdut)",
                         "/org/freedesktop/UPower/devices/battery_BAT0",
                         UP_DEVICE_KIND_BATTERY,
                         ". GThemedIcon battery-good-symbolic gpm-battery-060 battery-good ",
                         52.871712,
                         UP_DEVICE_STATE_DISCHARGING,
                         8834);
  GVariant * value = g_variant_builder_end (builder);
  get_devices_retval = g_variant_new_tuple (&value, 1);
  g_variant_builder_unref (builder);

  // create an i-power dbus listener to watch for GSD
  ASSERT_EQ (g_slist_length(devices), 0);
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DBUS_LISTENER_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DBUS_LISTENER(o));
  g_signal_connect(o, INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED,
                   G_CALLBACK(on_devices_enumerated), this);
  g_main_loop_run (mainloop);

  // test the devices should have gotten
  ASSERT_EQ (g_slist_length(devices), 2);
  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_slist_nth_data (devices, 0));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_LINE_POWER);
  ASSERT_STREQ ("/org/freedesktop/UPower/devices/line_power_AC", indicator_power_device_get_object_path(device));
  device = INDICATOR_POWER_DEVICE (g_slist_nth_data (devices, 1));
  ASSERT_EQ (UP_DEVICE_KIND_BATTERY, indicator_power_device_get_kind(device));
  ASSERT_STREQ ("/org/freedesktop/UPower/devices/battery_BAT0", indicator_power_device_get_object_path(device));

  // cleanup
  g_object_run_dispose (o); // used to get coverage of both branches in the object's dispose func's g_clear_*() calls
  g_object_unref (o); 
}

TEST_F(DbusListenerTest, GSDHasNoDevices)
{
  GVariantBuilder * builder = g_variant_builder_new (G_VARIANT_TYPE("a(susdut)"));
  GVariant * value = g_variant_builder_end (builder);
  get_devices_retval = g_variant_new_tuple (&value, 1);
  g_variant_builder_unref (builder);

  // create an i-power dbus listener to watch for GSD
  ASSERT_EQ (g_slist_length(devices), 0);
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DBUS_LISTENER_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DBUS_LISTENER(o));
  g_signal_connect(o, INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED,
                   G_CALLBACK(on_devices_enumerated), this);
  g_main_loop_run (mainloop);

  // test the devices should have gotten
  ASSERT_EQ (g_slist_length(devices), 0);

  // FIXME: this test improves coverage and confirms that the code
  // doesn't crash, but more meaningful tests would be better too.

  // cleanup
  g_object_run_dispose (o); // used to get coverage of both branches in the object's dispose func's g_clear_*() calls
  g_object_unref (o); 
}

TEST_F(DbusListenerTest, GSDReturnsError)
{
  gsd_power_error_string = g_strdup ("no devices for you lol");

  // create an i-power dbus listener to watch for GSD
  ASSERT_EQ (g_slist_length(devices), 0);
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DBUS_LISTENER_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DBUS_LISTENER(o));
  g_signal_connect(o, INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED,
                   G_CALLBACK(on_devices_enumerated), this);
  g_main_loop_run (mainloop);

  // test the devices should have gotten
  ASSERT_EQ (g_slist_length(devices), 0);

  // FIXME: this test improves coverage and confirms that the code
  // doesn't crash, but more meaningful tests would be better too.

  // cleanup
  g_object_run_dispose (o); // used to get coverage of both branches in the object's dispose func's g_clear_*() calls
  g_object_unref (o); 
}

/* This test emits a PropertiesChanged signal and confirms that
   the dbus listener asks for a fresh set of devices as a result. */
TEST_F(DbusListenerTest, GSDPropChanged)
{
  gsd_power_error_string = g_strdup ("no devices for you lol");

  // create an i-power dbus listener to watch for GSD
  ASSERT_EQ (g_slist_length(devices), 0);
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DBUS_LISTENER_TYPE, NULL));
  ASSERT_TRUE (INDICATOR_IS_POWER_DBUS_LISTENER(o));
  g_signal_connect(o, INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED,
                   G_CALLBACK(on_devices_enumerated), this);
  g_main_loop_run (mainloop);
  // test the devices should have gotten
  ASSERT_EQ (g_slist_length(devices), 0);

  // build a GetDevices retval that shows an AC line and a discharging battery
  g_clear_pointer (&gsd_power_error_string, g_free);
  GVariantBuilder * builder = g_variant_builder_new (G_VARIANT_TYPE("a(susdut)"));
  g_variant_builder_add (builder, "(susdut)",
                         "/org/freedesktop/UPower/devices/line_power_AC",
                         UP_DEVICE_KIND_LINE_POWER,
                         ". GThemedIcon ac-adapter-symbolic ac-adapter ",
                         0.0,
                         UP_DEVICE_STATE_UNKNOWN,
                         0);
  g_variant_builder_add (builder, "(susdut)",
                         "/org/freedesktop/UPower/devices/battery_BAT0",
                         UP_DEVICE_KIND_BATTERY,
                         ". GThemedIcon battery-good-symbolic gpm-battery-060 battery-good ",
                         52.871712,
                         UP_DEVICE_STATE_DISCHARGING,
                         8834);
  GVariant * value = g_variant_builder_end (builder);
  get_devices_retval = g_variant_new_tuple (&value, 1);
  g_variant_builder_unref (builder);

  // tell the listener that some property changed
  GVariantBuilder props_builder;
  g_variant_builder_init (&props_builder, G_VARIANT_TYPE ("a{sv}"));
  GVariant * props_changed = g_variant_new ("(s@a{sv}@as)", GSD_POWER_DBUS_INTERFACE,
    g_variant_builder_end (&props_builder),
    g_variant_new_strv (NULL, 0));
  g_variant_ref_sink (props_changed);
  GError * error = NULL;
  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 GSD_POWER_DBUS_PATH,
                                 "org.freedesktop.DBus.Properties",
                                 "PropertiesChanged",
                                 props_changed,
                                 &error);
  ASSERT_TRUE(error == NULL);

  // give i-power's dbus listener a chance to ask for a fresh device list
  g_main_loop_run (mainloop);
  ASSERT_EQ (g_slist_length(devices), 2);

  // cleanup
  g_object_unref (o); 
}
