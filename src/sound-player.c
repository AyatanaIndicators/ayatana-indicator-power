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

#include "sound-player.h"

G_DEFINE_INTERFACE (IndicatorPowerSoundPlayer,
                    indicator_power_sound_player,
                    0)

static void
indicator_power_sound_player_default_init (IndicatorPowerSoundPlayerInterface * klass G_GNUC_UNUSED)
{
}

/***
****  PUBLIC API
***/

void
indicator_power_sound_player_play_uri (IndicatorPowerSoundPlayer * self,
                                       const gchar               * uri)
{
  IndicatorPowerSoundPlayerInterface * iface;

  g_return_if_fail (INDICATOR_IS_POWER_SOUND_PLAYER (self));
  iface = INDICATOR_POWER_SOUND_PLAYER_GET_INTERFACE (self);
  g_return_if_fail (iface->play_uri != NULL);
  iface->play_uri (self, uri);
}
