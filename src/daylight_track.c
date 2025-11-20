#include "station_include.h"

gMonStatus  staDaylightTrackInit(gardenMonitor_t *gmon) {
    gMonStatus  status = GMON_RESP_OK;
    if (gmon == NULL) { return GMON_RESP_ERRARGS; }
    status = staSetRequiredDaylenTicks(gmon, GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS);
    return status;
}

gMonStatus  staSetRequiredDaylenTicks(gardenMonitor_t *gmon, unsigned int light_length) {
    gMonStatus  status = GMON_RESP_OK;
    if (gmon == NULL) { return GMON_RESP_ERRARGS; }
    if(light_length <= GMON_MAX_REQUIRED_LIGHT_LENGTH_TICKS) {
        stationSysEnterCritical();
        gmon->user_ctrl.required_light_daylength_ticks = light_length;
        stationSysExitCritical();
    } else {
        status = GMON_RESP_INVALID_REQ;
    }
    return status;
}

void lightControllerTaskFn(void* params) {
    const uint32_t  block_time = 0;
    unsigned int    lightness = 0,  curr_ticks = 0,  curr_days  = 0;
    gMonStatus status = GMON_RESP_OK;
    gmonEvent_t        *event = NULL;

    gardenMonitor_t *gmon = (gardenMonitor_t *)params;
    while(1) {
        curr_ticks = stationGetTicksPerDay();
        curr_days  = stationGetDays();
        // Interactively read from light-relevant sensors
        status = GMON_SENSOR_READ_FN_LIGHT(&lightness);
        if(status == GMON_RESP_OK) {
            event = staAllocSensorEvent();
            if (event != NULL) {
                event->event_type      = GMON_EVENT_LIGHTNESS_UPDATED;
                event->data.lightness  = lightness;
                event->curr_ticks      = curr_ticks;
                event->curr_days       = curr_days;
                // Pass the read data to message pipe
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            // FIXME, TODO,
            // - remove below
            // - redesign how to determine max work time of artifical light
            gmon->outdev.bulb.sensor_read_interval = 123;
            gmon->outdev.bulb.max_worktime = 1230;
            status = GMON_OUTDEV_TRIG_FN_BULB(&gmon->outdev.bulb, lightness);
        }
        // apply configurable delay time which will be updated by network handling task
        stationSysDelayMs(gmon->outdev.bulb.sensor_read_interval);
    }
}
