#include "station_include.h"

gMonStatus staActuatorGrowSize(gMonActuators_t *ators, unsigned char new_count) {
    if (ators == NULL || new_count == 0)
        return GMON_RESP_ERRARGS;
    if (ators->count >= new_count)
        return GMON_RESP_SKIP;
    gMonActuator_t *new_entries = XREALLOC(ators->entries, new_count * sizeof(gMonActuator_t));
    if (new_entries == NULL)
        return GMON_RESP_ERRMEM;
    XMEMSET(&new_entries[ators->count], 0, (new_count - ators->count) * sizeof(gMonActuator_t));
    ators->entries = new_entries;
    ators->count = new_count;
    return GMON_RESP_OK;
}

gMonStatus
staActuatorShrinkSize(gMonActuators_t *ators, unsigned char new_count, const gMonActuatorId_t *ids2rm) {
    if (ators == NULL || ators->entries == NULL || ators->count == 0) {
        return GMON_RESP_ERRARGS;
    } else if (new_count > 0 && ids2rm == NULL) {
        return GMON_RESP_ERRARGS;
    } else if (ators->count <= new_count) {
        return GMON_RESP_SKIP;
    }
    if (new_count == 0) {
        XMEMFREE(ators->entries);
        *ators = (gMonActuators_t){0};
        return GMON_RESP_OK;
    }
    unsigned char idx = 0, num_ids_to_remove = ators->count - new_count;
    size_t        flg_sz = 0;
    for (idx = 0; idx < ators->count; ++idx) {
        if (ators->entries[idx].id == 0)
            return GMON_RESP_ERR_NOT_SUPPORT;
    }
    for (idx = 0; idx < num_ids_to_remove; ++idx) {
        if (ids2rm[idx] == 0) {
            return GMON_RESP_ERR_NOT_SUPPORT;
        } else if (flg_sz < ids2rm[idx]) {
            flg_sz = ids2rm[idx] + 1;
        }
    }
    // Use a bit flag array to mark IDs that should be removed
    unsigned char ids_to_remove_flags[flg_sz];
    XMEMSET(ids_to_remove_flags, 0, flg_sz * sizeof(unsigned char));
    for (idx = 0; idx < num_ids_to_remove; ++idx) {
        unsigned short pos = ids2rm[idx] - 1;
        staSetBitFlag(ids_to_remove_flags, pos, 1);
    }
    gMonActuatorId_t write_idx = 0, read_idx = 0;
    for (read_idx = 0; read_idx < ators->count; ++read_idx) {
        unsigned short pos = ators->entries[read_idx].id - 1;
        // If the current actuator's ID is NOT marked for removal
        if (staGetBitFlag(ids_to_remove_flags, pos) == 0) {
            if (write_idx != read_idx) {
                // Avoid self-assignment if element is already in its correct place
                ators->entries[write_idx] = ators->entries[read_idx];
            }
            write_idx++;
        }
    }
    gMonActuator_t *new_entries = XREALLOC(ators->entries, new_count * sizeof(gMonActuator_t));
    if (new_entries == NULL)
        return GMON_RESP_ERRMEM;
    ators->entries = new_entries;
    ators->count = new_count;
    return GMON_RESP_OK;
}

gMonStatus staActuatorGenericInit(gMonActuator_t *ator, gMonActuatorId_t id, unsigned char ema_lambda_fixpt) {
    if (ator == NULL || id == 0)
        return GMON_RESP_ERRARGS;
    ator->lowlvl = NULL;
    ator->status = GMON_OUT_DEV_STATUS_OFF;
    ator->ema.last_aggregated = 0;
    ator->ema.lambda_fixp = ema_lambda_fixpt;
    ator->id = id;
    return GMON_RESP_OK;
}

gMonStatus staActuatorUpdateParam(
    gMonActuatorParam_t *old_param, const gMonActuatorParam_t *new_param,
    gMonStatus (*set_threshold_fn)(gMonActuatorParam_t *, unsigned int)
) {
    if (old_param == NULL || set_threshold_fn == NULL || new_param == NULL)
        return GMON_RESP_ERRARGS;
    set_threshold_fn(old_param, (unsigned int)new_param->threshold);
    old_param->max_worktime = new_param->max_worktime;
    old_param->min_resttime = new_param->min_resttime;
    // TODO, runtime configurable through remote network users
    old_param->sensor_id_mask = new_param->sensor_id_mask;
    return GMON_RESP_OK;
}

