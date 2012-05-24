/*

A simple Device structure used internally by indicator-power

Copyright 2012 Canonical Ltd.

Authors:
    Charles Kerr <charles.kerr@canonical.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 3.0 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 3.0 for more details.

You should have received a copy of the GNU General Public
License along with this library. If not, see
<http://www.gnu.org/licenses/>.
*/

#ifndef __INDICATOR_POWER_DEVICE_H__
#define __INDICATOR_POWER_DEVICE_H__

#include <glib-object.h>
#include <libupower-glib/upower.h>

G_BEGIN_DECLS

#define INDICATOR_POWER_DEVICE_TYPE            (indicator_power_device_get_type ())
#define INDICATOR_POWER_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_POWER_DEVICE_TYPE, IndicatorPowerDevice))
#define INDICATOR_POWER_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  INDICATOR_POWER_DEVICE_TYPE, IndicatorPowerDeviceClass))
#define INDICATOR_IS_POWER_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_POWER_DEVICE_TYPE))
#define INDICATOR_IS_POWER_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  INDICATOR_POWER_DEVICE_TYPE))
#define INDICATOR_POWER_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  INDICATOR_POWER_DEVICE_TYPE, IndicatorPowerDeviceClass))

typedef struct _IndicatorPowerDevice IndicatorPowerDevice;
typedef struct _IndicatorPowerDeviceClass IndicatorPowerDeviceClass;
typedef struct _IndicatorPowerDevicePrivate IndicatorPowerDevicePrivate;

#define INDICATOR_POWER_DEVICE_KIND         "indicator-power-device-kind"
#define INDICATOR_POWER_DEVICE_STATE        "indicator-power-device-state"
#define INDICATOR_POWER_DEVICE_OBJECT_PATH  "indicator-power-device-object-path"
#define INDICATOR_POWER_DEVICE_ICON         "indicator-power-device-icon"
#define INDICATOR_POWER_DEVICE_PERCENTAGE   "indicator-power-device-percentage"
#define INDICATOR_POWER_DEVICE_TIME         "indicator-power-device-time"

/**
 * IndicatorPowerDeviceClass:
 * @parent_class: #GObjectClass
 */
struct _IndicatorPowerDeviceClass
{
	GObjectClass parent_class;
};

/**
 * IndicatorPowerDevice:
 * @parent: #GObject
 * @priv: A cached reference to the private data for the instance.
*/
struct _IndicatorPowerDevice
{
	GObject parent;
	IndicatorPowerDevicePrivate * priv;
};

/***
****
***/

GType indicator_power_device_get_type (void);

IndicatorPowerDevice* indicator_power_device_new (const gchar    * object_path,
                                                  UpDeviceKind     kind,
                                                  const gchar    * icon,
                                                  gdouble          percentage,
                                                  UpDeviceState    state,
                                                  time_t           time);

/**
 * Convenience wrapper around indicator_power_device_new()
 * @variant holds the same args as indicator_power_device_new() in "(susdut)"
 */
IndicatorPowerDevice* indicator_power_device_new_from_variant (GVariant * variant);


UpDeviceKind  indicator_power_device_get_kind        (const IndicatorPowerDevice * device);
UpDeviceState indicator_power_device_get_state       (const IndicatorPowerDevice * device);
const gchar * indicator_power_device_get_object_path (const IndicatorPowerDevice * device);
const gchar * indicator_power_device_get_icon        (const IndicatorPowerDevice * device);
gdouble       indicator_power_device_get_percentage  (const IndicatorPowerDevice * device);
time_t        indicator_power_device_get_time        (const IndicatorPowerDevice * device);


G_END_DECLS

#endif
