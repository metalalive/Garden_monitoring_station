#include "station_include.h"

gMonStatus staActuatorInitPump(gMonActuators_t *ators) {
    XMEMSET(ators, 0x00, sizeof(gMonActuators_t));
    gMonStatus status = staActuatorAdjustSize(ators, GMON_CFG_ACTUATOR_NUM_PUMPS);
    if (status != GMON_RESP_OK)
        goto done;
    gMonActuatorParam_t new_param = {
        .threshold = GMON_CFG_ACTUATOR_TRIG_THRESHOLD_PUMP,
        .max_worktime = GMON_CFG_ACTUATOR_MAX_WORKTIME_PUMP,
        .min_resttime = GMON_CFG_ACTUATOR_MIN_RESTTIME_PUMP,
        .sensor_id_mask = GMON_CFG_ACTUATOR_SENSOR_MASK_PUMP,
    };
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        status = staActuatorGenericInit(ator, GMON_CFG_ACTUATOR_EMA_LAMBDA_PUMP);
        if (status != GMON_RESP_OK)
            goto done;
        new_param.sensor_id_mask = new_param.sensor_id_mask << 1;
        status = staActuatorUpdateParam(ator, &new_param, staSetTrigThresholdPump);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorPlatformInitPump(&ator->lowlvl);
        if (status != GMON_RESP_OK)
            goto done;
        XASSERT(ator->lowlvl != NULL);
    }
done:
    return status;
}

gMonStatus staActuatorDeinitPump(gMonActuators_t *ators) { return staActuatorAdjustSize(ators, 0); }

gMonStatus staActuatorInitFan(gMonActuators_t *ators) {
    XMEMSET(ators, 0x00, sizeof(gMonActuators_t));
    gMonStatus status = staActuatorAdjustSize(ators, GMON_CFG_ACTUATOR_NUM_FANS);
    if (status != GMON_RESP_OK)
        goto done;
    const gMonActuatorParam_t new_param = {
        .threshold = GMON_CFG_ACTUATOR_TRIG_THRESHOLD_FAN,
        .max_worktime = GMON_CFG_ACTUATOR_MAX_WORKTIME_FAN,
        .min_resttime = GMON_CFG_ACTUATOR_MIN_RESTTIME_FAN,
        .sensor_id_mask = GMON_CFG_ACTUATOR_SENSOR_MASK_FAN,
    };
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        status = staActuatorGenericInit(ator, GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorUpdateParam(ator, &new_param, staSetTrigThresholdFan);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorPlatformInitFan(&ator->lowlvl);
        if (status != GMON_RESP_OK)
            goto done;
        XASSERT(ator->lowlvl != NULL);
    }
done:
    return status;
}

gMonStatus staActuatorDeinitFan(gMonActuators_t *ators) { return staActuatorAdjustSize(ators, 0); }

gMonStatus staActuatorInitBulb(gMonActuators_t *ators) {
    XMEMSET(ators, 0x00, sizeof(gMonActuators_t));
    gMonStatus status = staActuatorAdjustSize(ators, GMON_CFG_ACTUATOR_NUM_BULBS);
    if (status != GMON_RESP_OK)
        goto done;
    const gMonActuatorParam_t new_param = {
        .threshold = GMON_CFG_ACTUATOR_TRIG_THRESHOLD_BULB,
        .max_worktime = GMON_CFG_ACTUATOR_MAX_WORKTIME_BULB,
        .min_resttime = GMON_CFG_ACTUATOR_MIN_RESTTIME_BULB,
        .sensor_id_mask = GMON_CFG_ACTUATOR_SENSOR_MASK_BULB,
    };
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        status = staActuatorGenericInit(ator, GMON_CFG_ACTUATOR_EMA_LAMBDA_BULB);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorUpdateParam(ator, &new_param, staSetTrigThresholdBulb);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorPlatformInitBulb(&ator->lowlvl);
        if (status != GMON_RESP_OK)
            goto done;
        XASSERT(ator->lowlvl != NULL);
    }
done:
    return status;
}

gMonStatus staActuatorDeinitBulb(gMonActuators_t *ators) { return staActuatorAdjustSize(ators, 0); }

