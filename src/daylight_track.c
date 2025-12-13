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
    gMonStatus     status = GMON_RESP_OK;
    gmonEvent_t   *event = NULL;

    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    gMonSensorMeta_t   *sensor = &gmon->sensors.light;
    gmonSensorSample_t *read_vals = staAllocSensorSampleBuffer(sensor, GMON_SENSOR_DATA_TYPE_U32);
    while (1) {
        // The interval for bulb will be updated by network handling task during runtime
        stationSysDelayMs(gmon->sensors.light.read_interval_ms);
        // Interactively read from light-relevant sensors
        status = GMON_SENSOR_READ_FN_LIGHT(sensor, read_vals);
        if (status != GMON_RESP_OK)
            continue;
        event = staAllocSensorEvent(&gmon->sensors.event, GMON_EVENT_LIGHTNESS_UPDATED, sensor->num_items);
        if (event == NULL)
            continue;
        status = staSensorSampleToEvent(event, read_vals);
        XASSERT(status == GMON_RESP_OK);
        // TODO, redesign how to determine max work time of artifical light
        gmon->actuator.bulb.max_worktime = 1230;
        status = GMON_ACTUATOR_TRIG_FN_BULB(&gmon->actuator.bulb, event, &gmon->sensors.light);
        event->curr_ticks = stationGetTicksPerDay(&gmon->tick);
        event->curr_days = stationGetDays(&gmon->tick);
        // Pass the read data to message pipe
        staNotifyOthersWithEvent(gmon, event, block_time);
    }
} // end of lightControllerTaskFn
