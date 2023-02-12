/*
 * Copyright 2017 The UBports project
 * Copyright 2023 Robert Tari
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
 *   Robert Tari <robert@tari.in>
 */

#include "flashlight.h"

#include <gio/gio.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define QCOM_ENABLE "255"
#define QCOM_DISABLE "0"
#define SIMPLE_ENABLE "1"
#define SIMPLE_DISABLE "0"

const size_t qcom_sysfs_size = 7;
const char* const qcom_sysfs[] = {"/sys/class/leds/torch-light/brightness",
                                  "/sys/class/leds/led:flash_torch/brightness",
                                  "/sys/class/leds/flashlight/brightness",
                                  "/sys/class/leds/torch-light0/brightness",
                                  "/sys/class/leds/torch-light1/brightness",
                                  "/sys/class/leds/led:torch_0/brightness",
                                  "/sys/class/leds/led:torch_1/brightness"};
const size_t qcom_switch_size = 2;
const char* const qcom_switch[] = {"/sys/class/leds/led:switch/brightness",
                                   "/sys/class/leds/led:switch_0/brightness"};

const size_t simple_sysfs_size = 2;
const char* const simple_sysfs[] = {"/sys/class/flashlight_core/flashlight/flashlight_torch",
                                    "/sys/class/leds/white:flash/brightness"};

char* flash_sysfs_path = NULL;
char* qcom_switch_path = NULL;

enum TorchType torch_type = SIMPLE;
gboolean activated = 0;

int
set_sysfs_path()
{
  for (size_t i = 0; i < qcom_sysfs_size; i++) {
    if (access(qcom_sysfs[i], F_OK ) != -1){
        flash_sysfs_path = (char*)qcom_sysfs[i];
        torch_type = QCOM;
        /* Qualcomm torch; determine switch file (if one is needed) */
        for (size_t i = 0; i < qcom_switch_size; i++) {
          if (access(qcom_switch[i], F_OK ) != -1)
              qcom_switch_path = (char*)qcom_switch[i];
        }
        return 1;
    }
  }
  for (size_t i = 0; i < simple_sysfs_size; i++) {
    if (access(simple_sysfs[i], F_OK ) != -1){
        flash_sysfs_path = (char*)simple_sysfs[i];
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

int
toggle_flashlight_action_qcom()
{
  FILE *fd1 = NULL, *fd2 = NULL;
  int needs_enable;

  fd1 = fopen(flash_sysfs_path, "w");
  if (fd1 != NULL) {
    needs_enable = access(qcom_switch_path, F_OK ) != -1;
    if (needs_enable)
      fd2 = fopen(qcom_switch_path, "w");
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
    return 1;
  }
  return 0;
}

int
toggle_flashlight_action_simple()
{
  FILE *fd = NULL;

  fd = fopen(flash_sysfs_path, "w");
  if (fd != NULL) {
    fprintf(fd, activated ? SIMPLE_DISABLE : SIMPLE_ENABLE);
    fclose(fd);
    return 1;
  }
  return 0;
}

void
toggle_flashlight_action(GAction *action,
                         GVariant *parameter G_GNUC_UNUSED,
                         gpointer data G_GNUC_UNUSED)
{
  GVariant *state;
  int toggled;

  if (!set_sysfs_path())
    return;

  state = g_action_get_state(action);
  activated = g_variant_get_boolean(state);
  g_variant_unref(state);

  if (torch_type == QCOM)
    toggled = toggle_flashlight_action_qcom();
  else
    toggled = toggle_flashlight_action_simple();

  if (toggled)
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean(!activated));
}

int
flashlight_supported()
{
  return set_sysfs_path();
}
