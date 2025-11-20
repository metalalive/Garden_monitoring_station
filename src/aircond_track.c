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
                event->data.air_temp = air_temp;
                event->data.air_humid = air_humid;
                event->curr_ticks = stationGetTicksPerDay();
                event->curr_days = stationGetDays();
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            gmon->outdev.fan.sensor_read_interval = 123; // FIXME, TODO, remove
            status = GMON_OUTDEV_TRIG_FN_FAN(&gmon->outdev.fan , air_temp);
        }
        // apply configurable delay time which will be updated by network handling task
        stationSysDelayMs(gmon->outdev.fan.sensor_read_interval);
    }
}
