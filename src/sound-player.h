/*
 * Copyright 2016 Canonical Ltd.
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

#ifndef __INDICATOR_POWER_SOUND_PLAYER__H__
#define __INDICATOR_POWER_SOUND_PLAYER__H__

#include <glib-object.h>

G_BEGIN_DECLS

#define INDICATOR_TYPE_POWER_SOUND_PLAYER \
  (indicator_power_sound_player_get_type ())

#define INDICATOR_POWER_SOUND_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               INDICATOR_TYPE_POWER_SOUND_PLAYER, \
                               IndicatorPowerSoundPlayer))

#define INDICATOR_IS_POWER_SOUND_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_TYPE_POWER_SOUND_PLAYER))

#define INDICATOR_POWER_SOUND_PLAYER_GET_INTERFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), \
                                  INDICATOR_TYPE_POWER_SOUND_PLAYER, \
                                  IndicatorPowerSoundPlayerInterface))

typedef struct _IndicatorPowerSoundPlayer
                IndicatorPowerSoundPlayer;

typedef struct _IndicatorPowerSoundPlayerInterface
                IndicatorPowerSoundPlayerInterface;

/**
 * An interface class for an object that plays sounds.
 */
struct _IndicatorPowerSoundPlayerInterface
{
  GTypeInterface parent_iface;

  /* virtual functions */
  void (*play_uri) (IndicatorPowerSoundPlayer * self,
                    const gchar * uri);
};

GType indicator_power_sound_player_get_type (void);

/***
****
***/

void indicator_power_sound_player_play_uri (IndicatorPowerSoundPlayer * self,
                                            const gchar * uri);

G_END_DECLS

#endif /* __INDICATOR_POWER_SOUND_PLAYER__H__ */
