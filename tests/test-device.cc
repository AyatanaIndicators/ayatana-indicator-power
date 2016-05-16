/*
 * Copyright 2012-2016 Canonical Ltd.
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

#include "device.h"
#include "service.h"

#include <gio/gio.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath> // ceil()
#include <string>


class DeviceTest : public ::testing::Test
{
  private:

    guint log_handler_id;

    int log_count_ipower_actual;

    static void log_count_func (const gchar    * log_domain G_GNUC_UNUSED,
                                GLogLevelFlags   log_level  G_GNUC_UNUSED,
                                const gchar    * message    G_GNUC_UNUSED,
                                gpointer         gself)
    {
      reinterpret_cast<DeviceTest*>(gself)->log_count_ipower_actual++;
    }

  protected:

    int log_count_ipower_expected;

  protected:

    virtual void SetUp()
    {
      const GLogLevelFlags flags = GLogLevelFlags(G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING);
      log_handler_id = g_log_set_handler(G_LOG_DOMAIN, flags, log_count_func, this);
      log_count_ipower_expected = 0;
      log_count_ipower_actual = 0;
    }

    virtual void TearDown()
    {
      ASSERT_EQ (log_count_ipower_expected, log_count_ipower_actual);
      g_log_remove_handler (G_LOG_DOMAIN, log_handler_id);
    }

  protected:

    static std::string get_icon_names_from_device(const IndicatorPowerDevice* device)
    {
      std::string ret;
      char ** names = indicator_power_device_get_icon_names (device);
      char * str = g_strjoinv (";", names);
      if (str != nullptr)
        ret = str;
      g_free (str);
      g_strfreev (names);
      return ret;
    }

    void check_label (const IndicatorPowerDevice * device,
                      const char * expected_label)
    {
      char * label = indicator_power_device_get_readable_text (device);
      EXPECT_STREQ (expected_label, label);
      g_free (label);
    }

    void check_header (const IndicatorPowerDevice * device,
                       const char * expected_time_and_percent,
                       const char * expected_time,
                       const char * expected_percent,
                       const char * expected_a11y)
    {
      char * a11y = NULL;
      char * title = NULL;

      title = indicator_power_device_get_readable_title (device, true, true);
      if (expected_time_and_percent)
        EXPECT_STREQ (expected_time_and_percent, title);
      else
        EXPECT_EQ(NULL, title);
      g_free (title);

      title = indicator_power_device_get_readable_title (device, true, false);
      if (expected_time)
        EXPECT_STREQ (expected_time, title);
      else
        EXPECT_EQ(NULL, title);
      g_free (title);

      title = indicator_power_device_get_readable_title (device, false, true);
      if (expected_percent)
        EXPECT_STREQ (expected_percent, title);
      else
        EXPECT_EQ(NULL, title);
      g_free (title);

      title = indicator_power_device_get_readable_title (device, false, false);
      EXPECT_EQ(NULL, title);
      g_free (title);

      a11y = indicator_power_device_get_accessible_title (device, false, false);
      if (expected_a11y)
        EXPECT_STREQ (expected_a11y, a11y);
      else
        EXPECT_EQ(NULL, a11y);
      g_free (a11y);
    }
};

#define EXPECT_ICON_NAMES_EQ(expected_in,device_in)                          \
  do {                                                                       \
    const std::string tmp_expected {expected_in};                            \
    const std::string tmp_actual {get_icon_names_from_device(device_in)};    \
    EXPECT_EQ(tmp_expected, tmp_actual);                                     \
  } while(0)

/***
****
***/

TEST_F(DeviceTest, GObjectNew)
{
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

  o = G_OBJECT (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(o));

  // Test getting & setting a Device's properties.

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
  ASSERT_EQ(int(d), 50);

  // TIME
  key = INDICATOR_POWER_DEVICE_TIME;
  g_object_set (o, key, guint64(30), NULL);
  g_object_get (o, key, &u64, NULL);
  ASSERT_EQ(u64, 30);

  // cleanup
  g_object_unref (o);
}

TEST_F(DeviceTest, New)
{
  IndicatorPowerDevice * device = indicator_power_device_new ("/object/path",
                                                              UP_DEVICE_KIND_BATTERY,
                                                              50.0,
                                                              UP_DEVICE_STATE_CHARGING,
                                                              30,
                                                              TRUE);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (UP_DEVICE_KIND_BATTERY, indicator_power_device_get_kind(device));
  ASSERT_EQ (UP_DEVICE_STATE_CHARGING, indicator_power_device_get_state(device));
  ASSERT_STREQ ("/object/path", indicator_power_device_get_object_path(device));
  ASSERT_EQ (50, int(indicator_power_device_get_percentage(device)));
  ASSERT_EQ (30, indicator_power_device_get_time(device));
  ASSERT_TRUE (indicator_power_device_get_power_supply(device));

  // cleanup
  g_object_unref (device);
}

