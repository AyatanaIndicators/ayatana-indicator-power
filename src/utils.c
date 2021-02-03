/*
 * Copyright 2013 Canonical Ltd.
 * Copytight 2018 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 *
 * Authors (@Canonical):
 *   Charles Kerr <charles.kerr@canonical.com>
 *   Ted Gould <ted@canonical.com>
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

#include "utils.h"

#ifdef HAS_URLDISPATCHER
# include <lomiri-url-dispatcher.h>
#endif

/* Run a particular program based on an activation */
void
execute_command (const gchar * cmd)
{
  GError * err = NULL;

  g_debug ("Issuing command '%s'", cmd);

  if (!g_spawn_command_line_async (cmd, &err))
    {
      g_warning ("Unable to start %s: %s", cmd, err->message);
      g_error_free (err);
    }
}

gboolean
zenity_warning (const char * icon_name,
                const char * title,
                const char * text)
{
  char * command_line;
  int exit_status;
  GError * error;
  gboolean confirmed;
  char * zenity;

  confirmed = FALSE;
  zenity = g_find_program_in_path ("zenity");

  if (zenity)
    {
      command_line = g_strdup_printf ("%s"
                                      " --warning"
                                      " --icon-name=\"%s\""
                                      " --title=\"%s\""
                                      " --text=\"%s\""
                                      " --no-wrap",
                                      zenity,
                                      icon_name,
                                      title,
                                      text);

      /* Treat errors as user confirmation.
         Otherwise how will the user ever log out? */
      exit_status = -1;
      error = NULL;
      if (!g_spawn_command_line_sync (command_line, NULL, NULL, &exit_status, &error))
        {
          confirmed = TRUE;
        }
      else
        {
          confirmed = g_spawn_check_exit_status (exit_status, &error);
        }

      g_free (command_line);
    }
  g_free (zenity);
  return confirmed;
}

void
utils_handle_settings_request (void)
{
  static const gchar *control_center_cmd = NULL;

  if (control_center_cmd == NULL)
    {
#ifdef HAS_URLDISPATCHER
      if (g_getenv ("MIR_SOCKET") != NULL)
        {
          lomiri_url_dispatch_send("settings:///system/battery", NULL, NULL);
          return;
        }
      else
#endif
      /* XFCE does not set XDG_CURRENT_DESKTOP, it seems... */
      if (is_xfce())
        {
          control_center_cmd = "xfce4-power-manager-settings";
        }
      else if (is_mate())
        {
          control_center_cmd = "mate-power-preferences";
        }
      else if (is_pantheon())
        {
          control_center_cmd = "switchboard --open-plug system-pantheon-power";
        }
      else if (is_unity() || is_gnome())
        {
          gchar *path;

          path = g_find_program_in_path ("unity-control-center");
          if (path != NULL)
            control_center_cmd = "unity-control-center power";
          else
            control_center_cmd = "gnome-control-center power";

          g_free (path);
        }
      else
       {
         zenity_warning ("dialog-warning",
                         _("Warning"),
                         _("The Ayatana Power Indicator does not support evoking the\npower settings dialog of your desktop environment, yet.\n\nPlease report this to the developers at:\nhttps://github.com/ArcticaProject/ayatana-indicator-power/issues"));
       }
    }

    if (control_center_cmd)
    {
        execute_command(control_center_cmd);
    }
}

gboolean
is_unity ()
{
  const gchar *xdg_current_desktop;
  gchar **desktop_names;
  int i;

  xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
  if (xdg_current_desktop != NULL) {
    desktop_names = g_strsplit (xdg_current_desktop, ":", 0);
    for (i = 0; desktop_names[i]; ++i) {
      if (!g_strcmp0 (desktop_names[i], "Unity")) {
        g_strfreev (desktop_names);
        return TRUE;
      }
    }
    g_strfreev (desktop_names);
  }
  return FALSE;
}

gboolean
is_gnome ()
{
  const gchar *xdg_current_desktop;
  gchar **desktop_names;
  int i;

  xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
  if (xdg_current_desktop != NULL) {
    desktop_names = g_strsplit (xdg_current_desktop, ":", 0);
    for (i = 0; desktop_names[i]; ++i) {
      if (!g_strcmp0 (desktop_names[i], "GNOME")) {
        g_strfreev (desktop_names);
        return TRUE;
      }
    }
    g_strfreev (desktop_names);
  }
  return FALSE;
}

gboolean
is_mate ()
{
  const gchar *xdg_current_desktop;
  gchar **desktop_names;
  int i;

  xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
  if (xdg_current_desktop != NULL) {
    desktop_names = g_strsplit (xdg_current_desktop, ":", 0);
    for (i = 0; desktop_names[i]; ++i) {
      if (!g_strcmp0 (desktop_names[i], "MATE")) {
        g_strfreev (desktop_names);
        return TRUE;
      }
    }
    g_strfreev (desktop_names);
  }
  return FALSE;
}

gboolean
is_xfce ()
{
  const gchar *xdg_current_desktop;
  gchar **desktop_names;
  int i;

  xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
  if (xdg_current_desktop != NULL) {
    desktop_names = g_strsplit (xdg_current_desktop, ":", 0);
    for (i = 0; desktop_names[i]; ++i) {
      if (!g_strcmp0 (desktop_names[i], "XFCE")) {
        g_strfreev (desktop_names);
        return TRUE;
      }
    }
    g_strfreev (desktop_names);
  }
  return FALSE;
}

gboolean
is_pantheon ()
{
  const gchar *xdg_current_desktop;
  gchar **desktop_names;
  int i;

  xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
  if (xdg_current_desktop != NULL) {
    desktop_names = g_strsplit (xdg_current_desktop, ":", 0);
    for (i = 0; desktop_names[i]; ++i) {
      if (!g_strcmp0 (desktop_names[i], "Pantheon")) {
        g_strfreev (desktop_names);
        return TRUE;
      }
    }
    g_strfreev (desktop_names);
  }
  return FALSE;
}
