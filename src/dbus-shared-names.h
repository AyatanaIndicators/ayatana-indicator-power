/*
A small wrapper utility to load indicators and put them as menu items
into the gnome-panel using it's applet interface.

Copyright 2010 Codethink Ltd.

Authors:
    Javier Jardon <javier.jardon@codethink.co.uk>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __DBUS_SHARED_NAMES_H__
#define __DBUS_SHARED_NAMES_H__ 1

#define INDICATOR_POWER_DBUS_NAME              "com.canonical.indicator.power"
#define INDICATOR_POWER_DBUS_VERSION           1
#define INDICATOR_POWER_DBUS_OBJECT            "/com/canonical/indicator/power/menu"
#define INDICATOR_POWER_SERVICE_DBUS_OBJECT    "/com/canonical/indicator/power/service"
#define INDICATOR_POWER_SERVICE_DBUS_INTERFACE "com.canonical.indicator.power.service"


#endif /* __DBUS_SHARED_NAMES_H__ */