TEST_F(DeviceTest, NewFromVariant)
{
  auto variant = g_variant_new("(susdutb)",
                               "/object/path",
                               guint32(UP_DEVICE_KIND_BATTERY),
                               "icon",
                               50.0,
                               guint32(UP_DEVICE_STATE_CHARGING),
                               guint64(30),
                               TRUE);
  IndicatorPowerDevice * device = indicator_power_device_new_from_variant (variant);
  ASSERT_TRUE (variant != NULL);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (UP_DEVICE_KIND_BATTERY, indicator_power_device_get_kind(device));
  ASSERT_EQ (UP_DEVICE_STATE_CHARGING, indicator_power_device_get_state(device));
  ASSERT_STREQ ("/object/path", indicator_power_device_get_object_path(device));
  ASSERT_EQ (50, int(indicator_power_device_get_percentage(device)));
  ASSERT_EQ (30, indicator_power_device_get_time(device));
  ASSERT_TRUE (indicator_power_device_get_power_supply(device));

  // cleanup
  g_object_unref (device);
  g_variant_unref (variant);
}

TEST_F(DeviceTest, BadAccessors)
{
  // test that these functions can handle being passed NULL pointers
  IndicatorPowerDevice * device = NULL;
  indicator_power_device_get_kind (device);
  indicator_power_device_get_time (device);
  indicator_power_device_get_state (device);
  indicator_power_device_get_percentage (device);
  indicator_power_device_get_object_path (device);
  log_count_ipower_expected += 5;

  // test that these functions can handle being passed non-device GObjects
  device = reinterpret_cast<IndicatorPowerDevice*>(g_cancellable_new ());
  indicator_power_device_get_kind (device);
  indicator_power_device_get_time (device);
  indicator_power_device_get_state (device);
  indicator_power_device_get_percentage (device);
  indicator_power_device_get_object_path (device);
  log_count_ipower_expected += 5;

  g_object_unref (device);
}

/***
****
***/

