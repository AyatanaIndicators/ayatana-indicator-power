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

#include "unity-menu-item.h"

GMenuItem*
unity_menu_item_slider_new (const gchar *label,
                            const gchar *value_action,
                            int min_value,
                            int max_value,
                            GVariant * left_icon,
                            GVariant * right_icon)
{
    GMenuItem *menu_item = g_menu_item_new (label, value_action);

    g_menu_item_set_attribute (menu_item,
                               "x-canonical-type",
                               "s",
                               "com.canonical.unity.slider");
    g_menu_item_set_attribute (menu_item,
                               "x-canonical-min",
                               "i",
                               min_value);
    g_menu_item_set_attribute (menu_item,
                               "x-canonical-max",
                               "i",
                               max_value);
    if (left_icon != NULL) {
        g_menu_item_set_attribute (menu_item,
                                   "min-icon",
                                   "*",
                                   left_icon);
    }

    if (right_icon != NULL) {
        g_menu_item_set_attribute (menu_item,
                                   "max-icon",
                                   "*",
                                   right_icon);
    }

    return menu_item;
}


GMenuItem*
unity_menu_item_switch_new (const gchar *label,
                            const gchar *state_action)
{
    GMenuItem *menu_item = g_menu_item_new (label, state_action);

    g_menu_item_set_attribute (menu_item,
                               "x-canonical-type",
                               "s",
                               "com.canonical.unity.switch");

    return menu_item;
}
