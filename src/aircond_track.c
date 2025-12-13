#include "station_include.h"

void airQualityMonitorTaskFn(void *params) {
    const uint32_t      block_time = 0;
    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    gMonSensorMeta_t   *sensor = &gmon->sensors.air_temp;
    gmonSensorSample_t *read_vals = staAllocSensorSampleBuffer(sensor, GMON_SENSOR_DATA_TYPE_AIRCOND);
    while (1) {
        // The interval for fan will be updated by network handling task during runtime
        stationSysDelayMs(gmon->sensors.air_temp.read_interval_ms);
        gMonStatus status = GMON_SENSOR_READ_FN_AIR_TEMP(sensor, read_vals);
        if (status != GMON_RESP_OK)
            continue;
        status = staSensorDetectNoise(sensor, read_vals);
        if (status != GMON_RESP_OK)
            continue;
        gmonEvent_t *event =
            staAllocSensorEvent(&gmon->sensors.event, GMON_EVENT_AIR_TEMP_UPDATED, sensor->num_items);
        if (event == NULL)
            continue;
        status = staSensorSampleToEvent(event, read_vals);
        XASSERT(status == GMON_RESP_OK)
        // TODO, calibration, reference point from remote user request
        status = GMON_ACTUATOR_TRIG_FN_FAN(&gmon->actuator.fan, event, &gmon->sensors.air_temp);
        XASSERT(status == GMON_RESP_OK);
        event->curr_ticks = stationGetTicksPerDay(&gmon->tick);
        event->curr_days = stationGetDays(&gmon->tick);
        staNotifyOthersWithEvent(gmon, event, block_time);
    }
}