TEST_F(DeviceTest, IconNames)
{
  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  GObject * o = G_OBJECT(device);

  // bad arguments
  log_count_ipower_expected++;
  ASSERT_TRUE (indicator_power_device_get_icon_names (NULL) == NULL);

  // power
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_LINE_POWER,
                   NULL);
  EXPECT_ICON_NAMES_EQ("ac-adapter-symbolic;"
                       "ac-adapter",
                       device);

  // monitor
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_MONITOR,
                   NULL);
  EXPECT_ICON_NAMES_EQ("gpm-monitor-symbolic;"
                       "gpm-monitor",
                       device);

  // devices that hold a charge
  struct {
    int kind;
    const gchar * kind_str;
  } devices[] = {
    { UP_DEVICE_KIND_BATTERY, "battery" },
    { UP_DEVICE_KIND_UPS, "ups" },
    { UP_DEVICE_KIND_MOUSE, "mouse" },
    { UP_DEVICE_KIND_KEYBOARD, "keyboard" },
    { UP_DEVICE_KIND_PHONE, "phone" }
  };

  GString * expected = g_string_new (NULL);
  for (int i=0, n=G_N_ELEMENTS(devices); i<n; i++)
    {
      const int kind = devices[i].kind;
      const gchar * kind_str = devices[i].kind_str;

      // empty
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_EMPTY,
                       NULL);
      g_string_append_printf (expected, "%s-empty-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-empty;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-empty", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // charged
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                       NULL);
      g_string_append_printf (expected, "%s-full-charged-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-full;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100;", kind_str);
      g_string_append_printf (expected, "%s-full-charged;", kind_str);
      g_string_append_printf (expected, "%s-full-charging", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // charging, 95%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                       NULL);
      g_string_append_printf (expected, "%s-100-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100-charging;", kind_str);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full-charging;", kind_str);
      g_string_append_printf (expected, "%s-100;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // charging, 85%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                       NULL);
      g_string_append_printf (expected, "%s-090-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-090-charging;", kind_str);
      g_string_append_printf (expected, "%s-080-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080-charging;", kind_str);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full-charging;", kind_str);
      g_string_append_printf (expected, "%s-090;", kind_str);
      g_string_append_printf (expected, "gpm-%s-090;", kind_str);
      g_string_append_printf (expected, "%s-080;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // charging, 50%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                       NULL);
      g_string_append_printf (expected, "%s-050-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-050-charging;", kind_str);
      g_string_append_printf (expected, "%s-060-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060-charging;", kind_str);
      g_string_append_printf (expected, "%s-good-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good-charging;", kind_str);
      g_string_append_printf (expected, "%s-050;", kind_str);
      g_string_append_printf (expected, "gpm-%s-050;", kind_str);
      g_string_append_printf (expected, "%s-060;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // charging, 25%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       NULL);
      g_string_append_printf (expected, "%s-030-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-030-charging;", kind_str);
      g_string_append_printf (expected, "%s-040-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040-charging;", kind_str);
      g_string_append_printf (expected, "%s-low-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low-charging;", kind_str);
      g_string_append_printf (expected, "%s-030;", kind_str);
      g_string_append_printf (expected, "gpm-%s-030;", kind_str);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-low-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // charging, 5%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       NULL);
      g_string_append_printf (expected, "%s-010-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-010-charging;", kind_str);
      g_string_append_printf (expected, "%s-000-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000-charging;", kind_str);
      g_string_append_printf (expected, "%s-caution-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution-charging;", kind_str);
      g_string_append_printf (expected, "%s-010;", kind_str);
      g_string_append_printf (expected, "gpm-%s-010;", kind_str);
      g_string_append_printf (expected, "%s-000;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-caution-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 95%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                   NULL);
      g_string_append_printf (expected, "%s-100;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 85%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                       NULL);
      g_string_append_printf (expected, "%s-090;", kind_str);
      g_string_append_printf (expected, "gpm-%s-090;", kind_str);
      g_string_append_printf (expected, "%s-080;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 50% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                       INDICATOR_POWER_DEVICE_TIME, guint64(60*60),
                       NULL);
      g_string_append_printf (expected, "%s-050;", kind_str);
      g_string_append_printf (expected, "gpm-%s-050;", kind_str);
      g_string_append_printf (expected, "%s-060;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 25% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       INDICATOR_POWER_DEVICE_TIME, guint64(60*60),
                       NULL);
      g_string_append_printf (expected, "%s-030;", kind_str);
      g_string_append_printf (expected, "gpm-%s-030;", kind_str);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-low-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 25% -- 15 minutes left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       INDICATOR_POWER_DEVICE_TIME, guint64(60*15),
                       NULL);
      g_string_append_printf (expected, "%s-030;", kind_str);
      g_string_append_printf (expected, "gpm-%s-030;", kind_str);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-low-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 5% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       INDICATOR_POWER_DEVICE_TIME, guint64(60*60),
                   NULL);
      g_string_append_printf (expected, "%s-010;", kind_str);
      g_string_append_printf (expected, "gpm-%s-010;", kind_str);
      g_string_append_printf (expected, "%s-000;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-caution-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);

      // discharging, 5% -- 15 minutes left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       INDICATOR_POWER_DEVICE_TIME, guint64(60*15),
                       NULL);
      g_string_append_printf (expected, "%s-010;", kind_str);
      g_string_append_printf (expected, "gpm-%s-010;", kind_str);
      g_string_append_printf (expected, "%s-000;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-caution-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution", kind_str);
      EXPECT_ICON_NAMES_EQ(expected->str, device);

      // if we know the charge level, but not that it’s charging,
      // then we should use the same icons as when it’s discharging. 
      // https://wiki.ubuntu.com/Power?action=diff&rev2=78&rev1=77
      // https://bugs.launchpad.net/ubuntu/+source/indicator-power/+bug/1470080
      g_object_set (o, INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN,
                       NULL);
      EXPECT_ICON_NAMES_EQ(expected->str, device);
      g_string_truncate (expected, 0);
  }
  g_string_free (expected, TRUE);

  // cleanup
  g_object_unref(o);
}


TEST_F(DeviceTest, Labels)
{
  // set our language so that i18n won't break these tests
  char * real_lang = g_strdup(g_getenv ("LANG"));
  g_setenv ("LANG", "en_US.UTF-8", TRUE);

  // bad args: NULL device
  log_count_ipower_expected++;
  check_label (NULL, NULL);
  log_count_ipower_expected += 5;
  check_header (nullptr, nullptr, nullptr, nullptr, nullptr);

  // bad args: a GObject that isn't a device
  GObject * o = G_OBJECT(g_cancellable_new());
  log_count_ipower_expected++;
  check_label (INDICATOR_POWER_DEVICE(o), nullptr);
  log_count_ipower_expected += 5;
  check_header (NULL, NULL, NULL, NULL, NULL);
  g_object_unref (o);

  /**
  ***
  **/

  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  o = G_OBJECT(device);

  // charging
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*61),
                   NULL);
  check_label (device, "Battery (1:01 to charge)");
  check_header (device, "(1:01, 50%)",
                        "(1:01)",
                        "(50%)",
                        "Battery (1 hour 1 minute to charge)");

  // discharging, < 12 hours left
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*61),
                   NULL);
  check_label (device, "Battery (1:01 left)");
  check_header (device, "(1:01, 50%)",
                        "(1:01)",
                        "(50%)",
                        "Battery (1 hour 1 minute left)");

  // discharging, > 24 hours left
  // we don't show the clock time when > 24 hours discharging
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*60*25),
                   NULL);
  check_label (device, "Battery");
  check_header (device, "(50%)",
                        NULL,
                        "(50%)",
                        "Battery");

