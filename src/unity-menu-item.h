/*
 * Copyright 2012 Canonical Ltd.
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
 *     Renato Araujo Oliveira Filho <renato@canonical.com>
 */

#ifndef __UNITY_MENU_ITEM_H_
#define __UNITY_MENU_ITEM_H_

#include <gio/gio.h>

GMenuItem*  unity_menu_item_slider_new (const gchar *label,
                                        const gchar *value_action,
                                        int min_value,
                                        int max_value,
                                        GVariant * left_icon,
                                        GVariant * right_icon);

GMenuItem*  unity_menu_item_switch_new (const gchar *label,
                                        const gchar *state_action);

#endif
