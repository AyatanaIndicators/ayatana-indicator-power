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

const size_t qcom_sysfs_size = 5;
const char* const qcom_sysfs[] = {"/sys/class/leds/torch-light/brightness",
                                  "/sys/class/leds/led:flash_torch/brightness",
                                  "/sys/class/leds/flashlight/brightness",
                                  "/sys/class/leds/torch-light0/brightness",
                                  "/sys/class/leds/torch-light1/brightness"};

const char* qcom_torch_enable = "/sys/class/leds/led:switch/brightness";

char* flash_sysfs_path = NULL;
gboolean activated = 0;

int
set_sysfs_path()
{
  for (size_t i = 0; i < qcom_sysfs_size; i++) {
    if (access(qcom_sysfs[i], F_OK ) != -1){
        flash_sysfs_path = (char*)qcom_sysfs[i];
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
  FILE *fd1 = NULL, *fd2 = NULL;

  int needs_enable;

  if (!set_sysfs_path())
    return;

  state = g_action_get_state(action);
  activated = g_variant_get_boolean(state);
  g_variant_unref(state);
  fd1 = fopen(flash_sysfs_path, "w");
  if (fd1 != NULL) {
    needs_enable = access(qcom_torch_enable, F_OK ) != -1;
    if (needs_enable)
      fd2 = fopen(qcom_torch_enable, "w");
    if (activated)
      if (needs_enable && fd2 != NULL)
        fprintf(fd2, "0");
      else
        fprintf(fd1, QCOM_DISABLE);
    else {
      fprintf(fd1, QCOM_ENABLE);
      if (needs_enable && fd2 != NULL)
        fprintf(fd2, "1");
    }
    fclose(fd1);
    if (fd2 !=NULL)
      fclose(fd2);
    g_action_change_state(action, g_variant_new_boolean(!activated));
  }
}

int
flashlight_supported()
{
  return set_sysfs_path();
}
