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

#include "device-provider.h"

enum
{
  SIGNAL_DEVICES_CHANGED,
  SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

G_DEFINE_INTERFACE (IndicatorPowerDeviceProvider,
                    indicator_power_device_provider,
                    0);

static void
indicator_power_device_provider_default_init (IndicatorPowerDeviceProviderInterface * klass)
{
  signals[SIGNAL_DEVICES_CHANGED] = g_signal_new (
      "devices-changed",
      G_TYPE_FROM_CLASS(klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (IndicatorPowerDeviceProviderInterface, devices_changed),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

/***
****  PUBLIC API
***/

/**
 * Get a list of devices
 *
 * An easy way to free the list properly in one step is as follows:
 *
 *   g_slist_free_full (list, (GDestroyNotify)g_object_unref);
 *
 * Return value: (element-type IndicatorPowerDevice)
 *               (transfer full):
 *               list of devices
 */
GList *
indicator_power_device_provider_get_devices (IndicatorPowerDeviceProvider * self)
{
  GList * devices;
  IndicatorPowerDeviceProviderInterface * iface;

  g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE_PROVIDER (self), NULL);
  iface = INDICATOR_POWER_DEVICE_PROVIDER_GET_INTERFACE (self);

  if (iface->get_devices != NULL)
    devices = iface->get_devices (self);
  else
    devices = NULL;

  return devices;
}

/**
 * Emits the "devices-changed" signal.
 *
 * This should only be called by subclasses.
 */
void
indicator_power_device_provider_emit_devices_changed (IndicatorPowerDeviceProvider * self)
{
  g_return_if_fail (INDICATOR_IS_POWER_DEVICE_PROVIDER (self));

  g_signal_emit (self, signals[SIGNAL_DEVICES_CHANGED], 0, NULL);
}
