/*
 * Copyright 2013-2016 Canonical Ltd.
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

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "device.h"
#include "notifier.h"
#include "service.h"
#include "sound-player-gst.h"
#include "testing.h"

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
  IndicatorPowerSoundPlayer * sound_player;
  IndicatorPowerNotifier * notifier;
  IndicatorPowerService * service;
  IndicatorPowerTesting * testing;
  GMainLoop * loop;

  /* boilerplate i18n */
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  /* run */
  sound_player = indicator_power_sound_player_gst_new ();
  notifier = indicator_power_notifier_new (sound_player);
  service = indicator_power_service_new(NULL, notifier);
  testing = indicator_power_testing_new (service);
  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (service, INDICATOR_POWER_SERVICE_SIGNAL_NAME_LOST,
                    G_CALLBACK(on_name_lost), loop);
  g_main_loop_run (loop);

  /* cleanup */
  g_main_loop_unref (loop);
  g_clear_object (&testing);
  g_clear_object (&service);
  g_clear_object (&notifier);
  g_clear_object (&sound_player);
  return 0;
}
