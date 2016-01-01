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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "sound-player.h"
#include "sound-player-mock.h"

enum
{
  SIGNAL_URI_PLAYED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/***
****  GObject boilerplate
***/

static void indicator_power_sound_player_interface_init (
                                IndicatorPowerSoundPlayerInterface * iface);

G_DEFINE_TYPE_WITH_CODE (
  IndicatorPowerSoundPlayerMock,
  indicator_power_sound_player_mock,
  G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (INDICATOR_TYPE_POWER_SOUND_PLAYER,
                         indicator_power_sound_player_interface_init))

/***
****  IndicatorPowerSoundPlayer virtual functions
***/

static void
my_play_uri (IndicatorPowerSoundPlayer * self, const gchar * uri)
{
  g_signal_emit (self, signals[SIGNAL_URI_PLAYED], 0, uri, NULL);
}

/***
****  Instantiation
***/

static void
indicator_power_sound_player_mock_class_init (IndicatorPowerSoundPlayerMockClass * klass G_GNUC_UNUSED)
{
  signals[SIGNAL_URI_PLAYED] = g_signal_new (
    "uri-played",
    G_TYPE_FROM_CLASS(klass),
    G_SIGNAL_RUN_LAST,
    G_STRUCT_OFFSET (IndicatorPowerSoundPlayerMockClass, uri_played),
    NULL, NULL,
    g_cclosure_marshal_VOID__STRING,
    G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
indicator_power_sound_player_interface_init (IndicatorPowerSoundPlayerInterface * iface)
{
  iface->play_uri = my_play_uri;
}

static void
indicator_power_sound_player_mock_init (IndicatorPowerSoundPlayerMock * self G_GNUC_UNUSED)
{
}

/***
****  Public API
***/

IndicatorPowerSoundPlayer *
indicator_power_sound_player_mock_new (void)
{
  gpointer o = g_object_new (INDICATOR_TYPE_POWER_SOUND_PLAYER_MOCK, NULL);

  return INDICATOR_POWER_SOUND_PLAYER (o);
}

