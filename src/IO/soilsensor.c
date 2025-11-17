#include "station_include.h"
// current implementation can fit with following sensors : 
// e.g. YL69, Capactive Soil Moisture Sensor v1.2

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

