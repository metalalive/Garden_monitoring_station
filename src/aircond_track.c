#include "station_include.h"

gMonStatus  staAirCondTrackInit(void) {
    return GMON_RESP_OK;
}

void airQualityMonitorTaskFn(void* params) {
    float air_temp = 0.f, air_humid = 0.f;

    const uint32_t block_time = 0;
    gardenMonitor_t *gmon = (gardenMonitor_t *)params;
    while(1) {
        gMonStatus status = GMON_SENSOR_READ_FN_AIR_TEMP(&air_temp, &air_humid);
        if(status == GMON_RESP_OK) {
            gmonEvent_t *event = staAllocSensorEvent();
            if (event != NULL) {
                // TODO, calibration, reference point from remote user request
                event->event_type = GMON_EVENT_AIR_TEMP_UPDATED;
                event->data.air_cond.temporature = air_temp;
                event->data.air_cond.humidity = air_humid;
                event->curr_ticks = stationGetTicksPerDay();
                event->curr_days = stationGetDays();
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            // The interval for fan will be updated by network handling task during runtime
            status = GMON_OUTDEV_TRIG_FN_FAN(&gmon->outdev.fan , air_temp, &gmon->sensors.air_temp);
        }
        stationSysDelayMs(gmon->sensors.air_temp.read_interval_ms);
    }
}
