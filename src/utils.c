/*
 * Copyright 2013 Canonical Ltd.
 * Copytight 2018 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 * Copytight 2021 Robert Tari <robert@tari.in>
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

#include <ayatana/common/utils.h>
#include "utils.h"

#ifdef HAS_URLDISPATCHER
# include <lomiri-url-dispatcher.h>
#endif

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
      else if (is_budgie() || is_unity() || is_gnome())
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
