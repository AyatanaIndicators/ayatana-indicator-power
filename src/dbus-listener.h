/*

Listens for Power changes from org.gnome.SettingsDaemon.Power on Dbus

Copyright 2012 Canonical Ltd.

Authors:
    Javier Jardon <javier.jardon@codethink.co.uk>
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

#ifndef __INDICATOR_POWER_DBUS_LISTENER_H__
#define __INDICATOR_POWER_DBUS_LISTENER_H__

#include <glib-object.h>
#include <libupower-glib/upower.h>

G_BEGIN_DECLS

#define INDICATOR_POWER_DBUS_LISTENER_TYPE            (indicator_power_dbus_listener_get_type ())
#define INDICATOR_POWER_DBUS_LISTENER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_POWER_DBUS_LISTENER_TYPE, IndicatorPowerDbusListener))
#define INDICATOR_POWER_DBUS_LISTENER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  INDICATOR_POWER_DBUS_LISTENER_TYPE, IndicatorPowerDbusListenerClass))
#define INDICATOR_IS_POWER_DBUS_LISTENER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_POWER_DBUS_LISTENER_TYPE))
#define INDICATOR_IS_POWER_DBUS_LISTENER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  INDICATOR_POWER_DBUS_LISTENER_TYPE))
#define INDICATOR_POWER_DBUS_LISTENER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  INDICATOR_POWER_DBUS_LISTENER_TYPE, IndicatorPowerDbusListenerClass))

typedef struct _IndicatorPowerDbusListener IndicatorPowerDbusListener;
typedef struct _IndicatorPowerDbusListenerClass IndicatorPowerDbusListenerClass;
typedef struct _IndicatorPowerDbusListenerPrivate IndicatorPowerDbusListenerPrivate;

#define GSD_SERVICE               "org.gnome.SettingsDaemon"
#define GSD_PATH                  "/org/gnome/SettingsDaemon"
#define GSD_POWER_DBUS_INTERFACE  GSD_SERVICE ".Power"
#define GSD_POWER_DBUS_PATH       GSD_PATH "/Power"

/* signals */
#define INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED  "devices-enumerated"

/**
 * IndicatorPowerDbusListenerClass:
 * @parent_class: #GObjectClass
 */
struct _IndicatorPowerDbusListenerClass
{
  GObjectClass parent_class;

  void (* devices_enumerated) (IndicatorPowerDbusListener*, GSList * devices);
};

/**
 * IndicatorPowerDbusListener:
 * @parent: #GObject
 * @priv: A cached reference to the private data for the instance.
*/
struct _IndicatorPowerDbusListener
{
  GObject parent;
  IndicatorPowerDbusListenerPrivate * priv;
};

/***
****
***/

GType indicator_power_dbus_listener_get_type (void);

IndicatorPowerDbusListener* indicator_power_dbus_listener_new (void);


G_END_DECLS

#endif
