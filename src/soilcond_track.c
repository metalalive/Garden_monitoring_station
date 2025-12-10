#include "station_include.h"

void pumpControllerTaskFn(void *params) {
    const uint32_t block_time = 0;
    unsigned int   soil_moist = 0, curr_ticks = 0, curr_days = 0;
    gMonStatus     status = GMON_RESP_OK;
    gmonEvent_t   *event = NULL;

    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    gMonSensor_t       *sensor = &gmon->sensors.soil_moist;
    gmonSensorSample_t *read_vals = staAllocSensorSampleBuffer(sensor, GMON_SENSOR_DATA_TYPE_U32);
    while (1) {
        curr_ticks = stationGetTicksPerDay(&gmon->tick);
        curr_days = stationGetDays(&gmon->tick);

        status = GMON_SENSOR_READ_FN_SOIL_MOIST(sensor, read_vals);
        if (status == GMON_RESP_OK) {
            soil_moist = ((unsigned int *)read_vals[0].data)[0]; // TODO
            event = staAllocSensorEvent(&gmon->sensors.event);
            if (event != NULL) {
                event->event_type = GMON_EVENT_SOIL_MOISTURE_UPDATED;
                event->data.soil_moist = soil_moist;
                event->curr_ticks = curr_ticks;
                event->curr_days = curr_days;
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            status = GMON_ACTUATOR_TRIG_FN_PUMP(&gmon->actuator.pump, soil_moist, &gmon->sensors.soil_moist);
        }
        // apply configurable delay time for this sensor.
        // The interval will be updated by network handling task during runtime
        stationSysDelayMs(gmon->sensors.soil_moist.read_interval_ms);
    }
} // end of pumpControllerTaskFn
