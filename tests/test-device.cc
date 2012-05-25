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

/***
****
***/

TEST(DeviceTest, GObjectNew)
{
  ensure_glib_initialized ();

  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(o));
  g_object_unref (o);
}

TEST(DeviceTest, Properties)
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

  // ICON
  key = INDICATOR_POWER_DEVICE_ICON;
  g_object_set (o, key, "something", NULL);
  g_object_get (o, key, &str, NULL);
  ASSERT_STREQ (str, "something");
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

TEST(DeviceTest, New)
{
  ensure_glib_initialized ();

  IndicatorPowerDevice * device = indicator_power_device_new ("/object/path",
                                                              UP_DEVICE_KIND_BATTERY,
                                                              "icon",
                                                              50.0,
                                                              UP_DEVICE_STATE_CHARGING,
                                                              30);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_BATTERY);
  ASSERT_EQ (indicator_power_device_get_state(device), UP_DEVICE_STATE_CHARGING);
  ASSERT_STREQ (indicator_power_device_get_object_path(device), "/object/path");
  ASSERT_STREQ (indicator_power_device_get_icon(device), "icon");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

  /* cleanup */
  g_object_unref (device);
}

TEST(DeviceTest, NewFromVariant)
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
  ASSERT_STREQ (indicator_power_device_get_icon(device), "icon");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

  /* cleanup */
  g_object_unref (device);
  g_variant_unref (variant);
}

TEST(DeviceTest, BadAccessors)
{
  ensure_glib_initialized ();

  // test that these functions can handle being passed NULL pointers
  IndicatorPowerDevice * device = NULL;
  indicator_power_device_get_kind(device);
  indicator_power_device_get_state(device);
  indicator_power_device_get_object_path(device);
  indicator_power_device_get_icon(device);
  indicator_power_device_get_percentage(device);
  indicator_power_device_get_time(device);
}
