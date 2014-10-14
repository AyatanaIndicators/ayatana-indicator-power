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

#ifndef __INDICATOR_POWER_TESTING_H__
#define __INDICATOR_POWER_TESTING_H__

#include <gio/gio.h>

#include "service.h"

G_BEGIN_DECLS

/* standard GObject macros */
#define INDICATOR_POWER_TESTING(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), INDICATOR_TYPE_POWER_TESTING, IndicatorPowerTesting))
#define INDICATOR_TYPE_POWER_TESTING         (indicator_power_testing_get_type())
#define INDICATOR_IS_POWER_TESTING(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), INDICATOR_TYPE_POWER_TESTING))
#define INDICATOR_POWER_TESTING_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  INDICATOR_TYPE_POWER_TESTING, IndicatorPowerTestingClass))

typedef struct _IndicatorPowerTesting         IndicatorPowerTesting;
typedef struct _IndicatorPowerTestingClass    IndicatorPowerTestingClass;

/**
 * The Indicator Power Testing.
 */
struct _IndicatorPowerTesting
{
  /*< private >*/
  GObject parent;
};

struct _IndicatorPowerTestingClass
{
  GObjectClass parent_class;
};

/***
****
***/

GType indicator_power_testing_get_type (void);

IndicatorPowerTesting * indicator_power_testing_new (IndicatorPowerService * service);

void indicator_power_testing_set_bus (IndicatorPowerTesting * self,
                                      GDBusConnection       * connection);


G_END_DECLS

#endif /* __INDICATOR_POWER_TESTING_H__ */
