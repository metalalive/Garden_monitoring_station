#include "station_include.h"

// The parameters below are used to store default maximum continuously working time of user's bulb
// , since the maximum working time of a bulb would be adjust every 24 hours for unstable natural light environment
// e.g. the plant you grow need 7 hours growing light every day but your garden can only provide 4 hours natural
//      light per day
// TODO, recheck if It's necessary to use this
static unsigned int gmon_bulb_max_worktime_default;

gMonStatus  staOutdevInitGenericPump(gMonOutDev_t *dev) {
    if(dev == NULL) { return GMON_RESP_ERRARGS; }
    staSetTrigThresholdPump(dev, (unsigned int)GMON_CFG_OUTDEV_TRIG_THRESHOLD_PUMP);
    dev->status        = GMON_OUT_DEV_STATUS_OFF;
    dev->max_worktime  = GMON_CFG_OUTDEV_MAX_WORKTIME_PUMP;
    dev->min_resttime  = GMON_CFG_OUTDEV_MIN_RESTTIME_PUMP;
    return GMON_RESP_OK;
}

gMonStatus  staOutdevDeinitGenericPump(void) {
    return GMON_RESP_OK;
}

gMonStatus  staOutdevInitGenericFan(gMonOutDev_t *dev) {
    if(dev == NULL) { return GMON_RESP_ERRARGS; }
    staSetTrigThresholdFan(dev, (unsigned int)GMON_CFG_OUTDEV_TRIG_THRESHOLD_FAN);
    dev->status       = GMON_OUT_DEV_STATUS_OFF;
    dev->max_worktime = GMON_CFG_OUTDEV_MAX_WORKTIME_FAN;
    dev->min_resttime = GMON_CFG_OUTDEV_MIN_RESTTIME_FAN;
    return GMON_RESP_OK;
}

gMonStatus  staOutdevDeinitGenericFan(void) {
    return GMON_RESP_OK;
}

gMonStatus  staOutdevInitGenericBulb(gMonOutDev_t *dev) {
    if(dev == NULL) { return GMON_RESP_ERRARGS; }
    staSetTrigThresholdBulb(dev, (unsigned int)GMON_CFG_OUTDEV_TRIG_THRESHOLD_BULB);
    dev->status        = GMON_OUT_DEV_STATUS_OFF;
    dev->max_worktime  = 0;
    dev->min_resttime  = GMON_CFG_OUTDEV_MIN_RESTTIME_BULB;
    // blub device is mapped to light sensor, which is read in about every 10 to 30 minutes
    gmon_bulb_max_worktime_default = GMON_CFG_OUTDEV_MAX_WORKTIME_BULB;
    return GMON_RESP_OK;
}

gMonStatus  staOutdevDeinitGenericBulb(void) {
    return GMON_RESP_OK;
}

static gMonStatus  staSetTrigThresholdGenericOutdev(gMonOutDev_t *dev, unsigned int new_val, unsigned int max_thre, unsigned int min_thre)
{
    gMonStatus status = GMON_RESP_OK;
    if(dev != NULL) {
        if(new_val >= min_thre && new_val <= max_thre) {
            dev->threshold = new_val;
        } else {
            status = GMON_RESP_INVALID_REQ;
        }
    } else {
        status = GMON_RESP_ERRARGS;
    }
    return status;
} // end of staSetTrigThresholdGenericOutdev


gMonStatus  staSetTrigThresholdPump(gMonOutDev_t *dev, unsigned int new_val) {
    return  staSetTrigThresholdGenericOutdev(dev, new_val, 
                (unsigned int)GMON_MAX_OUTDEV_TRIG_THRESHOLD_PUMP,
                (unsigned int)GMON_MIN_OUTDEV_TRIG_THRESHOLD_PUMP);
}

gMonStatus  staSetTrigThresholdFan(gMonOutDev_t *dev, unsigned int new_val) {
    return  staSetTrigThresholdGenericOutdev(dev, new_val,
                (unsigned int)GMON_MAX_OUTDEV_TRIG_THRESHOLD_FAN,
                (unsigned int)GMON_MIN_OUTDEV_TRIG_THRESHOLD_FAN);
}
 
gMonStatus  staSetTrigThresholdBulb(gMonOutDev_t *dev, unsigned int new_val) {
    return  staSetTrigThresholdGenericOutdev(dev, new_val,
                (unsigned int)GMON_MAX_OUTDEV_TRIG_THRESHOLD_BULB,
                (unsigned int)GMON_MIN_OUTDEV_TRIG_THRESHOLD_BULB);
}

gMonStatus  staPauseWorkingRealtimeOutdevs(gardenMonitor_t *gmon) {
    if(gmon == NULL) { return GMON_RESP_ERRARGS; }
    gMonOutDev_t  *dev = NULL;
    gMonStatus  status = GMON_RESP_OK;
    dev = &gmon->outdev.pump;
    if(dev->status == GMON_OUT_DEV_STATUS_ON) {
        dev->curr_worktime = dev->max_worktime;
        status = GMON_OUTDEV_TRIG_FN_PUMP(dev, (dev->threshold + 1));
        XASSERT(dev->status == GMON_OUT_DEV_STATUS_PAUSE);
    }
    return status;
}

gMonOutDevStatus  staOutDevMeasureWorkingTime(gMonOutDev_t *dev) {
    gMonOutDevStatus dev_status = GMON_OUT_DEV_STATUS_OFF;
    switch(dev->status) {
        case GMON_OUT_DEV_STATUS_OFF:
            if(dev->max_worktime > 0) {
                dev_status = GMON_OUT_DEV_STATUS_ON;
                dev->curr_worktime = dev->sensor_read_interval;
                dev->curr_resttime = 0;
            }
            break;
        case GMON_OUT_DEV_STATUS_ON:
            dev->curr_worktime += dev->sensor_read_interval;
            if(dev->curr_worktime >= dev->max_worktime) {
                dev->curr_worktime = 0;
                dev_status = GMON_OUT_DEV_STATUS_PAUSE;
            } else {
                dev_status = GMON_OUT_DEV_STATUS_ON;
            }
            break;
        case GMON_OUT_DEV_STATUS_PAUSE:
            dev->curr_resttime += dev->sensor_read_interval;
            if(dev->curr_resttime >= dev->min_resttime) {
                dev->curr_resttime = 0;
                dev_status = GMON_OUT_DEV_STATUS_ON;
            } else {
                dev_status = GMON_OUT_DEV_STATUS_PAUSE;
            }
            break;
        default:
            dev_status = GMON_OUT_DEV_STATUS_OFF;
            break;
    } // end of switch case
    return dev_status;
} // end of staOutDevMeasureWorkingTime