gMonActuator_t *staActuatorFindById(gMonActuators_t *ators, gMonActuatorId_t id) {
    gMonActuator_t *out = NULL;
    if (ators != NULL && id > 0) {
        for (unsigned char idx = 0; (out == NULL) && (idx < ators->count); idx++) {
            if (ators->entries[idx].id == id) {
                out = &ators->entries[idx];
            }
        }
    }
    return out;
}

gMonStatus staSetTrigThresholdPump(gMonActuatorParam_t *param, unsigned int new_val) {
    if (param == NULL)
        return GMON_RESP_ERRARGS;
    return staSetUintInRange(
        (unsigned int *)&param->threshold, new_val, (unsigned int)GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP,
        (unsigned int)GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP
    );
}
gMonStatus staSetTrigThresholdFan(gMonActuatorParam_t *param, unsigned int new_val) {
    if (param == NULL)
        return GMON_RESP_ERRARGS;
    return staSetUintInRange(
        (unsigned int *)&param->threshold, new_val, (unsigned int)GMON_MAX_ACTUATOR_TRIG_THRESHOLD_FAN,
        (unsigned int)GMON_MIN_ACTUATOR_TRIG_THRESHOLD_FAN
    );
}
gMonStatus staSetTrigThresholdBulb(gMonActuatorParam_t *param, unsigned int new_val) {
    if (param == NULL)
        return GMON_RESP_ERRARGS;
    return staSetUintInRange(
        (unsigned int *)&param->threshold, new_val, (unsigned int)GMON_MAX_ACTUATOR_TRIG_THRESHOLD_BULB,
        (unsigned int)GMON_MIN_ACTUATOR_TRIG_THRESHOLD_BULB
    );
}

gMonStatus staActuatorAggregateU32(gmonEvent_t *evt, gMonActuator_t *dev, int *value) {
    if (evt == NULL || dev == NULL || value == NULL)
        return GMON_RESP_ERRARGS;
    if (evt->num_active_sensors == 0x0 || dev->param.sensor_id_mask == 0x0)
        return GMON_RESP_MALFORMED_DATA;

    unsigned int *event_data_u32 = (unsigned int *)evt->data;
    unsigned int  sum = 0, count = 0, avg = 0;
    for (unsigned char i = 0; i < evt->num_active_sensors; i++) {
        // Aggregate data if the sensor is relevant to the actuator (mask bit set)
        // and its data is not marked as corrupted (corruption flag clear).
        unsigned char idmsk = dev->param.sensor_id_mask;
        if (staGetBitFlag(&idmsk, i) && !staGetBitFlag(&evt->flgs.corruption, i)) {
            sum += event_data_u32[i];
            count++;
        }
    }
    if (count == 0 || sum == 0)
        return GMON_RESP_SKIP;
    avg = sum / count;
    if (avg == 0)
        return GMON_RESP_SKIP;
    int ma0 = dev->ema.last_aggregated, ma1 = 0;
    if (ma0 == 0) {
        ma1 = (int)avg;
    } else {
        ma1 = staExpMovingAvg((int)avg, ma0, dev->ema.lambda_fixp);
    }
    *value = ma1;
    dev->ema.last_aggregated = ma1;
    return GMON_RESP_OK;
} // end of staActuatorAggregateU32

