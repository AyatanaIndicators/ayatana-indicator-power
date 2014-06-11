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

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <cmath> // ceil()
#include "device.h"
#include "service.h"

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
      log_handler_id = g_log_set_handler ("Indicator-Power", flags, log_count_func, this);
      log_count_ipower_expected = 0;
      log_count_ipower_actual = 0;
    }

    virtual void TearDown()
    {
      ASSERT_EQ (log_count_ipower_expected, log_count_ipower_actual);
      g_log_remove_handler ("Indicator-Power", log_handler_id);
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

  // cleanup
  g_object_unref (device);
}

TEST_F(DeviceTest, NewFromVariant)
{
  GVariant * variant = g_variant_new ("(susdut)",
                                      "/object/path",
                                      (guint32) UP_DEVICE_KIND_BATTERY,
                                      "icon",
                                      (gdouble) 50.0,
                                      (guint32) UP_DEVICE_STATE_CHARGING,
                                      (guint64) 30);
  IndicatorPowerDevice * device = indicator_power_device_new_from_variant (variant);
  ASSERT_TRUE (variant != NULL);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_BATTERY);
  ASSERT_EQ (indicator_power_device_get_state(device), UP_DEVICE_STATE_CHARGING);
  ASSERT_STREQ (indicator_power_device_get_object_path(device), "/object/path");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

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
  check_icon_names (device, "ac-adapter-symbolic;"
                            "ac-adapter");

  // monitor
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_MONITOR,
                   NULL);
  check_icon_names (device, "gpm-monitor-symbolic;"
                            "gpm-monitor");

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
      check_icon_names (device, expected->str);
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
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 95%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                       NULL);

      g_string_append_printf (expected, "%s-100-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100-charging;", kind_str);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 85%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                       NULL);
      g_string_append_printf (expected, "%s-080-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080-charging;", kind_str);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 50%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                       NULL);
      g_string_append_printf (expected, "%s-060-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060-charging;", kind_str);
      g_string_append_printf (expected, "%s-good-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 25%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       NULL);
      g_string_append_printf (expected, "%s-020-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-020-charging;", kind_str);
      g_string_append_printf (expected, "%s-low-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 5%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       NULL);
      g_string_append_printf (expected, "%s-000-charging;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000-charging;", kind_str);
      g_string_append_printf (expected, "%s-caution-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution-charging", kind_str);
      check_icon_names (device, expected->str);
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
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 85%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                       NULL);
      g_string_append_printf (expected, "%s-080;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 50% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                       NULL);
      g_string_append_printf (expected, "%s-060;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 25% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                       NULL);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 25% -- 15 minutes left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*15),
                       NULL);
      g_string_append_printf (expected, "%s-020;", kind_str);
      g_string_append_printf (expected, "gpm-%s-020;", kind_str);
      g_string_append_printf (expected, "%s-low-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 5% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                   NULL);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 5% -- 15 minutes left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*15),
                       NULL);
      g_string_append_printf (expected, "%s-000;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-caution-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // state unknown
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN,
                       NULL);
      g_string_append_printf (expected, "%s-missing-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-missing;", kind_str);
      g_string_append_printf (expected, "%s-missing", kind_str);
      check_icon_names (device, expected->str);
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
  check_header (NULL, NULL, NULL, NULL, NULL);

  // bad args: a GObject that isn't a device
  GObject * o = G_OBJECT(g_cancellable_new());
  log_count_ipower_expected++;
  check_label ((IndicatorPowerDevice*)o, NULL);
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
  struct Description
  { 
    const char * path;
    UpDeviceKind kind;
    UpDeviceState state;
    guint64 time;
    double percentage;
  };

  const Description descriptions[] = {
    { "/some/path/d0", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   10,  60.0 }, // 0
    { "/some/path/d1", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   20,  80.0 }, // 1
    { "/some/path/d2", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   30, 100.0 }, // 2

    { "/some/path/c0", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      10,  60.0 }, // 3
    { "/some/path/c1", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      20,  80.0 }, // 4
    { "/some/path/c2", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      30, 100.0 }, // 5

    { "/some/path/f0", UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_FULLY_CHARGED,  0, 100.0 }, // 6
    { "/some/path/m0", UP_DEVICE_KIND_MOUSE,      UP_DEVICE_STATE_DISCHARGING,   20,  80.0 }, // 7
    { "/some/path/m1", UP_DEVICE_KIND_MOUSE,      UP_DEVICE_STATE_FULLY_CHARGED,  0, 100.0 }, // 8
    { "/some/path/pw", UP_DEVICE_KIND_LINE_POWER, UP_DEVICE_STATE_UNKNOWN,        0,   0.0 }  // 9
  };

  std::vector<IndicatorPowerDevice*> devices;
  for(const auto& desc : descriptions)
    devices.push_back(indicator_power_device_new(desc.path, desc.kind, desc.percentage, desc.state, desc.time));

  const struct {
    std::vector<int> device_indices;
    Description expected;
  } tests[] = {

    { { 0 }, descriptions[0] }, // 1 discharging
    { { 0, 1 },    { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_DISCHARGING, 20, 70.0 } }, // 2 discharging
    { { 1, 2 },    { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_DISCHARGING, 30, 90.0 } }, // 2 discharging
    { { 0, 1, 2 }, { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_DISCHARGING, 30, 80.0 } }, // 3 discharging

    { { 3 }, descriptions[3] }, // 1 charging
    { { 3, 4 },    { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_CHARGING, 20, 70.0 } }, // 2 charging
    { { 4, 5 },    { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_CHARGING, 30, 90.0 } }, // 2 charging
    { { 3, 4, 5 }, { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_CHARGING, 30, 80.0 } }, // 3 charging

    { { 6 }, descriptions[6] }, // 1 charged
    { { 6, 0 },    { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_DISCHARGING, 10, 80.0 } }, // 1 charged, 1 discharging
    { { 6, 3 },    { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_CHARGING,    10, 80.0 } }, // 1 charged, 1 charging
    { { 6, 0, 3 }, { nullptr, UP_DEVICE_KIND_BATTERY, UP_DEVICE_STATE_DISCHARGING, 10, 73.3 } }, // 1 charged, 1 charging, 1 discharging

    { { 0, 7 }, descriptions[0] }, // 1 discharging battery, 1 discharging mouse. pick the one with the least time left.
    { { 2, 7 }, descriptions[7] }, // 1 discharging battery, 1 discharging mouse. pick the one with the least time left.

    { { 0, 8 }, descriptions[0] }, // 1 discharging battery, 1 fully-charged mouse. pick the one that's discharging.
    { { 6, 7 }, descriptions[7] }, // 1 discharging mouse, 1 fully-charged battery. pick the one that's discharging.

    { { 0, 9 }, descriptions[0] }, // everything comes before power lines
    { { 3, 9 }, descriptions[3] },
    { { 7, 9 }, descriptions[7] }
  };
  
  for(const auto& test : tests)
  {
    const auto& x = test.expected;

    GList * device_glist = nullptr;
    for(const auto& i : test.device_indices)
      device_glist = g_list_append(device_glist, devices[i]);

    auto primary = indicator_power_service_choose_primary_device(device_glist);
    EXPECT_STREQ(x.path, indicator_power_device_get_object_path(primary));
    EXPECT_EQ(x.kind, indicator_power_device_get_kind(primary));
    EXPECT_EQ(x.state, indicator_power_device_get_state(primary));
    EXPECT_EQ(x.time, indicator_power_device_get_time(primary));
    EXPECT_EQ((int)ceil(x.percentage), (int)ceil(indicator_power_device_get_percentage(primary)));
    g_object_unref(primary);

    // reverse the list and repeat the test
    // to confirm that list order doesn't matter
    device_glist = g_list_reverse (device_glist);
    primary = indicator_power_service_choose_primary_device (device_glist);
    EXPECT_STREQ(x.path, indicator_power_device_get_object_path(primary));
    EXPECT_EQ(x.kind, indicator_power_device_get_kind(primary));
    EXPECT_EQ(x.state, indicator_power_device_get_state(primary));
    EXPECT_EQ(x.time, indicator_power_device_get_time(primary));
    EXPECT_EQ((int)ceil(x.percentage), (int)ceil(indicator_power_device_get_percentage(primary)));
    g_object_unref(primary);

    // cleanup
    g_list_free(device_glist);
  }

  for (auto& device : devices)
    g_object_unref (device);
}
