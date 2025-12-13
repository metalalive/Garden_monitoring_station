#include "station_include.h"

// The parameters below are used to store default maximum continuously working time of user's bulb
// , since the maximum working time of a bulb would be adjust every 24 hours for unstable natural light
// environment e.g. the plant you grow need 7 hours growing light every day but your garden can only provide 4
// hours natural
//      light per day
// TODO, recheck if It's necessary to use this
static unsigned int gmon_bulb_max_worktime_default;

gMonStatus staActuatorInitGenericPump(gMonActuator_t *dev) {
    if (dev == NULL)
        return GMON_RESP_ERRARGS;
    staSetTrigThresholdPump(dev, (unsigned int)GMON_CFG_ACTUATOR_TRIG_THRESHOLD_PUMP);
    dev->status = GMON_OUT_DEV_STATUS_OFF;
    dev->ema.last_aggregated = 0;
    dev->max_worktime = GMON_CFG_ACTUATOR_MAX_WORKTIME_PUMP;
    dev->min_resttime = GMON_CFG_ACTUATOR_MIN_RESTTIME_PUMP;
    // TODO, the fields below should also be able to change through remote network users
    dev->sensor_id_mask = GMON_CFG_ACTUATOR_SENSOR_MASK_PUMP;
    dev->ema.lambda_fixp = GMON_CFG_ACTUATOR_EMA_LAMBDA_PUMP;
    return GMON_RESP_OK;
}

gMonStatus staActuatorDeinitGenericPump(void) { return GMON_RESP_OK; }

gMonStatus staActuatorInitGenericFan(gMonActuator_t *dev) {
    if (dev == NULL)
        return GMON_RESP_ERRARGS;
    staSetTrigThresholdFan(dev, (unsigned int)GMON_CFG_ACTUATOR_TRIG_THRESHOLD_FAN);
    dev->status = GMON_OUT_DEV_STATUS_OFF;
    dev->ema.last_aggregated = 0;
    dev->max_worktime = GMON_CFG_ACTUATOR_MAX_WORKTIME_FAN;
    dev->min_resttime = GMON_CFG_ACTUATOR_MIN_RESTTIME_FAN;
    dev->sensor_id_mask = GMON_CFG_ACTUATOR_SENSOR_MASK_FAN;
    dev->ema.lambda_fixp = GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN;
    return GMON_RESP_OK;
}

gMonStatus staActuatorDeinitGenericFan(void) { return GMON_RESP_OK; }

gMonStatus staActuatorInitGenericBulb(gMonActuator_t *dev) {
    if (dev == NULL)
        return GMON_RESP_ERRARGS;
    staSetTrigThresholdBulb(dev, (unsigned int)GMON_CFG_ACTUATOR_TRIG_THRESHOLD_BULB);
    dev->status = GMON_OUT_DEV_STATUS_OFF;
    dev->ema.last_aggregated = 0;
    dev->max_worktime = 0;
    dev->min_resttime = GMON_CFG_ACTUATOR_MIN_RESTTIME_BULB;
    dev->sensor_id_mask = GMON_CFG_ACTUATOR_SENSOR_MASK_BULB;
    dev->ema.lambda_fixp = GMON_CFG_ACTUATOR_EMA_LAMBDA_BULB;
    // blub device is mapped to light sensor, which is read in about every 10 to 30 minutes
    gmon_bulb_max_worktime_default = GMON_CFG_ACTUATOR_MAX_WORKTIME_BULB;
    return GMON_RESP_OK;
}

gMonStatus staActuatorDeinitGenericBulb(void) { return GMON_RESP_OK; }

static gMonStatus staSetTrigThresholdGenericActuator(
    gMonActuator_t *dev, unsigned int new_val, unsigned int max_thre, unsigned int min_thre
) {
    gMonStatus status = GMON_RESP_OK;
    if (dev != NULL) {
        if (new_val >= min_thre && new_val <= max_thre) {
            dev->threshold = new_val;
        } else {
            status = GMON_RESP_INVALID_REQ;
        }
    } else {
        status = GMON_RESP_ERRARGS;
    }
    return status;
}

gMonStatus staSetTrigThresholdPump(gMonActuator_t *dev, unsigned int new_val) {
    return staSetTrigThresholdGenericActuator(
        dev, new_val, (unsigned int)GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP,
        (unsigned int)GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP
    );
}

