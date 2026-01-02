#include "station_include.h"

void pumpControllerTaskFn(void *params) {
    const uint32_t block_time = 0;
    gMonStatus     status = GMON_RESP_OK;

    gardenMonitor_t      *gmon = (gardenMonitor_t *)params;
    gMonSoilSensorMeta_t *sensor = &gmon->sensors.soil_moist;
    gmonSensorSamples_t   read_vals =
        staAllocSensorSampleBuffer((gmonSensorSamples_t){0}, &sensor->super, GMON_SENSOR_DATA_TYPE_U32);
    while (1) {
        // apply configurable delay time for this sensor.
        // The interval will be updated by network handling task during runtime
        stationSysDelayMs(staSensorReadInterval(sensor));
        staSensorRefreshFastPollRatio(sensor);
        read_vals = staAllocSensorSampleBuffer(read_vals, &sensor->super, GMON_SENSOR_DATA_TYPE_U32);
        if (read_vals.entries == NULL)
            continue;
        status = GMON_SENSOR_READ_FN_SOIL_MOIST(sensor, read_vals.entries);
        if (status != GMON_RESP_OK)
            continue;
        gmonEvent_t *event = staAllocSensorEvent(
            &gmon->sensors.event, GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor->super.num_items
        );
        if (event == NULL)
            continue;
        status = staSensorSampleToEvent(event, read_vals.entries);
        XASSERT(status == GMON_RESP_OK)
        status = GMON_ACTUATOR_TRIG_FN_PUMP(&gmon->actuator.pump, event, sensor);
        // always pass event to message pipe regardless of actuator's return value
        event->curr_ticks = stationGetTicksPerDay(&gmon->tick);
        event->curr_days = stationGetDays(&gmon->tick);
        staNotifyOthersWithEvent(gmon, event, block_time);
    }
} // end of pumpControllerTaskFn
