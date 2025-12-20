#include "station_include.h"

gMonStatus staSensorSetReadInterval(gMonSensorMeta_t *s, unsigned int new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    return staSetUintInRange(
        &s->read_interval_ms, new_val, (unsigned int)GMON_MAX_SENSOR_READ_INTERVAL_MS,
        (unsigned int)GMON_MIN_SENSOR_READ_INTERVAL_MS
    );
}

gMonStatus staSetNumLightSensor(gMonSensorMeta_t *s, unsigned char new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    unsigned int temp_num_items = 0;
    gMonStatus   status = staSetUintInRange(
        &temp_num_items, (unsigned int)new_val, (unsigned int)GMON_MAXNUM_LIGHT_SENSORS, 0U
    );
    if (status == GMON_RESP_OK)
        s->num_items = (unsigned char)temp_num_items;
    return status;
}

gMonStatus staSetNumResamplesLightSensor(gMonSensorMeta_t *s, unsigned char new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    unsigned int temp_num_resamples = 0;
    gMonStatus   status = staSetUintInRange(
        &temp_num_resamples, (unsigned int)new_val, (unsigned int)GMON_MAX_OVERSAMPLES_LIGHT_SENSORS, 1U
    );
    if (status == GMON_RESP_OK)
        s->num_resamples = (unsigned char)temp_num_resamples;
    return status;
}

gMonStatus staSensorInitLight(gMonSensorMeta_t *s) {
    gMonStatus status = staSensorSetReadInterval(s, GMON_CFG_SENSOR_READ_INTERVAL_MS);
    if (status != GMON_RESP_OK)
        return status;
    status = staSetNumLightSensor(s, GMON_CFG_NUM_LIGHT_SENSORS);
    if (status != GMON_RESP_OK)
        return status;
    status = staSetNumResamplesLightSensor(s, GMON_CFG_LIGHT_SENSOR_NUM_OVERSAMPLE);
    if (status != GMON_RESP_OK)
        return status;
    s->outlier_threshold = GMON_LIGHT_SENSOR_OUTLIER_THRESHOLD;
    s->mad_threshold = GMON_LIGHT_SENSOR_MAD_THRESHOLD;
    return staSensorPlatformInitLight(s);
}

gMonStatus staSensorDeInitLight(gMonSensorMeta_t *s) { return staSensorPlatformDeInitLight(s); }

gMonStatus staSensorReadLight(gMonSensorMeta_t *sensor, gmonSensorSample_t *read_val) {
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadLightSensor(sensor, read_val);
    stationSysExitCritical();
    if (status == GMON_RESP_OK) {
        status = staSensorDetectNoise(sensor, read_val);
    }
    return status;
}
