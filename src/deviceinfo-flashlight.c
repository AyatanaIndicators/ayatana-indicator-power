/*
 * Copyright 2023 The UBports project
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
 *   Muhammad <muhammad23012009@hotmail.com>
 */

#include <deviceinfo_c_api.h>

char* flashlight_path()
{
        struct DeviceInfo* di = deviceinfo_new();
        return deviceinfo_get(di, "FlashlightSysfsPath", "");
}

char* flashlight_switch_path()
{
        struct DeviceInfo* di = deviceinfo_new();
        return deviceinfo_get(di, "FlashlightSwitchPath", "");
}

char* flashlight_simple_enable_value()
{
         struct DeviceInfo* di = deviceinfo_new();
         return deviceinfo_get(di, "FlashlightSimpleEnableValue", "");
}

char* flashlight_simple_disable_value()
{
         struct DeviceInfo* di = deviceinfo_new();
         return deviceinfo_get(di, "FlashlightSimpleDisableValue", "");
}
