#include "station_include.h"

gMonStatus staSensorInitLight(gMonSensor_t *s) {
    s->read_interval_ms = GMON_CFG_SENSOR_READ_INTERVAL_MS;
    s->num_items = GMON_CFG_NUM_LIGHT_SENSORS;
    s->num_resamples = GMON_CFG_LIGHT_SENSOR_NUM_OVERSAMPLE;
    s->outlier_threshold = GMON_LIGHT_SENSOR_OUTLIER_THRESHOLD;
    return staSensorPlatformInitLight(s);
}

gMonStatus staSensorDeInitLight(gMonSensor_t *s) { return staSensorPlatformDeInitLight(s); }

gMonStatus staSensorReadLight(gMonSensor_t *sensor, gmonSensorSample_t *read_val) {
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadLightSensor(sensor, read_val);
    stationSysExitCritical();
    if (status == GMON_RESP_OK) {
        status = staSensorDetectNoise(sensor->outlier_threshold, read_val, sensor->num_items);
    }
    return status;
}
