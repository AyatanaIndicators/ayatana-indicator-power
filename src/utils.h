/*
 * Copytight 2018 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
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

#ifndef __INDICATOR_POWER_UTILS_H__
#define __INDICATOR_POWER_UTILS_H__

#include <glib.h>
#include <string.h>

void execute_command (const gchar * cmd);
void utils_handle_settings_request(void);

gboolean is_unity();
gboolean is_gnome();
gboolean is_mate();

#endif /* __INDICATOR_POWER_UTILS_H__ */
