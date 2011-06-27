/*
An indicator to time and date related information in the menubar.

Copyright 2011 Canonical Ltd.

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

#ifndef __POWER_SERVICE_DBUS_H__
#define __POWER_SERVICE_DBUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define POWER_SERVICE_DBUS_TYPE            (power_service_dbus_get_type ())
#define POWER_SERVICE_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), POWER_SERVICE_DBUS_TYPE, PowerServiceDbus))
#define POWER_SERVICE_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), POWER_SERVICE_DBUS_TYPE, PowerServiceDbusClass))
#define IS_POWER_SERVICE_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), POWER_SERVICE_DBUS_TYPE))
#define IS_POWER_SERVICE_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), POWER_SERVICE_DBUS_TYPE))
#define POWER_SERVICE_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), POWER_SERVICE_DBUS_TYPE, PowerServiceDbusClass))

typedef struct _PowerServiceDbus         PowerServiceDbus;
typedef struct _PowerServiceDbusClass    PowerServiceDbusClass;
typedef struct _PowerServiceDbusPrivate  PowerServiceDbusPrivate;

struct _PowerServiceDbus
{
  GObject parent_instance;

  PowerServiceDbusPrivate *priv;
};

struct _PowerServiceDbusClass
{
  GObjectClass parent_class;
};

GType power_service_dbus_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __POWER_SERVICE_DBUS_H__ */
