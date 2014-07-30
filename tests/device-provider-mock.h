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

#ifndef __INDICATOR_POWER_DEVICE_PROVIDER_MOCK__H__
#define __INDICATOR_POWER_DEVICE_PROVIDER_MOCK__H__

#include <glib-object.h> /* parent class */

#include "device.h"
#include "device-provider.h"

G_BEGIN_DECLS

#define INDICATOR_TYPE_POWER_DEVICE_PROVIDER_MOCK \
  (indicator_power_device_provider_mock_get_type())

#define INDICATOR_POWER_DEVICE_PROVIDER_MOCK(o) \
  (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                               INDICATOR_TYPE_POWER_DEVICE_PROVIDER_MOCK, \
                               IndicatorPowerDeviceProviderMock))

#define INDICATOR_POWER_DEVICE_PROVIDER_MOCK_GET_CLASS(o) \
 (G_TYPE_INSTANCE_GET_CLASS ((o), \
                             INDICATOR_TYPE_POWER_DEVICE_PROVIDER_MOCK, \
                             IndicatorPowerDeviceProviderMockClass))

#define INDICATOR_IS_POWER_DEVICE_PROVIDER_MOCK(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                               INDICATOR_TYPE_POWER_DEVICE_PROVIDER_MOCK))

typedef struct _IndicatorPowerDeviceProviderMock
                IndicatorPowerDeviceProviderMock;
typedef struct _IndicatorPowerDeviceProviderMockPriv
                IndicatorPowerDeviceProviderMockPriv;
typedef struct _IndicatorPowerDeviceProviderMockClass
                IndicatorPowerDeviceProviderMockClass;

/**
 * An IndicatorPowerDeviceProvider which gets its devices from Mock.
 */
struct _IndicatorPowerDeviceProviderMock
{
  GObject parent_instance;

  /*< private >*/
  GList * devices;
};

struct _IndicatorPowerDeviceProviderMockClass
{
  GObjectClass parent_class;
};

GType indicator_power_device_provider_mock_get_type (void);

IndicatorPowerDeviceProvider * indicator_power_device_provider_mock_new (void);

void indicator_power_device_provider_add_device (IndicatorPowerDeviceProviderMock * provider,
                                                 IndicatorPowerDevice             * device);

G_END_DECLS

#endif /* __INDICATOR_POWER_DEVICE_PROVIDER_MOCK__H__ */
