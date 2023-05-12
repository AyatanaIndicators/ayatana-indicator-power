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
