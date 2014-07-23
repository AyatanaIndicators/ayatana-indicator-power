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

#ifndef __INDICATOR_POWER_NOTIFIER_H__
#define __INDICATOR_POWER_NOTIFIER_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h> /* GDBusConnection */

#include "device.h"

G_BEGIN_DECLS

/* standard GObject macros */
#define INDICATOR_POWER_NOTIFIER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), INDICATOR_TYPE_POWER_NOTIFIER, IndicatorPowerNotifier))
#define INDICATOR_TYPE_POWER_NOTIFIER         (indicator_power_notifier_get_type())
#define INDICATOR_IS_POWER_NOTIFIER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), INDICATOR_TYPE_POWER_NOTIFIER))
#define INDICATOR_POWER_NOTIFIER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  INDICATOR_TYPE_POWER_NOTIFIER, IndicatorPowerNotifierClass))

typedef struct _IndicatorPowerNotifier         IndicatorPowerNotifier;
typedef struct _IndicatorPowerNotifierClass    IndicatorPowerNotifierClass;
typedef struct _IndicatorPowerNotifierPrivate  IndicatorPowerNotifierPrivate;

typedef enum
{
  POWER_LEVEL_OK,
  POWER_LEVEL_LOW,
  POWER_LEVEL_VERY_LOW,
  POWER_LEVEL_CRITICAL
}
PowerLevel;

/**
 * The Indicator Power Notifier.
 */
struct _IndicatorPowerNotifier
{
  /*< private >*/
  GObject parent;
  IndicatorPowerNotifierPrivate * priv;
};

struct _IndicatorPowerNotifierClass
{
  GObjectClass parent_class;

  /*< private >*/
  gint instance_count;
};

/***
****
***/

GType indicator_power_notifier_get_type (void);

IndicatorPowerNotifier * indicator_power_notifier_new (void);

void indicator_power_notifier_set_bus (IndicatorPowerNotifier  * self,
                                       GDBusConnection         * connection);

void indicator_power_notifier_set_battery (IndicatorPowerNotifier  * self,
                                           IndicatorPowerDevice    * battery);

PowerLevel indicator_power_notifier_get_power_level (IndicatorPowerDevice * battery);

G_END_DECLS

#endif /* __INDICATOR_POWER_NOTIFIER_H__ */
