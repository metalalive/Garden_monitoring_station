#include "station_include.h"
// current implementation can fit with following sensors :
// e.g. YL69, Capactive Soil Moisture Sensor v1.2

gMonStatus staSetNumSoilSensor(gMonSensorMeta_t *s, unsigned char new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    unsigned int temp_num_items = 0;
    gMonStatus   status =
        staSetUintInRange(&temp_num_items, (unsigned int)new_val, (unsigned int)GMON_MAXNUM_SOIL_SENSORS, 1U);
    if (status == GMON_RESP_OK)
        s->num_items = (unsigned char)temp_num_items;
    return status;
}

gMonStatus staSetNumResamplesSoilSensor(gMonSensorMeta_t *s, unsigned char new_val) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    unsigned int temp_num_resamples = 0;
    gMonStatus   status = staSetUintInRange(
        &temp_num_resamples, (unsigned int)new_val, (unsigned int)GMON_MAX_OVERSAMPLES_SOIL_SENSORS, 1U
    );
    if (status == GMON_RESP_OK)
        s->num_resamples = (unsigned char)temp_num_resamples;
    return status;
}

gMonStatus staSensorInitSoilMoist(gMonSoilSensorMeta_t *s) {
    gMonStatus status = staSensorSetReadInterval(&s->super, GMON_CFG_SENSOR_READ_INTERVAL_MS);
    if (status != GMON_RESP_OK)
        return status;
    status = staSetNumSoilSensor(&s->super, GMON_CFG_NUM_SOIL_SENSORS);
    if (status != GMON_RESP_OK)
        return status;
    status = staSetNumResamplesSoilSensor(&s->super, GMON_CFG_SOIL_SENSOR_NUM_OVERSAMPLE);
    if (status != GMON_RESP_OK)
        return status;
    status = staSensorSetOutlierThreshold(&s->super, GMON_SOIL_SENSOR_OUTLIER_THRESHOLD);
    if (status != GMON_RESP_OK)
        return status;
    status = staSensorSetMinMAD(&s->super, GMON_SOIL_SENSOR_MAD_THRESHOLD);
    if (status != GMON_RESP_OK)
        return status;
    XMEMSET(s->fast_poll.enabled, 0x0, sizeof(unsigned char) * 1);
    s->fast_poll.divisor = GMON_CFG_SENSOR_FASTPOLL_DIVISOR;
    s->fast_poll._div_cnt = 0;
    return staSensorPlatformInitSoilMoist(&s->super);
}

gMonStatus staSensorDeInitSoilMoist(gMonSoilSensorMeta_t *s) {
    return staSensorPlatformDeInitSoilMoist(&s->super);
}

static unsigned char soilSensorAnyFastPoll(gMonSoilSensorMeta_t *s_meta) {
    // any non-zero bit field indicates at least one sensor requires fast-polling,
    // note currently this application supports max number of 8 identical sensors.
    return s_meta->fast_poll.enabled[0] != 0x0;
}

// This function enables or disables faster polling for specific sensors based on the
// state of an associated actuator.
gMonStatus staSensorFastPollToggle(gMonSoilSensorMeta_t *s_meta, gMonActuator_t *actuator) {
    if (s_meta == NULL || actuator == NULL)
        return GMON_RESP_ERRARGS;
    stationSysEnterCritical();
    switch (actuator->status) {
    case GMON_OUT_DEV_STATUS_ON:
        if (!soilSensorAnyFastPoll(s_meta))
            s_meta->fast_poll._div_cnt = s_meta->fast_poll.divisor;
        s_meta->fast_poll.enabled[0] |= actuator->sensor_id_mask;
        break;
    case GMON_OUT_DEV_STATUS_OFF:
        s_meta->fast_poll.enabled[0] &= ~actuator->sensor_id_mask;
        if (!soilSensorAnyFastPoll(s_meta))
            s_meta->fast_poll._div_cnt = 0;
        break;
    case GMON_OUT_DEV_STATUS_PAUSE:
    case GMON_OUT_DEV_STATUS_BROKEN:
        break;
    }
    stationSysExitCritical();
    return GMON_RESP_OK;
}

// This function manages an internal counter to control the timing of fast polling. It
// ensures accelerated polling rate is maintained by the counter value ranging from
// zero to the configured divisor.
gMonStatus staSensorRefreshFastPollRatio(gMonSoilSensorMeta_t *s_meta) {
    if (s_meta == NULL)
        return GMON_RESP_ERRARGS;
    if (!soilSensorAnyFastPoll(s_meta))
        return GMON_RESP_SKIP;
    unsigned char cntdown = s_meta->fast_poll._div_cnt;
    if (cntdown == 0)
        cntdown = s_meta->fast_poll.divisor;
    s_meta->fast_poll._div_cnt = cntdown - 1;
    return GMON_RESP_OK;
}

// This function determines the appropriate read interval for sensors. It returns
// a reduced (faster) interval if any sensor is currently configured for fast polling;
// otherwise, it returns the default (slower) read interval .
unsigned int staSensorReadInterval(gMonSoilSensorMeta_t *s_meta) {
    if (s_meta == NULL)
        return 0;
    unsigned int out = s_meta->super.read_interval_ms;
    if (soilSensorAnyFastPoll(s_meta)) {
        out = out / s_meta->fast_poll.divisor;
    }
    return out;
}

// This function checks if a particular sensor should be polled in the current cycle.
// - If the fast-poll timing mechanism indicates a "full" poll, all sensors are
//   considered enabled.
// - Otherwise, only sensors specifically marked for fast polling by an active actuator
//   are considered enabled.
char staSensorPollEnabled(gMonSoilSensorMeta_t *s_meta, unsigned short idx) {
    if (s_meta == NULL || s_meta->super.num_items <= idx)
        return 0;
    if (s_meta->fast_poll._div_cnt == 0) {
        return 1; // all sensors is considered enabled polling
    } else {
        return staGetBitFlag(s_meta->fast_poll.enabled, idx);
    }
}

gMonStatus staSensorReadSoilMoist(gMonSoilSensorMeta_t *s_meta, gmonSensorSample_t *readval) {
    gMonStatus status = GMON_RESP_OK;
    stationSysEnterCritical();
    status = staPlatformReadSoilMoistSensor(&s_meta->super, readval);
    stationSysExitCritical();
    if (status == GMON_RESP_OK) {
        status = staSensorDetectNoise(&s_meta->super, readval);
    }
    return status;
}
