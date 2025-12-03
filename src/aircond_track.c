#include "station_include.h"

void airQualityMonitorTaskFn(void *params) {
    const uint32_t      block_time = 0;
    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    gMonSensor_t       *sensor = &gmon->sensors.air_temp;
    gmonSensorSample_t *read_vals = staAllocSensorSampleBuffer(sensor, GMON_SENSOR_DATA_TYPE_AIRCOND);
    while (1) {
        gMonStatus status = GMON_SENSOR_READ_FN_AIR_TEMP(sensor, read_vals);
        if (status == GMON_RESP_OK) {
            gmonAirCond_t *newread = &((gmonAirCond_t *)read_vals[0].data)[0]; // TODO
            gmonEvent_t   *event = staAllocSensorEvent(gmon);
            if (event != NULL) {
                // TODO, calibration, reference point from remote user request
                event->event_type = GMON_EVENT_AIR_TEMP_UPDATED;
                event->data.air_cond.temporature = newread->temporature;
                event->data.air_cond.humidity = newread->humidity;
                event->curr_ticks = stationGetTicksPerDay(&gmon->tick);
                event->curr_days = stationGetDays(&gmon->tick);
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            // The interval for fan will be updated by network handling task during runtime
            status =
                GMON_ACTUATOR_TRIG_FN_FAN(&gmon->actuator.fan, newread->temporature, &gmon->sensors.air_temp);
        }
        stationSysDelayMs(gmon->sensors.air_temp.read_interval_ms);
    }
}
