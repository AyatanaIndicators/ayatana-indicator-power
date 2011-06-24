/*
An indicator to power related information in the menubar.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* GStuff */
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>

/* DBusMenu */
#include <libido/libido.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <libdbusmenu-gtk3/menu.h>
#include <libdbusmenu-gtk3/menuitem.h>
#else
#include <libdbusmenu-gtk/menu.h>
#include <libdbusmenu-gtk/menuitem.h>
#endif
