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
#include "device.h"

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

class DeviceTest : public ::testing::Test
{
  protected:

    virtual void SetUp()
    {
      ensure_glib_initialized ();
    }

    virtual void TearDown()
    {
    }

  protected:

    void check_icon_names (const IndicatorPowerDevice * device, const char * expected)
    {
      char ** names = indicator_power_device_get_icon_names (device);
      char * str = g_strjoinv (";", names);
      ASSERT_STREQ (expected, str);
      g_free (str);
      g_strfreev (names);
    }

    void check_strings (const IndicatorPowerDevice * device,
                        const char * expected_timestring,
                        const char * expected_details,
                        const char * expected_accessible)
    {
      char * timestring = NULL;
      char * details = NULL;
      char * accessible = NULL;

      indicator_power_device_get_time_details (device, &timestring, &details, &accessible);
      EXPECT_STREQ (expected_timestring, timestring);
      EXPECT_STREQ (expected_details, details);
      EXPECT_STREQ (expected_accessible, accessible);

      g_free (accessible);
      g_free (details);
      g_free (timestring);
    }
};


/***
****
***/

TEST_F(DeviceTest, GObjectNew)
{
  ensure_glib_initialized ();

  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(o));
  g_object_run_dispose (o); // used to get coverage of both branches in the object's dispose func's g_clear_*() calls
  g_object_unref (o);
}

TEST_F(DeviceTest, Properties)
{
  int i;
  gdouble d;
  GObject * o;
  gchar * str;
  guint64 u64;
  const gchar * key;

  ensure_glib_initialized ();

  o = G_OBJECT (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(o));

  /* Test getting & setting a Device's properties. */

  // KIND
  key = INDICATOR_POWER_DEVICE_KIND;
  g_object_set (o, key, UP_DEVICE_KIND_BATTERY, NULL);
  g_object_get (o, key, &i, NULL);
  ASSERT_EQ (i, UP_DEVICE_KIND_BATTERY);

  // STATE
  key = INDICATOR_POWER_DEVICE_STATE;
  g_object_set (o, key, UP_DEVICE_STATE_CHARGING, NULL);
  g_object_get (o, key, &i, NULL);
  ASSERT_EQ (i, UP_DEVICE_STATE_CHARGING);

  // OBJECT_PATH
  key = INDICATOR_POWER_DEVICE_OBJECT_PATH;
  g_object_set (o, key, "/object/path", NULL);
  g_object_get (o, key, &str, NULL);
  ASSERT_STREQ (str, "/object/path");
  g_free (str);

  // PERCENTAGE
  key = INDICATOR_POWER_DEVICE_PERCENTAGE;
  g_object_set (o, key, 50.0, NULL);
  g_object_get (o, key, &d, NULL);
  ASSERT_EQ((int)d, 50);

  // TIME
  key = INDICATOR_POWER_DEVICE_TIME;
  g_object_set (o, key, (guint64)30, NULL);
  g_object_get (o, key, &u64, NULL);
  ASSERT_EQ(u64, 30);

  // cleanup
  g_object_unref (o);
}

TEST_F(DeviceTest, New)
{
  ensure_glib_initialized ();

  IndicatorPowerDevice * device = indicator_power_device_new ("/object/path",
                                                              UP_DEVICE_KIND_BATTERY,
                                                              50.0,
                                                              UP_DEVICE_STATE_CHARGING,
                                                              30);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_BATTERY);
  ASSERT_EQ (indicator_power_device_get_state(device), UP_DEVICE_STATE_CHARGING);
  ASSERT_STREQ (indicator_power_device_get_object_path(device), "/object/path");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

  /* cleanup */
  g_object_unref (device);
}

TEST_F(DeviceTest, NewFromVariant)
{
  ensure_glib_initialized ();

  GVariant * variant = g_variant_new ("(susdut)",
                                      "/object/path",
                                      UP_DEVICE_KIND_BATTERY,
                                      "icon",
                                      50.0,
                                      UP_DEVICE_STATE_CHARGING,
                                      30);
  IndicatorPowerDevice * device = indicator_power_device_new_from_variant (variant);
  ASSERT_TRUE (variant != NULL);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_BATTERY);
  ASSERT_EQ (indicator_power_device_get_state(device), UP_DEVICE_STATE_CHARGING);
  ASSERT_STREQ (indicator_power_device_get_object_path(device), "/object/path");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

  /* cleanup */
  g_object_unref (device);
  g_variant_unref (variant);
}

TEST_F(DeviceTest, BadAccessors)
{
  ensure_glib_initialized ();

  // test that these functions can handle being passed NULL pointers
  IndicatorPowerDevice * device = NULL;
  indicator_power_device_get_kind (device);
  indicator_power_device_get_time (device);
  indicator_power_device_get_state (device);
  indicator_power_device_get_percentage (device);
  indicator_power_device_get_object_path (device);

  // test that these functions can handle being passed non-device GObjects
  device = reinterpret_cast<IndicatorPowerDevice*>(g_cancellable_new ());
  indicator_power_device_get_kind (device);
  indicator_power_device_get_time (device);
  indicator_power_device_get_state (device);
  indicator_power_device_get_percentage (device);
  indicator_power_device_get_object_path (device);
  g_object_unref (device);
}

