#include "station_include.h"

void pumpControllerTaskFn(void *params) {
    const uint32_t block_time = 0;
    gMonStatus     status = GMON_RESP_OK;

    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    gMonSensorMeta_t   *sensor = &gmon->sensors.soil_moist;
    gmonSensorSample_t *read_vals = staAllocSensorSampleBuffer(sensor, GMON_SENSOR_DATA_TYPE_U32);
    while (1) {
        // apply configurable delay time for this sensor.
        // The interval will be updated by network handling task during runtime
        stationSysDelayMs(gmon->sensors.soil_moist.read_interval_ms);
        status = GMON_SENSOR_READ_FN_SOIL_MOIST(sensor, read_vals);
        if (status != GMON_RESP_OK)
            continue;
        gmonEvent_t *event =
            staAllocSensorEvent(&gmon->sensors.event, GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor->num_items);
        if (event == NULL)
            continue;
        status = staSensorSampleToEvent(event, read_vals);
        XASSERT(status == GMON_RESP_OK)
        status = GMON_ACTUATOR_TRIG_FN_PUMP(&gmon->actuator.pump, event, &gmon->sensors.soil_moist);
        event->curr_ticks = stationGetTicksPerDay(&gmon->tick);
        event->curr_days = stationGetDays(&gmon->tick);
        staNotifyOthersWithEvent(gmon, event, block_time);
    }
} // end of pumpControllerTaskFn
