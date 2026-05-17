#include "station_include.h"

gMonStatus staActuatorInitPump(gMonActuators_t *ators) {
    XMEMSET(ators, 0x00, sizeof(gMonActuators_t));
    gMonStatus status = staActuatorGrowSize(ators, GMON_CFG_ACTUATOR_NUM_PUMPS);
    if (status != GMON_RESP_OK && status != GMON_RESP_SKIP)
        goto done;
    static const gMonActuatorConfig_t new_cfg[GMON_CFG_ACTUATOR_NUM_PUMPS] =
        GMON_CFG_ACTUATOR_INIT_PARAMS_PUMP;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        status = staActuatorGenericInit(ator, new_cfg[idx].id, GMON_CFG_ACTUATOR_EMA_LAMBDA_PUMP);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorUpdateParam(&ator->param, &new_cfg[idx].param, staSetTrigThresholdPump);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorPlatformInitPump(ator);
        if (status != GMON_RESP_OK)
            goto done;
        XASSERT(ator->lowlvl != NULL);
    }
done:
    return status;
}

gMonStatus staActuatorDeinitPump(gMonActuators_t *ators) {
    for (int i = 0; i < ators->count; i++)
        staActuatorPlatformDeInitPump(&ators->entries[i]);
    return staActuatorShrinkSize(ators, 0, NULL);
}

gMonStatus staActuatorInitFan(gMonActuators_t *ators) {
    XMEMSET(ators, 0x00, sizeof(gMonActuators_t));
    gMonStatus status = staActuatorGrowSize(ators, GMON_CFG_ACTUATOR_NUM_FANS);
    if (status != GMON_RESP_OK && status != GMON_RESP_SKIP)
        goto done;
    static const gMonActuatorConfig_t new_cfg[GMON_CFG_ACTUATOR_NUM_FANS] = GMON_CFG_ACTUATOR_INIT_PARAMS_FAN;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        status = staActuatorGenericInit(ator, new_cfg[idx].id, GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorUpdateParam(&ator->param, &new_cfg[idx].param, staSetTrigThresholdFan);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorPlatformInitFan(ator);
        if (status != GMON_RESP_OK)
            goto done;
        XASSERT(ator->lowlvl != NULL);
    }
done:
    return status;
}

gMonStatus staActuatorDeinitFan(gMonActuators_t *ators) {
    for (int i = 0; i < ators->count; i++)
        staActuatorPlatformDeInitFan(&ators->entries[i]);
    return staActuatorShrinkSize(ators, 0, NULL);
}

gMonStatus staActuatorInitBulb(gMonActuators_t *ators) {
    XMEMSET(ators, 0x00, sizeof(gMonActuators_t));
    gMonStatus status = staActuatorGrowSize(ators, GMON_CFG_ACTUATOR_NUM_BULBS);
    if (status != GMON_RESP_OK && status != GMON_RESP_SKIP)
        goto done;
    static const gMonActuatorConfig_t new_cfg[GMON_CFG_ACTUATOR_NUM_BULBS] =
        GMON_CFG_ACTUATOR_INIT_PARAMS_BULB;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        status = staActuatorGenericInit(ator, new_cfg[idx].id, GMON_CFG_ACTUATOR_EMA_LAMBDA_BULB);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorUpdateParam(&ator->param, &new_cfg[idx].param, staSetTrigThresholdBulb);
        if (status != GMON_RESP_OK)
            goto done;
        status = staActuatorPlatformInitBulb(ator);
        if (status != GMON_RESP_OK)
            goto done;
        XASSERT(ator->lowlvl != NULL);
    }
done:
    return status;
}

gMonStatus staActuatorDeinitBulb(gMonActuators_t *ators) {
    for (int i = 0; i < ators->count; i++)
        staActuatorPlatformDeInitBulb(&ators->entries[i]);
    return staActuatorShrinkSize(ators, 0, NULL);
}

static gMonStatus TrigSinglePump(gMonActuator_t *ator, gmonEvent_t *evt, gMonSoilSensorMeta_t *sensor) {
    int          soil_moist = 0;
    gMonStatus   status = staActuatorAggregateU32(evt, ator, &soil_moist);
    unsigned int read_period_ms = staSensorReadInterval(sensor);
    // output device starts working until either max working time reached or actual read
    // value lesser than threshold larger input value means dry soil
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (ator->param.threshold < soil_moist))
                                        ? staActuatorMeasureWorkingTime(ator, read_period_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (ator->status != dev_status) {
        ator->status = dev_status;
        staSensorFastPollToggle(sensor, ator);
        status = staPlatformActuatorSwitch(ator);
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

static gMonStatus TrigSingleFan(gMonActuator_t *ator, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    int        air_cond = 0;
    gMonStatus status = staActuatorAggregateAirCond(evt, ator, &air_cond);
    // output device starts working until either max working time reached or actual read
    // value lesser than threshold
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (ator->param.threshold < air_cond))
                                        ? staActuatorMeasureWorkingTime(ator, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (ator->status != dev_status) {
        ator->status = dev_status;
        status = staPlatformActuatorSwitch(ator);
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

static gMonStatus TrigSingleBulb(gMonActuator_t *ator, gmonEvent_t *evt, gMonSensorMeta_t *sensor) {
    // TODO: finish implementation, maximum working time per day for a bulb must be estimate,
    // in case that the plant you're growing still needs more growing light of a day.
    int        lightness = 0;
    gMonStatus status = staActuatorAggregateU32(evt, ator, &lightness);
    // smaller value means less natural light
    gMonActuatorStatus dev_status = ((status == GMON_RESP_OK) && (ator->param.threshold > lightness))
                                        ? staActuatorMeasureWorkingTime(ator, sensor->read_interval_ms)
                                        : GMON_OUT_DEV_STATUS_OFF;
    if (ator->status != dev_status) {
        ator->status = dev_status;
        status = staPlatformActuatorSwitch(ator);
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
