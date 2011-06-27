/*
An indicator to power related information in the menubar.

Copyright 2011 Codethink Ltd.

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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <libdbusmenu-gtk/menuitem.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/menuitem.h>

#include <libindicator/indicator-service.h>

#include "dbus-shared-names.h"

static IndicatorService *service = NULL;
static GMainLoop        *mainloop = NULL;
static DbusmenuServer   *server = NULL;
/*TODO Do we need this?*/
/*static PowerServiceDbus *dbus_interface = NULL;*/

/* Repsonds to the service object saying it's time to shutdown.
   It stops the mainloop. */
static void
service_shutdown (IndicatorService *service,
                  gpointer          user_data)
{
  g_warning("Shutting down service!");

  g_main_loop_quit(mainloop);
}

static void
spawn_on_activate_cb (DbusmenuMenuitem *mi,
                      guint             timestamp,
                      gpointer          user_data)
{
  GError * error = NULL;

  if (!g_spawn_command_line_async (user_data, &error))
  {
    g_warning ("Unable to start %s: %s", (char *)user_data, error->message);
    g_error_free (error);
  }
}

static void
build_menus (DbusmenuMenuitem *root_menuitem)
{
  DbusmenuMenuitem *power_mi = NULL;
  DbusmenuMenuitem *separator_mi = NULL;
  DbusmenuMenuitem *settings_mi = NULL;

  power_mi = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (power_mi,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  _("Laptop Battery 0:25")); /*TODO*/
  g_signal_connect (G_OBJECT (power_mi),
                    DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (spawn_on_activate_cb),
                    "gnome-power-statistic");

  settings_mi = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set (settings_mi,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  _("Power Settings..."));
  g_signal_connect (G_OBJECT (power_mi),
                    DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (spawn_on_activate_cb),
                    "gnome-power-preferences");

  separator_mi = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (separator_mi,
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_CLIENT_TYPES_SEPARATOR);


  dbusmenu_menuitem_child_append (root_menuitem, power_mi);
  dbusmenu_menuitem_child_append (root_menuitem, separator_mi);
  dbusmenu_menuitem_child_append (root_menuitem, settings_mi);
}

gint
main (gint    argc,
      gchar **argv)
{
  DbusmenuMenuitem *root_menuitem = NULL;

  g_type_init();

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
  setlocale (LC_ALL, "");

  /* Acknowledging the service init and setting up the interface */
  service = indicator_service_new_version (INDICATOR_POWER_DBUS_NAME,
                                           INDICATOR_POWER_DBUS_VERSION);
  g_signal_connect (service,
                    INDICATOR_SERVICE_SIGNAL_SHUTDOWN,
                    G_CALLBACK (service_shutdown),
                    NULL);

  /* Building the base menu */
  server = dbusmenu_server_new (INDICATOR_POWER_DBUS_OBJECT);
  root_menuitem = dbusmenu_menuitem_new ();
  dbusmenu_server_set_root (server, root_menuitem);

  build_menus (root_menuitem);

  /* Setup dbus interface */
  /*TODO*/
  /*dbus_interface = g_object_new (POWER_SERVICE_DBUS_TYPE, NULL);*/

  mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run(mainloop);

  g_object_unref (G_OBJECT (service));
  g_object_unref (G_OBJECT (server));
  g_object_unref (G_OBJECT (root_menuitem));

  return 0;
}
