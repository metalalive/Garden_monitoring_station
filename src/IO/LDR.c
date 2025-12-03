#include "station_include.h"

gMonStatus staSensorInitLight(gMonSensor_t *s) {
    s->read_interval_ms = GMON_CFG_SENSOR_READ_INTERVAL_MS;
    s->num_items = GMON_CFG_NUM_LIGHT_SENSORS;
    s->num_resamples = GMON_CFG_LIGHT_SENSOR_NUM_OVERSAMPLE;
    return staSensorPlatformInitLight(s);
}

gMonStatus staSensorDeInitLight(gMonSensor_t *s) { return staSensorPlatformDeInitLight(s); }

gMonStatus staSensorReadLight(gMonSensor_t *sensor, gmonSensorSample_t *read_val) {
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadLightSensor(sensor, read_val);
    // TODO, filter noise
    stationSysExitCritical();
    return status;
}
