#include "station_include.h"

void pumpControllerTaskFn(void* params) {
    const uint32_t  block_time = 0;
    unsigned int    soil_moist = 0, curr_ticks = 0, curr_days = 0;
    gMonStatus      status = GMON_RESP_OK;
    gmonEvent_t     *event = NULL;

    gardenMonitor_t *gmon = (gardenMonitor_t *)params;
    while(1) {
        curr_ticks = stationGetTicksPerDay();
        curr_days  = stationGetDays();

        // 1. Read Soil Moisture and create an event
        status = GMON_SENSOR_READ_FN_SOIL_MOIST(&soil_moist);
        if(status == GMON_RESP_OK) {
            event = staAllocSensorEvent();
            if (event != NULL) {
                event->event_type      = GMON_EVENT_SOIL_MOISTURE_UPDATED;
                event->data.soil_moist = soil_moist;
                event->curr_ticks      = curr_ticks;
                event->curr_days       = curr_days;
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
            status = GMON_OUTDEV_TRIG_FN_PUMP(&gmon->outdev.pump, soil_moist);
        }
        // apply configurable delay time for this sensor.
        // The interval for pump will be updated by network handling task during runtime
        stationSysDelayMs(gmon->outdev.pump.sensor_read_interval);
    }
}
