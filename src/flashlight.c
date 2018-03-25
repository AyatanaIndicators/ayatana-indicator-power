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

#include "flashlight.h"

#include <gio/gio.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define QCOM_ENABLE "255"
#define QCOM_DISABLE "0"

const size_t qcom_sysfs_size = 2;
const char* const qcom_sysfs[] = {"/sys/class/leds/torch-light/brightness", "/sys/class/leds/led:flash_torch/brightness"};

char* flash_sysfs_path = NULL;
gboolean activated = 0;

int
set_sysfs_path()
{
  for (size_t i = 0; i < qcom_sysfs_size; i++) {
    if (access(qcom_sysfs[i], F_OK ) != -1){
        flash_sysfs_path = qcom_sysfs[i];
        return 1;
    }
  }
  return 0;
}

gboolean
flashlight_activated()
{
  return activated;
}

void
toggle_flashlight_action(GAction *action,
                         GVariant *parameter G_GNUC_UNUSED,
                         gpointer data G_GNUC_UNUSED)
{
  GVariant *state;
  FILE* fd;

  if (!set_sysfs_path())
    return;

  state = g_action_get_state(action);
  activated = g_variant_get_boolean(state);
  g_variant_unref(state);
  fd = fopen(flash_sysfs_path, "w");
  if (fd != NULL){
      if (activated)
        fprintf(fd, QCOM_DISABLE);
      else
        fprintf(fd, QCOM_ENABLE);
      fclose(fd);
      g_action_change_state(action, g_variant_new_boolean(!activated));
  }
}

int
flashlight_supported()
{
  return set_sysfs_path();
}
