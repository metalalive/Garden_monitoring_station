#include "station_include.h"

gMonStatus staDaylightTrackInit(gardenMonitor_t *gmon) {
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    return staSetRequiredDaylenTicks(gmon, GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS);
}

gMonStatus staSetRequiredDaylenTicks(gardenMonitor_t *gmon, unsigned int light_length) {
    gMonStatus status = GMON_RESP_OK;
    if (gmon == NULL)
        return GMON_RESP_ERRARGS;
    if (light_length <= GMON_MAX_REQUIRED_LIGHT_LENGTH_TICKS) {
        stationSysEnterCritical();
        gmon->user_ctrl.required_light_daylength_ticks = light_length;
        stationSysExitCritical();
    } else {
        status = GMON_RESP_INVALID_REQ;
    }
    return status;
}

void lightControllerTaskFn(void *params) {
    const uint32_t block_time = 0;
    unsigned int   lightness = 0, curr_ticks = 0, curr_days = 0;
    gMonStatus     status = GMON_RESP_OK;
    gmonEvent_t   *event = NULL;

    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    gMonSensor_t       *sensor = &gmon->sensors.light;
    gmonSensorSample_t *read_vals = staAllocSensorSampleBuffer(sensor, GMON_SENSOR_DATA_TYPE_U32);
    while (1) {
        curr_ticks = stationGetTicksPerDay(&gmon->tick);
        curr_days = stationGetDays(&gmon->tick);
        // Interactively read from light-relevant sensors
        status = GMON_SENSOR_READ_FN_LIGHT(sensor, read_vals);
        if (status == GMON_RESP_OK) {
            lightness = ((unsigned int *)read_vals[0].data)[0];
            event = staAllocSensorEvent(gmon);
            if (event != NULL) {
                event->event_type = GMON_EVENT_LIGHTNESS_UPDATED;
                event->data.lightness = lightness;
                event->curr_ticks = curr_ticks;
                event->curr_days = curr_days;
                // Pass the read data to message pipe
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            // TODO, redesign how to determine max work time of artifical light
            gmon->actuator.bulb.max_worktime = 1230;
            // The interval for bulb will be updated by network handling task during runtime
            status = GMON_ACTUATOR_TRIG_FN_BULB(&gmon->actuator.bulb, lightness, &gmon->sensors.light);
        }
        stationSysDelayMs(gmon->sensors.light.read_interval_ms);
    }
} // end of lightControllerTaskFn
