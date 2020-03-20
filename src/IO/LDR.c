#include "station_include.h"

gMonStatus  staSensorInitLight(void)
{
    return  staSensorPlatformInitLight();
}

gMonStatus  staSensorDeInitLight(void)
{
    return  staSensorPlatformDeInitLight();
}

gMonStatus  staSensorReadLight(unsigned int *out)
{
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadLightSensor(out);
    stationSysExitCritical();
    return status;
}