/***
****
***/

TEST_F(DeviceTest, IconNames)
{
  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  GObject * o = G_OBJECT(device);

  /* bad arguments */
  ASSERT_TRUE (indicator_power_device_get_icon_names (NULL) == NULL);

  /* power */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_LINE_POWER,
                   NULL);
  check_icon_names (device, "ac-adapter-symbolic;"
                            "ac-adapter;");

  /* monitor */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_MONITOR,
                   NULL);
  check_icon_names (device, "gpm-monitor-symbolic;"
                            "gpm-monitor;");

  /* empty battery */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_EMPTY,
                   NULL);
  check_icon_names (device, "battery-empty-symbolic;"
                            "gpm-battery-empty;"
                            "gpm-battery-000;"
                            "battery-empty;");

  /* charged battery */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                   NULL);
  check_icon_names (device, "battery-full-charged-symbolic;battery-full-charging-symbolic;"
                            "gpm-battery-full;gpm-battery-100;"
                            "battery-full-charged;battery-full-charging;");

  /* charging battery, 95% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                   NULL);
  check_icon_names (device, "battery-caution-charging-symbolic;"
                            "gpm-battery-000-charging;"
                            "battery-caution-charging;");

  /* charging battery, 85% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                   NULL);
  check_icon_names (device, "battery-caution-charging-symbolic;"
                            "gpm-battery-000-charging;"
                            "battery-caution-charging;");

  /* charging battery, 50% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   NULL);
  check_icon_names (device, "battery-caution-charging-symbolic;"
                            "gpm-battery-000-charging;"
                            "battery-caution-charging;");

  /* charging battery, 25% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                   NULL);
  check_icon_names (device, "battery-caution-charging-symbolic;"
                            "gpm-battery-000-charging;"
                            "battery-caution-charging;");

  /* charging battery, 5% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                   NULL);
  check_icon_names (device, "battery-caution-charging-symbolic;"
                            "gpm-battery-000-charging;"
                            "battery-caution-charging;");


  /* discharging battery, 95% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                   NULL);
  check_icon_names (device, "battery-full-symbolic;"
                            "gpm-battery-100;"
                            "battery-full;");

  /* discharging battery, 85% */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                   NULL);
  check_icon_names (device, "battery-full-symbolic;"
                            "gpm-battery-080;"
                            "battery-full;");

  /* discharging battery, 50% -- 1 hour left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                   NULL);
  check_icon_names (device, "battery-good-symbolic;"
                            "gpm-battery-060;"
                            "battery-good;");

  /* discharging battery, 25% -- 1 hour left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                   INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                   NULL);
  check_icon_names (device, "battery-good-symbolic;"
                            "gpm-battery-040;"
                            "battery-good;");

  /* discharging battery, 25% -- 15 minutes left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                   INDICATOR_POWER_DEVICE_TIME, (guint64)(60*15),
                   NULL);
  check_icon_names (device, "battery-low-symbolic;"
                            "gpm-battery-020;"
                            "battery-low;");

  /* discharging battery, 5% -- 1 hour left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                   INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                   NULL);
  check_icon_names (device, "battery-good-symbolic;"
                            "gpm-battery-040;"
                            "battery-good;");

  /* discharging battery, 5% -- 15 minutes left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                   INDICATOR_POWER_DEVICE_TIME, (guint64)(60*15),
                   NULL);
  check_icon_names (device, "battery-caution-symbolic;"
                            "gpm-battery-000;"
                            "battery-caution;"); 
  /* state unknown */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN, 
                   NULL);
  check_icon_names (device, "battery-missing-symbolic;"
                            "gpm-battery-missing;"
                            "battery-missing;");

  /* cleanup */
  g_object_unref(o);
}


TEST_F(DeviceTest, Labels)
{
  // set our language so that i18n won't break these tests
  char * real_lang = g_strdup(g_getenv ("LANG"));
  g_setenv ("LANG", "en_US.UTF-8", TRUE);

  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  GObject * o = G_OBJECT(device);

  /* charging */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*61),
                   NULL);
  check_strings (device, "(1:01)",
                         "Battery (1:01 to charge)",
                         "Battery (1 hour 1 minute to charge (50%))");

  /* discharging, < 12 hours left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*61),
                   NULL);
  check_strings (device, "1:01",
                         "Battery (1:01 left)",
                         "Battery (1 hour 1 minute left (50%))");

  /* discharging, > 12 hours left */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*60*13),
                   NULL);
  check_strings (device, "13:00", "Battery", "Battery");

  /* fully charged */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 100.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_strings (device, "", "Battery (charged)", "Battery (charged)");

  /* percentage but no time estimate */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_strings (device, "(50%)", "Battery (50%)", "Battery (50%)");

  /* no percentage, no time estimate */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_strings (device, "(not present)", "Battery (not present)", "Battery (not present)");

  /* power line */
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_LINE_POWER,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_strings (device, "AC Adapter", "AC Adapter", "AC Adapter");

  /* cleanup */
  g_object_unref(o);
  g_setenv ("LANG", real_lang, TRUE);
  g_free (real_lang);
}