static gMonStatus TrigSinglePump(gMonActuator_t *dev, gmonEvent_t *evt, gMonSoilSensorMeta_t *sensor) {
    int          soil_moist = 0;
    gMonStatus   status = staActuatorAggregateU32(evt, dev, &soil_moist);
    unsigned int read_period_ms = staSensorReadInterval(sensor);
    // output device starts working until either max working time reached or actual read
    // value lesser than threshold larger input value means dry soil
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (dev->user_param.threshold < soil_moist))
                                        ? staActuatorMeasureWorkingTime(dev, read_period_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (dev->status != dev_status) {
        dev->status = dev_status;
        staSensorFastPollToggle(sensor, dev);
        uint8_t pin_state =
            (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET : GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(dev->lowlvl, pin_state);
    }
    return status;
}
gMonStatus staActuatorTrigPump(gMonActuators_t *ators, gmonEvent_t *evt, gMonSoilSensorMeta_t *s_meta) {
    if (ators == NULL || evt == NULL || s_meta == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (evt->event_type != GMON_EVENT_SOIL_MOISTURE_UPDATED) {
        return GMON_RESP_ERRARGS;
    } else if (ators->count == 0 || ators->entries == NULL) {
        return GMON_RESP_SKIP;
    }
    gMonStatus status = GMON_RESP_OK;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonStatus s = TrigSinglePump(&ators->entries[idx], evt, s_meta);
        if (s != GMON_RESP_OK)
            s = status;
    }
    return status;
}

static gMonStatus TrigSingleFan(gMonActuator_t *dev, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    int        air_cond = 0;
    gMonStatus status = staActuatorAggregateAirCond(evt, dev, &air_cond);
    // output device starts working until either max working time reached or actual read
    // value lesser than threshold
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (dev->user_param.threshold < air_cond))
                                        ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (dev->status != dev_status) {
        dev->status = dev_status;
        uint8_t pin_state =
            (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET : GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(dev->lowlvl, pin_state);
    }
    return status;
}
gMonStatus staActuatorTrigFan(gMonActuators_t *ators, gmonEvent_t *evt, gMonSensorMeta_t *s_meta) {
    if (ators == NULL || evt == NULL || s_meta == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (evt->event_type != GMON_EVENT_AIR_TEMP_UPDATED) {
        return GMON_RESP_ERRARGS;
    } else if (ators->count == 0 || ators->entries == NULL) {
        return GMON_RESP_SKIP;
    }
    gMonStatus status = GMON_RESP_OK;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonStatus s = TrigSingleFan(&ators->entries[idx], evt, s_meta);
        if (s != GMON_RESP_OK)
            s = status;
    }
    return status;
}

static gMonStatus TrigSingleBulb(gMonActuator_t *dev, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    // TODO: finish implementation, maximum working time per day for a bulb must be estimate,
    // in case that the plant you're growing still needs more growing light of a day.
    int        lightness = 0;
    gMonStatus status = staActuatorAggregateU32(evt, dev, &lightness);
    // smaller value means less natural light
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (dev->user_param.threshold > lightness))
                                        ? staActuatorMeasureWorkingTime(dev, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (dev->status != dev_status) {
        dev->status = dev_status;
        uint8_t pin_state =
            (dev_status == GMON_OUT_DEV_STATUS_ON ? GMON_PLATFORM_PIN_SET : GMON_PLATFORM_PIN_RESET);
        status = staPlatformWritePin(dev->lowlvl, pin_state);
    }
    return status;
}
gMonStatus staActuatorTrigBulb(gMonActuators_t *ators, gmonEvent_t *evt, gMonSensorMeta_t *s_meta) {
    if (ators == NULL || evt == NULL || s_meta == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (evt->event_type != GMON_EVENT_LIGHTNESS_UPDATED) {
        return GMON_RESP_ERRARGS;
    } else if (ators->count == 0 || ators->entries == NULL) {
        return GMON_RESP_SKIP;
    }
    gMonStatus status = GMON_RESP_OK;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonStatus s = TrigSingleBulb(&ators->entries[idx], evt, s_meta);
        if (s != GMON_RESP_OK)
            s = status;
    }
    return status;
}

gMonStatus staTurnOffActuator(gMonActuators_t *ators) {
    if (ators == NULL)
        return GMON_RESP_ERRARGS;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ac = &ators->entries[idx];
        ac->status = GMON_OUT_DEV_STATUS_OFF;
        staPlatformWritePin(ac->lowlvl, GMON_PLATFORM_PIN_RESET);
    }
    return GMON_RESP_OK;
}
