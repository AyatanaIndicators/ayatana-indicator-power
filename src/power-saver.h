/*
 * Copyright © 2026 Volla Systeme GmbH.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 */

#ifndef INDICATOR_POWER_SAVER_H
#define INDICATOR_POWER_SAVER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define INDICATOR_TYPE_POWER_SAVER (indicator_power_saver_get_type())
#define INDICATOR_POWER_SAVER(o)   (G_TYPE_CHECK_INSTANCE_CAST((o), INDICATOR_TYPE_POWER_SAVER, IndicatorPowerSaver))
#define INDICATOR_IS_POWER_SAVER(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), INDICATOR_TYPE_POWER_SAVER))

typedef struct _IndicatorPowerSaver      IndicatorPowerSaver;
typedef struct _IndicatorPowerSaverClass IndicatorPowerSaverClass;
typedef struct _IndicatorPowerSaverPriv  IndicatorPowerSaverPriv;

struct _IndicatorPowerSaver
{
  GObject parent;
  IndicatorPowerSaverPriv * priv;
};

struct _IndicatorPowerSaverClass
{
  GObjectClass parent_class;
};

GType indicator_power_saver_get_type(void);

IndicatorPowerSaver * indicator_power_saver_new(void);

gboolean indicator_power_saver_service_available(IndicatorPowerSaver * self);
gboolean indicator_power_saver_get_battery_saver(IndicatorPowerSaver * self);
void     indicator_power_saver_set_battery_saver(IndicatorPowerSaver * self, gboolean enabled);

G_END_DECLS

#endif /* INDICATOR_POWER_SAVER_H */
