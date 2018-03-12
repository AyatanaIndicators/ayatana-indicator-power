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
# include <url-dispatcher.h>
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

void
utils_handle_settings_request (void)
{
  static const gchar *control_center_cmd = NULL;

  if (control_center_cmd == NULL)
    {
#ifdef HAS_URLDISPATCHER
      if (g_getenv ("MIR_SOCKET") != NULL)
        {
          url_dispatch_send("settings:///system/battery", NULL, NULL);
          return;
        }
      else
#endif
      if ((!g_strcmp0 (g_getenv ("DESKTOP_SESSION"), "xubuntu")) || (!g_strcmp0 (g_getenv ("DESKTOP_SESSION"), "xfce")))
        {
          control_center_cmd = "xfce4-power-manager-settings";
        }
      else if (!g_strcmp0 (g_getenv ("DESKTOP_SESSION"), "mate"))
        {
          control_center_cmd = "mate-power-preferences";
        }
      else if (!g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "Pantheon"))
        {
          control_center_cmd = "switchboard --open-plug system-pantheon-power";
        }
      else
        {
          gchar *path;

          path = g_find_program_in_path ("unity-control-center");
          if (path != NULL)
            control_center_cmd = "unity-control-center power";
          else
            control_center_cmd = "gnome-control-center power";

          g_free (path);
        }
    }

  execute_command (control_center_cmd);
}
