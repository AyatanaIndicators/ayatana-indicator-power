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

#ifndef __INDICATOR_POWER_SERVICE_H__
#define __INDICATOR_POWER_SERVICE_H__

#include <glib.h>
#include <glib-object.h>

#include "device-provider.h"
#include "notifier.h"

G_BEGIN_DECLS

/* standard GObject macros */
#define INDICATOR_POWER_SERVICE(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), INDICATOR_TYPE_POWER_SERVICE, IndicatorPowerService))
#define INDICATOR_TYPE_POWER_SERVICE          (indicator_power_service_get_type())
#define INDICATOR_IS_POWER_SERVICE(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), INDICATOR_TYPE_POWER_SERVICE))

typedef struct _IndicatorPowerService         IndicatorPowerService;
typedef struct _IndicatorPowerServiceClass    IndicatorPowerServiceClass;
typedef struct _IndicatorPowerServicePrivate  IndicatorPowerServicePrivate;

/* signal keys */
#define INDICATOR_POWER_SERVICE_SIGNAL_NAME_LOST   "name-lost"

/**
 * The Indicator Power Service.
 */
struct _IndicatorPowerService
{
  /*< private >*/
  GObject parent;
  IndicatorPowerServicePrivate * priv;
};

struct _IndicatorPowerServiceClass
{
  GObjectClass parent_class;

  /* signals */

  void (* name_lost)(IndicatorPowerService * self);
};

/***
****
***/

GType indicator_power_service_get_type (void);

IndicatorPowerService * indicator_power_service_new (IndicatorPowerDeviceProvider * provider,
                                                     IndicatorPowerNotifier       * notifier);

void indicator_power_service_set_device_provider (IndicatorPowerService        * self,
                                                  IndicatorPowerDeviceProvider * provider);

void indicator_power_service_set_notifier (IndicatorPowerService  * self,
                                           IndicatorPowerNotifier * notifier);

IndicatorPowerDevice * indicator_power_service_choose_primary_device (GList * devices);



G_END_DECLS

#endif /* __INDICATOR_POWER_SERVICE_H__ */
