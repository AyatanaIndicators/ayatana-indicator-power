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

#ifndef INDICATOR_POWER_BRIGHTNESS__H
#define INDICATOR_POWER_BRIGHTNESS__H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* standard GObject macros */
#define INDICATOR_POWER_BRIGHTNESS(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), INDICATOR_TYPE_POWER_BRIGHTNESS, IndicatorPowerBrightness))
#define INDICATOR_TYPE_POWER_BRIGHTNESS          (indicator_power_brightness_get_type())
#define INDICATOR_IS_POWER_BRIGHTNESS(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), INDICATOR_TYPE_POWER_BRIGHTNESS))

typedef struct _IndicatorPowerBrightness         IndicatorPowerBrightness;
typedef struct _IndicatorPowerBrightnessClass    IndicatorPowerBrightnessClass;

/* property keys */
#define INDICATOR_POWER_BRIGHTNESS_PROP_PERCENTAGE  "percentage"

/**
 * The Indicator Power Brightness.
 */
struct _IndicatorPowerBrightness
{
  /*< private >*/
  GObject parent;
};

struct _IndicatorPowerBrightnessClass
{
  GObjectClass parent_class;
};

/***
****
***/

GType indicator_power_brightness_get_type(void);

IndicatorPowerBrightness * indicator_power_brightness_new(void);

void indicator_power_brightness_set_percentage(IndicatorPowerBrightness * self, double percentage);

double indicator_power_brightness_get_percentage(IndicatorPowerBrightness * self);

G_END_DECLS

#endif /* INDICATOR_POWER_BRIGHTNESS__H */
