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
};

/***
****
***/

TEST_F(IndicatorTest, GObjectNew)
{
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (IS_INDICATOR_POWER(o));
  g_object_unref (o);
}

TEST_F(IndicatorTest, SetDevices)
{
  IndicatorPower * power = INDICATOR_POWER(g_object_new (INDICATOR_POWER_TYPE, NULL));
  IndicatorPowerDevice * devices[] = { ac_device, battery_device };

  indicator_power_set_devices (power, devices, G_N_ELEMENTS(devices));
 
  g_object_unref (power);
}