// fully charged
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 100.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "Battery (charged)");
  check_header (device, "(100%)",
                        NULL,
                        "(100%)",
                        "Battery (charged)");

  // percentage but no time estimate
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "Battery (estimating…)");
  check_header (device, "(estimating…, 50%)",
                        "(estimating…)",
                        "(50%)",
                        "Battery (estimating…)");

  // no percentage, no time estimate
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "Battery");
  check_header (device, NULL, NULL, NULL, "Battery");

  // power line
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_LINE_POWER,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "AC Adapter");
  check_header (device, NULL, NULL, NULL, "AC Adapter");

  // cleanup
  g_object_unref(o);
  g_setenv ("LANG", real_lang, TRUE);
  g_free (real_lang);
}


TEST_F(DeviceTest, Inestimable___this_takes_80_seconds)
{
  // set our language so that i18n won't break these tests
  auto real_lang = g_strdup(g_getenv ("LANG"));
  g_setenv ("LANG", "en_US.UTF-8", true);

  // set up the main loop
  auto loop = g_main_loop_new (nullptr, false);
  auto loop_quit_sourcefunc = [](gpointer l){
    g_main_loop_quit(static_cast<GMainLoop*>(l));
    return G_SOURCE_REMOVE;
  };

  auto device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, nullptr));
  auto o = G_OBJECT(device);

  // percentage but no time estimate
  auto timer = g_timer_new ();
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   nullptr);

  /*
   * “estimating…” if the time remaining has been inestimable for
   * less than 30 seconds; otherwise “unknown” if the time remaining
   * has been inestimable for between 30 seconds and one minute;
   * otherwise the empty string.
   */
  for (;;)
    {
      g_timeout_add_seconds (1, loop_quit_sourcefunc, loop);
      g_main_loop_run (loop);

      const auto elapsed = g_timer_elapsed (timer, nullptr);

      if (elapsed < 30)
        {
          check_label (device, "Battery (estimating…)");
          check_header (device, "(estimating…, 50%)",
                                "(estimating…)",
                                "(50%)",
                                "Battery (estimating…)");
        }
      else if (elapsed < 60)
        {
          check_label (device, "Battery (unknown)");
          check_header (device, "(unknown, 50%)",
                                "(unknown)",
                                "(50%)",
                                "Battery (unknown)");
        }
      else if (elapsed < 80)
        {
          check_label (device, "Battery");
          check_header (device, "(50%)",
                                NULL,
                                "(50%)",
                                "Battery");
        }
      else
        {
          break;
        }
    }

  g_main_loop_unref (loop);

  // cleanup
  g_timer_destroy (timer);
  g_object_unref (o);
  g_setenv ("LANG", real_lang, TRUE);
  g_free (real_lang);
}

