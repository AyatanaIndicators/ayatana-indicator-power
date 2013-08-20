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

#ifndef __IB_BRIGHTNESS_CONTROL_H__
#define __IB_BRIGHTNESS_CONTROL_H__

#include <gio/gio.h>

typedef struct _IbBrightnessControl IbBrightnessControl;

IbBrightnessControl*  ib_brightness_control_new (void);
void                  ib_brightness_control_set_value (IbBrightnessControl* self, gint value);
gint                  ib_brightness_control_get_value (IbBrightnessControl* self);
gint                  ib_brightness_control_get_max_value (IbBrightnessControl* self);
void                  ib_brightness_control_free (IbBrightnessControl *self);

#endif
