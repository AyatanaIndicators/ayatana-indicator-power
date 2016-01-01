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

#ifndef __INDICATOR_POWER_SOUND_PLAYER_GST__H__
#define __INDICATOR_POWER_SOUND_PLAYER_GST__H__

#include <glib-object.h> /* parent class */

#include "device-provider.h"

G_BEGIN_DECLS

#define INDICATOR_TYPE_POWER_SOUND_PLAYER_GST \
  (indicator_power_sound_player_gst_get_type())

#define INDICATOR_POWER_SOUND_PLAYER_GST(o) \
  (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                               INDICATOR_TYPE_POWER_SOUND_PLAYER_GST, \
                               IndicatorPowerSoundPlayerGST))

#define INDICATOR_POWER_SOUND_PLAYER_GST_GET_CLASS(o) \
 (G_TYPE_INSTANCE_GET_CLASS ((o), \
                             INDICATOR_TYPE_POWER_SOUND_PLAYER_GST, \
                             IndicatorPowerSoundPlayerGSTClass))

#define INDICATOR_IS_POWER_SOUND_PLAYER_GST(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                               INDICATOR_TYPE_POWER_SOUND_PLAYER_GST))

typedef struct _IndicatorPowerSoundPlayerGST
                IndicatorPowerSoundPlayerGST;
typedef struct _IndicatorPowerSoundPlayerGSTClass
                IndicatorPowerSoundPlayerGSTClass;

/**
 * An IndicatorPowerSoundPlayer which gets its devices from GST.
 */
struct _IndicatorPowerSoundPlayerGST
{
  GObject parent_instance;
};

struct _IndicatorPowerSoundPlayerGSTClass
{
  GObjectClass parent_class;
};

GType indicator_power_sound_player_gst_get_type (void);

IndicatorPowerSoundPlayer * indicator_power_sound_player_gst_new (void);

G_END_DECLS

#endif /* __INDICATOR_POWER_SOUND_PLAYER_GST__H__ */
