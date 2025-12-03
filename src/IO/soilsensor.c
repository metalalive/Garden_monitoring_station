#include "station_include.h"
// current implementation can fit with following sensors :
// e.g. YL69, Capactive Soil Moisture Sensor v1.2

gMonStatus staSensorInitSoilMoist(gMonSensor_t *s) {
    s->read_interval_ms = GMON_CFG_SENSOR_READ_INTERVAL_MS;
    s->num_items = GMON_CFG_NUM_SOIL_SENSORS;
    s->num_resamples = GMON_CFG_SOIL_SENSOR_NUM_OVERSAMPLE;
    return staSensorPlatformInitSoilMoist(s);
}

gMonStatus staSensorDeInitSoilMoist(gMonSensor_t *s) { return staSensorPlatformDeInitSoilMoist(s); }

gMonStatus staSensorReadSoilMoist(gMonSensor_t *sensor, gmonSensorSample_t *readval) {
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadSoilMoistSensor(sensor, readval);
    // TODO, filter noise
    stationSysExitCritical();
    return status;
}
