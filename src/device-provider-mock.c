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

#include "device.h"
#include "device-provider.h"
#include "device-provider-mock.h"

/***
****  GObject boilerplate
***/

static void indicator_power_device_provider_interface_init (
                                IndicatorPowerDeviceProviderInterface * iface);

G_DEFINE_TYPE_WITH_CODE (
  IndicatorPowerDeviceProviderMock,
  indicator_power_device_provider_mock,
  G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (INDICATOR_TYPE_POWER_DEVICE_PROVIDER,
                         indicator_power_device_provider_interface_init))

/***
****  IndicatorPowerDeviceProvider virtual functions
***/

static GList *
my_get_devices (IndicatorPowerDeviceProvider * provider)
{
  IndicatorPowerDeviceProviderMock * self = INDICATOR_POWER_DEVICE_PROVIDER_MOCK(provider);

  return g_list_copy_deep (self->devices, (GCopyFunc)g_object_ref, NULL);
}

/***
****  GObject virtual functions
***/

static void
my_dispose (GObject * o)
{
  IndicatorPowerDeviceProviderMock * self = INDICATOR_POWER_DEVICE_PROVIDER_MOCK(o);

  g_list_free_full (self->devices, g_object_unref);

  G_OBJECT_CLASS (indicator_power_device_provider_mock_parent_class)->dispose (o);
}

/***
****  Instantiation
***/

static void
indicator_power_device_provider_mock_class_init (IndicatorPowerDeviceProviderMockClass * klass)
{
  GObjectClass * object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = my_dispose;
}

static void
indicator_power_device_provider_interface_init (IndicatorPowerDeviceProviderInterface * iface)
{
  iface->get_devices = my_get_devices;
}

static void
indicator_power_device_provider_mock_init (IndicatorPowerDeviceProviderMock * self G_GNUC_UNUSED)
{
}

/***
****  Public API
***/

IndicatorPowerDeviceProvider *
indicator_power_device_provider_mock_new (void)
{
  gpointer o = g_object_new (INDICATOR_TYPE_POWER_DEVICE_PROVIDER_MOCK, NULL);

  return INDICATOR_POWER_DEVICE_PROVIDER (o);
}

void
indicator_power_device_provider_add_device (IndicatorPowerDeviceProviderMock * provider,
                                            IndicatorPowerDevice             * device)
{
  provider->devices = g_list_append (provider->devices, g_object_ref(device));

  g_signal_connect_swapped (device, "notify", G_CALLBACK(indicator_power_device_provider_emit_devices_changed), provider);
}
