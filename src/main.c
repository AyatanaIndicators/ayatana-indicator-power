/*
 * Copyright 2013 Canonical Ltd.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
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
 */

#include <locale.h>
#include <stdlib.h> /* exit() */

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "device.h"
#include "device-provider-mock.h"
#include "device-provider-upower.h"
#include "service.h"

/***
****  MOCK
***/

static IndicatorPowerDevice*
get_mock_battery(GSettings * settings)
{
  const double percent = g_settings_get_int(settings, "mock-battery-level");

  const UpDeviceState state = g_settings_get_boolean(settings, "mock-battery-charging")
                            ? UP_DEVICE_STATE_CHARGING
                            : UP_DEVICE_STATE_DISCHARGING;

  const int seconds_left = g_settings_get_int(settings, "mock-battery-minutes-left") * 60;

  return indicator_power_device_new ("/some/path",
                                     UP_DEVICE_KIND_BATTERY,
                                     percent,
                                     state,
                                     seconds_left);
}

static IndicatorPowerDeviceProvider*
get_device_provider(GSettings * settings)
{
  IndicatorPowerDeviceProvider * provider = NULL;

  if (g_settings_get_boolean(settings, "mock-battery-enabled"))
    {
      IndicatorPowerDevice * battery = get_mock_battery(settings);
      provider = indicator_power_device_provider_mock_new ();
      indicator_power_device_provider_add_device (INDICATOR_POWER_DEVICE_PROVIDER_MOCK(provider), battery);
      g_object_unref (battery);
    }
  else
    {
      provider = indicator_power_device_provider_upower_new ();
    }

  return provider;
}

static void
on_mock_settings_changed(GSettings* settings,
                         gchar    * key       G_GNUC_UNUSED,
                         gpointer   service)
{
  IndicatorPowerDeviceProvider* provider = get_device_provider(settings);
  indicator_power_service_set_device_provider (INDICATOR_POWER_SERVICE(service), provider);
  g_object_unref(provider);
}

/***
****
***/

static void
on_name_lost (gpointer instance G_GNUC_UNUSED, gpointer loop)
{
  g_message ("exiting: service couldn't acquire or lost ownership of busname");
  g_main_loop_quit ((GMainLoop*)loop);
}

int
main (int argc G_GNUC_UNUSED, char ** argv G_GNUC_UNUSED)
{
  GSettings * settings;
  GMainLoop * loop;
  IndicatorPowerService * service;

  /* boilerplate i18n */
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  /* mock settings */
  service = indicator_power_service_new (NULL);
  settings = g_settings_new ("com.canonical.indicator.power");
  g_signal_connect(settings, "changed::mock-battery-enabled", G_CALLBACK(on_mock_settings_changed), service);
  g_signal_connect(settings, "changed::mock-battery-level", G_CALLBACK(on_mock_settings_changed), service);
  g_signal_connect(settings, "changed::mock-battery-charging", G_CALLBACK(on_mock_settings_changed), service);
  g_signal_connect(settings, "changed::mock-battery-minutes-left", G_CALLBACK(on_mock_settings_changed), service);
  on_mock_settings_changed(settings, NULL, service);

  /* run */
  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (service, INDICATOR_POWER_SERVICE_SIGNAL_NAME_LOST,
                    G_CALLBACK(on_name_lost), loop);
  g_main_loop_run (loop);

  /* cleanup */
  g_main_loop_unref (loop);
  g_clear_object (&service);
  g_clear_object (&settings);
  return 0;
}
