/*
 * Copyright 2025 The UBports project
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
 *   Lorenzo Torracchi <lorenzotorracchi@mail.com>
 */

#include "keep-screen-on.h"

#include <gio/gio.h>
#include <ayatana/common/utils.h>

#define UNITY_SCREEN_BUS_NAME "com.canonical.Unity.Screen"
#define UNITY_SCREEN_OBJECT_PATH "/com/canonical/Unity/Screen"
#define UNITY_SCREEN_INTERFACE "com.canonical.Unity.Screen"

static guint request_id = 0;

void
toggle_keep_screen_on_action(GAction *action,
                         GVariant *parameter G_GNUC_UNUSED,
                         gpointer data G_GNUC_UNUSED)
{
	GVariant *state = g_action_get_state(action);
  	gboolean enabled = g_variant_get_boolean(state);
    GError *error = NULL;
    GDBusProxy *proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        UNITY_SCREEN_BUS_NAME,
        UNITY_SCREEN_OBJECT_PATH,
        UNITY_SCREEN_INTERFACE,
        NULL,
        &error
    );

    if (!proxy) {
        g_warning("Failed to create D-Bus proxy: %s", error->message);
        g_clear_error(&error);
        return;
    }

    if (!enabled) {
        GVariant *result = g_dbus_proxy_call_sync(
            proxy,
            "keepDisplayOn",
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error
        );
        if (result) {
            g_variant_get(result, "(i)", &request_id);
            g_variant_unref(result);
			g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean(!enabled));
        } else {
            g_warning("D-Bus call keepDisplayOn failed: %s", error->message);
            g_clear_error(&error);
        }
    } else if (request_id != 0) {
        GVariant *params = g_variant_new("(i)", request_id);
        g_dbus_proxy_call_sync(
            proxy,
            "removeDisplayOnRequest",
            params,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error
        );
        if (error) {
            g_warning("D-Bus call removeDisplayOnRequest failed: %s", error->message);
            g_clear_error(&error);
        } else {
			g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean(!enabled));
			request_id = 0;
		}
    }

    g_object_unref(proxy);
}

int
keep_screen_on_supported()
{
  return ayatana_common_utils_is_lomiri();
}