namespace
{
  const std::array<std::pair<std::string,UpDeviceKind>,UP_DEVICE_KIND_LAST> kinds = {
    std::make_pair("unknown", UP_DEVICE_KIND_UNKNOWN),
    std::make_pair("line-power", UP_DEVICE_KIND_LINE_POWER),
    std::make_pair("battery", UP_DEVICE_KIND_BATTERY),
    std::make_pair("ups", UP_DEVICE_KIND_UPS),
    std::make_pair("monitor", UP_DEVICE_KIND_MONITOR),
    std::make_pair("mouse", UP_DEVICE_KIND_MOUSE),
    std::make_pair("keyboard", UP_DEVICE_KIND_KEYBOARD),
    std::make_pair("pda", UP_DEVICE_KIND_PDA),
    std::make_pair("phone", UP_DEVICE_KIND_PHONE),
    std::make_pair("media-player", UP_DEVICE_KIND_MEDIA_PLAYER),
    std::make_pair("tablet", UP_DEVICE_KIND_TABLET),
    std::make_pair("computer", UP_DEVICE_KIND_COMPUTER)
  };
  const std::string& kind2str(UpDeviceKind kind)
  {
    return std::find_if(
      kinds.begin(),
      kinds.end(),
      [kind](decltype(kinds[0])& i){return i.second==kind;}
    )->first;
  }
  UpDeviceKind str2kind(const std::string& str)
  {
    return std::find_if(
      kinds.begin(),
      kinds.end(),
      [str](decltype(kinds[0])& i){return i.first==str;}
    )->second;
  }

  /**
  **/

  const std::array<std::pair<std::string,UpDeviceState>,UP_DEVICE_STATE_LAST> states =
  {
    std::make_pair("unknown", UP_DEVICE_STATE_UNKNOWN),
    std::make_pair("charging", UP_DEVICE_STATE_CHARGING),
    std::make_pair("discharging", UP_DEVICE_STATE_DISCHARGING),
    std::make_pair("empty", UP_DEVICE_STATE_EMPTY),
    std::make_pair("charged", UP_DEVICE_STATE_FULLY_CHARGED),
    std::make_pair("pending-charge", UP_DEVICE_STATE_PENDING_CHARGE),
    std::make_pair("pending-discharge", UP_DEVICE_STATE_PENDING_DISCHARGE)
  };
  const std::string& state2str(UpDeviceState state)
  {
    return std::find_if(
      states.begin(),
      states.end(),
      [state](decltype(states[0])& i){return i.second==state;}
    )->first;
  }
  UpDeviceState str2state(const std::string& str)
  {
    return std::find_if(
      states.begin(),
      states.end(),
      [str](decltype(states[0])& i){return i.first==str;}
    )->second;
  }

  /**
  **/

  std::string device2str(IndicatorPowerDevice* device)
  {
    std::ostringstream o;
    const auto path = indicator_power_device_get_object_path(device);

    o << kind2str(indicator_power_device_get_kind(device))
      << ' ' << state2str(indicator_power_device_get_state(device))
      << ' ' << indicator_power_device_get_time(device)<<'m'
      << ' ' << int(ceil(indicator_power_device_get_percentage(device)))<<'%'
      << ' ' << (path ? path : "nopath")
      << ' ' << (indicator_power_device_get_power_supply(device) ? "1" : "0");

    return o.str();
  }

  IndicatorPowerDevice* str2device(const std::string& str)
  {
    auto tokens = g_strsplit(str.c_str(), " ", 0);
    g_assert(6u == g_strv_length(tokens));
    const auto kind = str2kind(tokens[0]);
    const auto state = str2state(tokens[1]);
    const time_t time = atoi(tokens[2]);
    const double pct = strtod(tokens[3],nullptr);
    const char* path = !g_strcmp0(tokens[4],"nopath") ? nullptr : tokens[4];
    const gboolean power_supply = atoi(tokens[5]);
    auto ret = indicator_power_device_new(path, kind, pct, state, time, power_supply);
    g_strfreev(tokens);
    return ret;
  }
}

/* If a device has multiple batteries and uses only one of them at a time,
   they should be presented as separate items inside the battery menu,
   but everywhere else they should be aggregated (bug 880881).
   Their percentages should be averaged. If any are discharging,
   the aggregated time remaining should be the maximum of the times
   for all those that are discharging, plus the sum of the times
   for all those that are idle. Otherwise, the aggregated time remaining
   should be the the maximum of the times for all those that are charging. */
