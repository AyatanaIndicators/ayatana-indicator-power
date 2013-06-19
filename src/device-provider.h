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

#ifndef __INDICATOR_POWER_DEVICE_PROVIDER__H__
#define __INDICATOR_POWER_DEVICE_PROVIDER__H__

#include <glib-object.h>

G_BEGIN_DECLS

#define INDICATOR_TYPE_POWER_DEVICE_PROVIDER \
  (indicator_power_device_provider_get_type ())

#define INDICATOR_POWER_DEVICE_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               INDICATOR_TYPE_POWER_DEVICE_PROVIDER, \
                               IndicatorPowerDeviceProvider))

#define INDICATOR_IS_POWER_DEVICE_PROVIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_TYPE_POWER_DEVICE_PROVIDER))

#define INDICATOR_POWER_DEVICE_PROVIDER_GET_INTERFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), \
                                  INDICATOR_TYPE_POWER_DEVICE_PROVIDER, \
                                  IndicatorPowerDeviceProviderInterface))

typedef struct _IndicatorPowerDeviceProvider
                IndicatorPowerDeviceProvider;

typedef struct _IndicatorPowerDeviceProviderInterface
                IndicatorPowerDeviceProviderInterface;

/**
 * An interface class for an object that provides IndicatorPowerDevices.
 *
 * Example uses:
 *  - in unit tests, a mock that feeds fake devices to the service
 *  - in production, an implementation that monitors upower
 *  - in the future, upower can be replaced by changing providers
 */
struct _IndicatorPowerDeviceProviderInterface
{
  GTypeInterface parent_iface;

  /* signals */
  void (*devices_changed) (IndicatorPowerDeviceProvider * self);

  /* virtual functions */
  GList* (*get_devices) (IndicatorPowerDeviceProvider * self);
};

GType indicator_power_device_provider_get_type (void);

/***
****
***/

GList * indicator_power_device_provider_get_devices          (IndicatorPowerDeviceProvider * self);

void    indicator_power_device_provider_emit_devices_changed (IndicatorPowerDeviceProvider * self);

G_END_DECLS

#endif /* __INDICATOR_POWER_DEVICE_PROVIDER__H__ */