gMonStatus staSetTrigThresholdFan(gMonActuator_t *dev, unsigned int new_val) {
    return staSetTrigThresholdGenericActuator(
        dev, new_val, (unsigned int)GMON_MAX_ACTUATOR_TRIG_THRESHOLD_FAN,
        (unsigned int)GMON_MIN_ACTUATOR_TRIG_THRESHOLD_FAN
    );
}

gMonStatus staSetTrigThresholdBulb(gMonActuator_t *dev, unsigned int new_val) {
    return staSetTrigThresholdGenericActuator(
        dev, new_val, (unsigned int)GMON_MAX_ACTUATOR_TRIG_THRESHOLD_BULB,
        (unsigned int)GMON_MIN_ACTUATOR_TRIG_THRESHOLD_BULB
    );
}

gMonStatus staActuatorAggregateU32(gmonEvent_t *evt, gMonActuator_t *dev, int *value) {
    if (evt == NULL || dev == NULL || value == NULL)
        return GMON_RESP_ERRARGS;
    if (evt->num_active_sensors == 0x0 || dev->sensor_id_mask == 0x0)
        return GMON_RESP_MALFORMED_DATA;

    unsigned int *event_data_u32 = (unsigned int *)evt->data;
    unsigned int  sum = 0, count = 0, avg = 0;
    for (unsigned char i = 0; i < evt->num_active_sensors; i++) {
        // Aggregate data if the sensor is relevant to the actuator (mask bit set)
        // and its data is not marked as corrupted (corruption flag clear).
        if (staGetBitFlag(&dev->sensor_id_mask, i) && !staGetBitFlag(&evt->flgs.corruption, i)) {
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
    if (evt->num_active_sensors == 0x0 || dev->sensor_id_mask == 0x0)
        return GMON_RESP_MALFORMED_DATA;

    gmonAirCond_t *event_data_ac = (gmonAirCond_t *)evt->data;
    gmonAirCond_t  sum = {0}, avg = {0};
    unsigned int   count = 0;
    unsigned char  i = 0;
    for (i = 0, count = 0; i < evt->num_active_sensors; i++) {
        // Aggregate data if the sensor is relevant to the actuator (mask bit set)
        // and its data is not marked as corrupted (corruption flag clear).
        if (staGetBitFlag(&dev->sensor_id_mask, i) && !staGetBitFlag(&evt->flgs.corruption, i)) {
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
    if (gmon == NULL) {
        return GMON_RESP_ERRARGS;
    }
    // pause the actuators which require precise control
    gMonActuator_t *dev = &gmon->actuator.pump;
    if (dev->status == GMON_OUT_DEV_STATUS_ON) {
        dev->curr_worktime = dev->max_worktime;
        // For pausing, we simply set the status and do not trigger the actuator with a sensor value.
        // The intention of this block is to force a PAUSE state, not to actually trigger the pump.
        // If a trigger based on a sensor is needed here, the sensor object needs to be passed.
        // Manually set to PAUSE for this specific function's logic
        dev->status = GMON_OUT_DEV_STATUS_PAUSE;
    }
    return GMON_RESP_OK;
}

gMonActuatorStatus staActuatorMeasureWorkingTime(gMonActuator_t *dev, unsigned int time_elapsed_ms) {
    gMonActuatorStatus next_status = GMON_OUT_DEV_STATUS_OFF;
    switch (dev->status) {
    case GMON_OUT_DEV_STATUS_OFF:
        if (dev->max_worktime > 0) {
            next_status = GMON_OUT_DEV_STATUS_ON;
            dev->curr_worktime = time_elapsed_ms;
            dev->curr_resttime = 0;
        }
        break;
    case GMON_OUT_DEV_STATUS_ON:
        dev->curr_worktime += time_elapsed_ms;
        if (dev->curr_worktime >= dev->max_worktime) {
            dev->curr_worktime = 0;
            next_status = GMON_OUT_DEV_STATUS_PAUSE;
        } else {
            next_status = GMON_OUT_DEV_STATUS_ON;
        }
        break;
    case GMON_OUT_DEV_STATUS_PAUSE:
        dev->curr_resttime += time_elapsed_ms;
        if (dev->curr_resttime >= dev->min_resttime) {
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