TEST_F(DeviceTest, ChoosePrimary)
{
  const struct {
    std::string description;
    std::string expected;
    std::vector<std::string> devices;
  } tests[] = {
    {
      "one discharging battery",
      "battery discharging 10m 60% bat01 1",
      { "battery discharging 10m 60% bat01 1" }
    },
    {
      "merge two discharging batteries",
      "battery discharging 20m 70% nopath 1",
      { "battery discharging 10m 60% bat01 1", "battery discharging 20m 80% bat02 1" }
    },
    {
      "merge two other discharging batteries",
      "battery discharging 30m 90% nopath 1",
      { "battery discharging 20m 80% bat01 1", "battery discharging 30m 100% bat02 1" }
    },
    {
      "merge three discharging batteries",
      "battery discharging 30m 80% nopath 1",
      { "battery discharging 10m 60% bat01 1", "battery discharging 20m 80% bat02 1", "battery discharging 30m 100% bat03 1" }
    },
    {
      "one charging battery",
      "battery charging 10m 60% bat01 1",
      { "battery charging 10m 60% bat01 1" }
    },
    {
      "merge two charging batteries",
      "battery charging 20m 70% nopath 1",
      { "battery charging 10m 60% bat01 1", "battery charging 20m 80% bat02 1" }
    },
    {
      "merge two other charging batteries",
      "battery charging 30m 90% nopath 1",
      { "battery charging 20m 80% bat01 1", "battery charging 30m 100% bat02 1" }
    },
    {
      "merge three charging batteries",
      "battery charging 30m 80% nopath 1",
      { "battery charging 10m 60% bat01 1", "battery charging 20m 80% bat02 1", "battery charging 30m 100% bat03 1" }
    },
    {
      "one charged battery",
      "battery charged 0m 100% bat01 1",
      { "battery charged 0m 100% bat01 1" }
    },
    {
      "merge one charged, one discharging",
      "battery discharging 10m 80% nopath 1",
      { "battery charged 0m 100% bat01 1", "battery discharging 10m 60% bat02 1" }
    },
    {
      "merged one charged, one charging",
      "battery charging 10m 80% nopath 1",
      { "battery charged 0m 100% bat01 1", "battery charging 10m 60% bat02 1" }
    },
    {
      "merged one charged, one charging, one discharging",
      "battery discharging 10m 74% nopath 1",
      { "battery charged 0m 100% bat01 1", "battery charging 10m 60% bat02 1", "battery discharging 10m 60% bat03 1" }
    },
    {
      "one discharging mouse and one discharging battery. ignore mouse because it doesn't supply the power",
      "battery discharging 10m 60% bat01 1",
      { "battery discharging 10m 60% bat01 1", "mouse discharging 20m 80% mouse01 0" }
    },
    {
      "one discharging mouse and a different discharging battery. ignore mouse because it doesn't supply the power",
      "battery discharging 30m 100% bat01 1",
      { "battery discharging 30m 100% bat01 1", "mouse discharging 20m 80% mouse01 0" }
    },
    {
      "everything comes before power lines #1",
      "battery discharging 10m 60% bat01 1",
      { "battery discharging 10m 60% bat01 1", "line-power unknown 0m 0% lp01 1" }
    },
    {
      "everything comes before power lines #2",
      "battery charging 10m 60% bat01 1",
      { "battery charging 10m 60% bat01 1", "line-power unknown 0m 0% lp01 1" }
    },
    {
      "everything comes before power lines #3 except that the mouse doesn't supply the power",
      "line-power unknown 0m 0% lp01 1",
      { "mouse discharging 20m 80% mouse01 0", "line-power unknown 0m 0% lp01 1" }
    },
    {
      // https://bugs.launchpad.net/ubuntu/+source/indicator-power/+bug/1470080/comments/10
      "don't select a device with unknown state when we have another device with a known state...",
      "battery charged 0m 100% bat01 1",
      { "battery charged 0m 100% bat01 1", "phone unknown 0m 61% phone01 1" }
    },
    {
      // https://bugs.launchpad.net/ubuntu/+source/indicator-power/+bug/1470080/comments/10
      "...but do select the unknown state device if nothing else is available",
      "phone unknown 0m 61% phone01 1",
      { "phone unknown 0m 61% phone01 1" }
    }
  };
  
  for(const auto& test : tests)
  {
    // build the device list
    GList* device_glist {};
    for (const auto& description : test.devices)
        device_glist = g_list_append(device_glist, str2device(description));

    // run the test
    auto primary = indicator_power_service_choose_primary_device(device_glist);
    EXPECT_EQ(test.expected, device2str(primary));
    g_clear_object(&primary);

    // reverse the list and repeat the test
    // to confirm that list order doesn't matter
    device_glist = g_list_reverse(device_glist);
    primary = indicator_power_service_choose_primary_device(device_glist);
    EXPECT_EQ(test.expected, device2str(primary));
    g_clear_object(&primary);

    // cleanup
    g_list_free_full(device_glist, g_object_unref);
  }
}
