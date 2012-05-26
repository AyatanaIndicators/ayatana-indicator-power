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
#include "indicator-power.h"

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

class IndicatorTest : public ::testing::Test
{
  protected:

    IndicatorPowerDevice * ac_device;
    IndicatorPowerDevice * battery_device;

    virtual void SetUp()
    {
      ensure_glib_initialized ();
         
      g_setenv( "GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, TRUE);

      ac_device = indicator_power_device_new (
        "/org/freedesktop/UPower/devices/line_power_AC",
        UP_DEVICE_KIND_LINE_POWER,
        ". GThemedIcon ac-adapter-symbolic ac-adapter ",
        0.0, UP_DEVICE_STATE_UNKNOWN, 0);

      battery_device = indicator_power_device_new (
        "/org/freedesktop/UPower/devices/battery_BAT0",
        UP_DEVICE_KIND_BATTERY,
        ". GThemedIcon battery-good-symbolic gpm-battery-060 battery-good ",
        52.871712, UP_DEVICE_STATE_DISCHARGING, 8834);
    }

    virtual void TearDown()
    {
      g_object_unref (battery_device);
      g_object_unref (ac_device);
    }

    const char* GetAccessibleDesc (IndicatorPower * power) const
    {
      GList * entries = indicator_object_get_entries (INDICATOR_OBJECT(power));
      g_assert (g_list_length(entries) == 1); 
      IndicatorObjectEntry * entry = static_cast<IndicatorObjectEntry*>(entries->data);
      const char * ret = entry->accessible_desc;
      g_list_free (entries);
      return ret;
    }
};

/***
****
***/

TEST_F(IndicatorTest, GObjectNew)
{
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (IS_INDICATOR_POWER(o));
  g_object_run_dispose (o); // used to get coverage of both branches in the object's dispose func's g_clear_*() calls
  g_object_unref (o);
}

TEST_F(IndicatorTest, SetDevices)
{
  IndicatorPower * power = INDICATOR_POWER(g_object_new (INDICATOR_POWER_TYPE, NULL));
  IndicatorPowerDevice * devices[] = { ac_device, battery_device };

  indicator_power_set_devices (power, devices, G_N_ELEMENTS(devices));
 
  g_object_unref (power);
}

TEST_F(IndicatorTest, DischargingStrings)
{
  IndicatorPower * power = INDICATOR_POWER(g_object_new (INDICATOR_POWER_TYPE, NULL));

  // give the indicator a discharging battery with 1 hour of life left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                INDICATOR_POWER_DEVICE_TIME, guint64(60*60),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1); 
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (1 hour left (50%))");

  // give the indicator a discharging battery with 2 hours of life left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_PERCENTAGE, 100.0,
                INDICATOR_POWER_DEVICE_TIME, guint64(60*60*2),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (2 hours left (100%))");

  // give the indicator a discharging battery with over 12 hours of life left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_TIME, guint64(60*60*12 + 1),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery");

  // give the indicator a discharging battery with 29 seconds left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_TIME, guint64(29),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (Unknown time left (100%))");

  // what happens if the time estimate isn't available
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_TIME, guint64(0),
                INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (50%)");

  // what happens if the time estimate AND percentage isn't available
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_TIME, guint64(0),
                INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (not present)");

  // cleanup
  g_object_unref (power);
}

TEST_F(IndicatorTest, ChargingStrings)
{
  IndicatorPower * power = INDICATOR_POWER(g_object_new (INDICATOR_POWER_TYPE, NULL));

  // give the indicator a discharging battery with 1 hour of life left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                INDICATOR_POWER_DEVICE_TIME, guint64(60*60),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (1 hour to charge (50%))");

  // give the indicator a discharging battery with 2 hours of life left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_TIME, guint64(60*60*2),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (2 hours to charge (50%))");

  // cleanup
  g_object_unref (power);
}

TEST_F(IndicatorTest, ChargedStrings)
{
  IndicatorPower * power = INDICATOR_POWER(g_object_new (INDICATOR_POWER_TYPE, NULL));

  // give the indicator a discharging battery with 1 hour of life left
  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                INDICATOR_POWER_DEVICE_PERCENTAGE, 100.0,
                INDICATOR_POWER_DEVICE_TIME, guint64(0),
                NULL);
  indicator_power_set_devices (power, &battery_device, 1);
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (charged)");

  // cleanup
  g_object_unref (power);
}

TEST_F(IndicatorTest, AvoidChargingBatteriesWithZeroSecondsLeft)
{
  IndicatorPower * power = INDICATOR_POWER(g_object_new (INDICATOR_POWER_TYPE, NULL));

  g_object_set (battery_device,
                INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                INDICATOR_POWER_DEVICE_PERCENTAGE, 100.0,
                INDICATOR_POWER_DEVICE_TIME, guint64(0),
                NULL);
  IndicatorPowerDevice * bad_battery_device  = indicator_power_device_new (
    "/org/freedesktop/UPower/devices/battery_BAT0",
    UP_DEVICE_KIND_BATTERY,
    ". GThemedIcon battery-good-symbolic gpm-battery-060 battery-good ",
    53, UP_DEVICE_STATE_CHARGING, 0);

  IndicatorPowerDevice * devices[] = { battery_device, bad_battery_device };
  indicator_power_set_devices (power, devices, G_N_ELEMENTS(devices));
  ASSERT_STREQ (GetAccessibleDesc(power), "Battery (53%)");

  // cleanup
  g_object_unref (power);
  g_object_unref (bad_battery_device);
}

