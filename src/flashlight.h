/*
 * Copyright 2017 The UBports project
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
 *
 * Authors:
 *   Marius Gripsgard <marius@ubports.com>
 */

#ifndef INDICATOR_POWER_FLASHLIGHT__H
#define INDICATOR_POWER_FLASHLIGHT__H

#include <gio/gio.h>

G_BEGIN_DECLS

int
toggle_flashlight_action_qcom();

int
toggle_flashlight_action_simple();

void
toggle_flashlight_action(GAction *action,
                         GVariant *parameter G_GNUC_UNUSED,
                         gpointer data G_GNUC_UNUSED);

int
flashlight_supported();

gboolean
flashlight_activated();

enum
TorchType { SIMPLE = 1, QCOM };

G_END_DECLS

#endif /* INDICATOR_POWER_FLASHLIGHT__H */
