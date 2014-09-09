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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include <cstdlib> 

#include <locale.h> // setlocale()
#include <libintl.h> // bindtextdomain()
#include <unistd.h> // STDIN_FILENO

#include <gio/gio.h>

#include "device-provider-mock.h"

#include "service.h"

/***
****
***/

static void
on_name_lost (gpointer instance G_GNUC_UNUSED, gpointer loop)
{
  g_message ("exiting: service couldn't acquire or lost ownership of busname");
  g_main_loop_quit ((GMainLoop*)loop);
}

static IndicatorPowerDevice * battery = nullptr;

static GMainLoop * loop = nullptr;

static gboolean on_command_stream_available (GIOChannel *source,
                                             GIOCondition /*condition*/,
                                             gpointer /*user_data*/)
{
  gchar * str = nullptr;
  GError * error = nullptr;
  auto status = g_io_channel_read_line (source, &str, nullptr, nullptr, &error);
  g_assert_no_error (error);

  if (status == G_IO_STATUS_NORMAL)
    {
      g_strstrip (str);

      if (!g_strcmp0 (str, "charging"))
        {
          g_object_set (battery, INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING, nullptr);
        }
      else if (!g_strcmp0 (str, "discharging"))
        {
          g_object_set (battery, INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING, nullptr);
        }
      else
        {
          g_object_set (battery, INDICATOR_POWER_DEVICE_PERCENTAGE, atof(str), nullptr);
        }
    }
  else if (status == G_IO_STATUS_EOF)
    {
      g_main_loop_quit (loop);
    }

  g_free (str);
  return G_SOURCE_CONTINUE;
}

/* this is basically indicator-power-service with a custom provider */
int
main (int argc G_GNUC_UNUSED, char ** argv G_GNUC_UNUSED)
{
  g_message ("This app is basically the same as indicator-power-service but,\n"
             "instead of the system's real devices, sees a single fake battery\n"
             "which can be manipulated by typing commands:\n"
             "'charging', 'discharging', a charge percentage, or ctrl-c.");

  IndicatorPowerDeviceProvider * device_provider;
  IndicatorPowerService * service;

  g_assert(g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIR, true));
  g_assert(g_setenv("GSETTINGS_BACKEND", "memory", true));

  /* boilerplate i18n */
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  /* read lines from the command line */
  auto channel = g_io_channel_unix_new (STDIN_FILENO);
  auto watch_tag = g_io_add_watch (channel, G_IO_IN, on_command_stream_available, nullptr);

  /* run */
  battery = indicator_power_device_new ("/some/path", UP_DEVICE_KIND_BATTERY, 50.0, UP_DEVICE_STATE_DISCHARGING, 30*60);
  device_provider = indicator_power_device_provider_mock_new ();
  indicator_power_device_provider_add_device (INDICATOR_POWER_DEVICE_PROVIDER_MOCK(device_provider), battery);
  service = indicator_power_service_new (device_provider);
  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (service, INDICATOR_POWER_SERVICE_SIGNAL_NAME_LOST,
                    G_CALLBACK(on_name_lost), loop);
  g_main_loop_run (loop);

  /* cleanup */
  g_main_loop_unref (loop);
  g_source_remove (watch_tag);
  g_io_channel_unref (channel);
  g_clear_object (&service);
  g_clear_object (&device_provider);
  g_clear_object (&battery);
  return 0;
}
