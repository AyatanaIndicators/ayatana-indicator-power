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

#ifndef __INDICATOR_POWER_DEVICE_PROVIDER_UPOWER__H__
#define __INDICATOR_POWER_DEVICE_PROVIDER_UPOWER__H__

#include <glib-object.h> /* parent class */

#include "device-provider.h"

G_BEGIN_DECLS

#define INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER \
  (indicator_power_device_provider_upower_get_type())

#define INDICATOR_POWER_DEVICE_PROVIDER_UPOWER(o) \
  (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                               INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER, \
                               IndicatorPowerDeviceProviderUPower))

#define INDICATOR_POWER_DEVICE_PROVIDER_UPOWER_GET_CLASS(o) \
 (G_TYPE_INSTANCE_GET_CLASS ((o), \
                             INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER, \
                             IndicatorPowerDeviceProviderUPowerClass))

#define INDICATOR_IS_POWER_DEVICE_PROVIDER_UPOWER(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                               INDICATOR_TYPE_POWER_DEVICE_PROVIDER_UPOWER))

typedef struct _IndicatorPowerDeviceProviderUPower
                IndicatorPowerDeviceProviderUPower;
typedef struct _IndicatorPowerDeviceProviderUPowerClass
                IndicatorPowerDeviceProviderUPowerClass;

/**
 * An IndicatorPowerDeviceProvider which gets its devices from UPower.
 */
struct _IndicatorPowerDeviceProviderUPower
{
  GObject parent_instance;
};

struct _IndicatorPowerDeviceProviderUPowerClass
{
  GObjectClass parent_class;
};

GType indicator_power_device_provider_upower_get_type (void);

IndicatorPowerDeviceProvider * indicator_power_device_provider_upower_new (void);

G_END_DECLS

#endif /* __INDICATOR_POWER_DEVICE_PROVIDER_UPOWER__H__ */
