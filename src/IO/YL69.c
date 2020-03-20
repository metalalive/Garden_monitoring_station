#include "station_include.h"

gMonStatus  staSensorInitSoilMoist(void)
{
    return  staSensorPlatformInitSoilMoist();
}

gMonStatus  staSensorDeInitSoilMoist(void)
{
    return  staSensorPlatformDeInitSoilMoist();
}

gMonStatus  staSensorReadSoilMoist(unsigned int *out)
{
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadSoilMoistSensor(out);
    stationSysExitCritical();
    return  status;
}