gMonStatus staActuatorAggregateAirCond(gmonEvent_t *evt, gMonActuator_t *dev, int *value) {
    if (evt == NULL || dev == NULL || value == NULL)
        return GMON_RESP_ERRARGS;
    if (evt->num_active_sensors == 0x0 || dev->param.sensor_id_mask == 0x0)
        return GMON_RESP_MALFORMED_DATA;

    gmonAirCond_t *event_data_ac = (gmonAirCond_t *)evt->data;
    gmonAirCond_t  sum = {0}, avg = {0};
    unsigned int   count = 0;
    unsigned char  i = 0;
    for (i = 0, count = 0; i < evt->num_active_sensors; i++) {
        // Aggregate data if the sensor is relevant to the actuator (mask bit set)
        // and its data is not marked as corrupted (corruption flag clear).
        unsigned char idmsk = dev->param.sensor_id_mask;
        if (staGetBitFlag(&idmsk, i) && !staGetBitFlag(&evt->flgs.corruption, i)) {
            sum.temporature += event_data_ac[i].temporature;
            sum.humidity += event_data_ac[i].humidity;
            count++;
        }
    }
    if (count == 0 || sum.temporature == 0.f || sum.humidity == 0.f)
        return GMON_RESP_SKIP;
    avg.temporature = sum.temporature / count;
    avg.humidity = sum.humidity / count;
    if (avg.temporature == 0.f || avg.humidity == 0.f)
        return GMON_RESP_SKIP;
    // normalize the average numbers, multiply by 100, and reuse `sum` to save them
    sum.temporature = (avg.temporature - GMON_MIN_AIR_TEMPERATURE) * 100 /
                      (GMON_MAX_AIR_TEMPERATURE - GMON_MIN_AIR_TEMPERATURE);
    sum.humidity = (avg.humidity - GMON_MIN_AIR_HUMIDITY_SUPPORTED) * 100 /
                   (GMON_MAX_AIR_HUMIDITY_SUPPORTED - GMON_MIN_AIR_HUMIDITY_SUPPORTED);
#define AIRTEMP_WEIGHT 0.55
    float new_aircond = AIRTEMP_WEIGHT * sum.temporature + (1 - AIRTEMP_WEIGHT) * sum.humidity;
#undef AIRTEMP_WEIGHT
    int ma0 = dev->ema.last_aggregated, ma1 = 0;
    if (ma0 == 0) {
        ma1 = (int)new_aircond;
    } else {
        ma1 = staExpMovingAvg((int)new_aircond, ma0, dev->ema.lambda_fixp);
    }
    *value = ma1;
    dev->ema.last_aggregated = ma1;
    return GMON_RESP_OK;
} // end of staActuatorAggregateAirCond

gMonStatus staPauseWorkingActuators(gardenMonitor_t *gmon) {
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    // pause the actuators which require precise control
    gMonActuators_t *ators = &gmon->actuator.pump;
    for (unsigned char idx = 0; idx < ators->count; idx++) {
        gMonActuator_t *ator = &ators->entries[idx];
        if (ator->status == GMON_OUT_DEV_STATUS_ON) {
            ator->curr_worktime = ator->param.max_worktime;
            // For pausing, we simply set the status and do not trigger the actuator with a sensor value.
            // The intention of this block is to force a PAUSE state, not to actually trigger the pump.
            // If a trigger based on a sensor is needed here, the sensor object needs to be passed.
            // Manually set to PAUSE for this specific function's logic
            ator->status = GMON_OUT_DEV_STATUS_PAUSE;
        }
    }
    return GMON_RESP_OK;
}

gMonStatus staEmergencyShutdownAllActuators(gardenMonitor_t *gmon) {
#define NUM_ACTUATOR_TYPES 3
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    gMonStatus status[NUM_ACTUATOR_TYPES] = {0};
    status[0] = staTurnOffActuator(&gmon->actuator.pump);
    status[1] = staTurnOffActuator(&gmon->actuator.fan);
    status[2] = staTurnOffActuator(&gmon->actuator.bulb);
    for (short idx = 0; idx < NUM_ACTUATOR_TYPES; idx++) {
        if (status[idx] != GMON_RESP_OK)
            return status[idx];
    }
    return GMON_RESP_OK;
#undef NUM_ACTUATOR_TYPES
}

gMonActuatorStatus staActuatorMeasureWorkingTime(gMonActuator_t *dev, unsigned int time_elapsed_ms) {
    gMonActuatorStatus next_status = GMON_OUT_DEV_STATUS_OFF;
    switch (dev->status) {
    case GMON_OUT_DEV_STATUS_OFF:
        if (dev->param.max_worktime > 0) {
            next_status = GMON_OUT_DEV_STATUS_ON;
            dev->curr_worktime = time_elapsed_ms;
            dev->curr_resttime = 0;
        }
        break;
    case GMON_OUT_DEV_STATUS_ON:
        dev->curr_worktime += time_elapsed_ms;
        if (dev->curr_worktime >= dev->param.max_worktime) {
            dev->curr_worktime = 0;
            next_status = GMON_OUT_DEV_STATUS_PAUSE;
        } else {
            next_status = GMON_OUT_DEV_STATUS_ON;
        }
        break;
    case GMON_OUT_DEV_STATUS_PAUSE:
        dev->curr_resttime += time_elapsed_ms;
        if (dev->curr_resttime >= dev->param.min_resttime) {
            dev->curr_resttime = 0;
            next_status = GMON_OUT_DEV_STATUS_ON;
        } else {
            next_status = GMON_OUT_DEV_STATUS_PAUSE;
        }
        break;
    default:
        next_status = GMON_OUT_DEV_STATUS_OFF;
        break;
    } // end of switch case
    return next_status;
} // end of staActuatorMeasureWorkingTime
