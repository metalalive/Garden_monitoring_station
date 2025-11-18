#include "station_include.h"

#define  GMON_SENSOR_RECORD_LIST_SZ   (GMON_CFG_NUM_SENSOR_RECORDS_KEEP << 1)

static gmonEvent_t   gmon_avail_record_list[GMON_SENSOR_RECORD_LIST_SZ];

static gmonEvent_t* staAllocSensorEvent(void) {
    gmonEvent_t *out = NULL;
    uint16_t  idx = 0;
    stationSysEnterCritical();
    for(idx = 0; idx < GMON_SENSOR_RECORD_LIST_SZ; idx++) {
        if(gmon_avail_record_list[idx].flgs.alloc == 0) {
            out = &gmon_avail_record_list[idx];
            XMEMSET(out, 0x00, sizeof(gmonEvent_t));
            out->flgs.alloc = 1;
            break;
        }
    }
    stationSysExitCritical();
    XASSERT(out != NULL); // check whether there's no item available
    return out;
}

gMonStatus  staFreeSensorEvent(gmonEvent_t* record) {
    gmonEvent_t *addr_start = NULL, *addr_end  = NULL;
    gMonStatus status = GMON_RESP_OK;
    uint16_t  idx = 0;
    stationSysEnterCritical();
    addr_start = &gmon_avail_record_list[0];
    addr_end   = &gmon_avail_record_list[GMON_SENSOR_RECORD_LIST_SZ];
    if((record < addr_start) || (addr_end <= record)) {
        status = GMON_RESP_ERRMEM;
        goto done;
    }
    for(idx = 0; idx < GMON_SENSOR_RECORD_LIST_SZ; idx++) {
        addr_start = &gmon_avail_record_list[idx];
        addr_end   = &gmon_avail_record_list[idx + 1];
        if((record > addr_start) && (record < addr_end)) {
            status = GMON_RESP_ERRMEM; // memory not aligned
            break;
        } else if (record == addr_start) {
            if(addr_start->flgs.alloc != 0) {
                addr_start->flgs.alloc = 0;
            }
            break;
        }
    }
done:
    stationSysExitCritical();
    return status;
}

gMonStatus  staCpySensorEvent(gmonEvent_t  *dst, gmonEvent_t *src, size_t  sz) {
    unsigned int  idx = 0;
    gMonStatus status = GMON_RESP_OK;
    if(dst == NULL || src == NULL || sz == 0) {
        return GMON_RESP_ERRARGS;
    }
    for(idx = 0; idx < sz; idx++) {
        if(dst[idx].flgs.alloc == 0 || src[idx].flgs.alloc == 0) {
            status = GMON_RESP_ERRMEM;
            break;
        }
    }
    if(status == GMON_RESP_OK) {
        XMEMCPY(dst, src, sizeof(gmonEvent_t) * sz);
    }
    return status;
}


static gMonStatus  staAddEventToMsgPipe(
    stationSysMsgbox_t msgbuf, gmonEvent_t *msg, uint32_t block_time
) {
    gMonStatus status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
    if(status != GMON_RESP_OK) {
        // if message box is full, abandon the oldest item of the message box, ensure all
        // available items in the message box are up-to-date.
        gmonEvent_t  *oldest_record = NULL;
        status = staSysMsgBoxGet(msgbuf, (void **)&oldest_record, block_time);
        XASSERT((status == GMON_RESP_OK) && (oldest_record != NULL));
        staFreeSensorEvent(oldest_record);
        // try adding new record again
        status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
        XASSERT(status == GMON_RESP_OK);
    }
    return status;
}

static gMonStatus  staNotifyOthersWithEvent(gardenMonitor_t *gmon, gmonEvent_t *event, uint32_t block_time) {
    gmonEvent_t  *event_copy = staAllocSensorEvent();
    gMonStatus status = staCpySensorEvent(event_copy, event, (size_t)0x1);
    staAddEventToMsgPipe(gmon->msgpipe.sensor2display, event, block_time);
    staAddEventToMsgPipe(gmon->msgpipe.sensor2net, event_copy, block_time);
    return status;
}

static void staAdjustSensorReadInterval(gardenMonitor_t *gmon) {
    unsigned int new_refresh_time = 0;
    //// bulb_status = gmon->outdev.bulb.status;
    gMonOutDevStatus  pump_status = gmon->outdev.pump.status;
    gMonOutDevStatus  fan_status  = gmon->outdev.fan.status;
    if(pump_status == GMON_OUT_DEV_STATUS_ON || pump_status == GMON_OUT_DEV_STATUS_PAUSE) {
        new_refresh_time = GMON_SENSOR_READ_INTERVAL_MS_PUMP_ON;
    } else if (fan_status == GMON_OUT_DEV_STATUS_ON || fan_status == GMON_OUT_DEV_STATUS_PAUSE) {
        new_refresh_time = GMON_SENSOR_READ_INTERVAL_MS_FAN_ON;
    } else { // TODO: let users modify default time interval to refresh sensor data through backend service
        new_refresh_time = gmon->sensor_read_interval.default_ms;
    }
    if(new_refresh_time != gmon->sensor_read_interval.curr_ms) {
        gmon->sensor_read_interval.curr_ms = new_refresh_time;
        gmon->outdev.pump.sensor_read_interval = new_refresh_time;
    }
}

gMonStatus  staSetDefaultSensorReadInterval(gardenMonitor_t *gmon, unsigned int new_interval)
{
    gMonStatus status = GMON_RESP_OK;
    if(gmon != NULL) {
        if(new_interval >= GMON_MIN_SENSOR_READ_INTERVAL_MS && new_interval <= GMON_MAX_SENSOR_READ_INTERVAL_MS) {
            gmon->sensor_read_interval.default_ms = new_interval;
        } else {
            status = GMON_RESP_INVALID_REQ;
        }
    } else {
        status = GMON_RESP_ERRARGS;
    }
    return status;
}

gMonStatus  stationIOinit(gardenMonitor_t *gmon) {
    gMonStatus status = GMON_RESP_OK;
    if(gmon == NULL) { status = GMON_RESP_ERRARGS; goto done;}
    status = staSetDefaultSensorReadInterval(gmon, (unsigned int)GMON_CFG_SENSOR_READ_INTERVAL_MS);
    if(status < 0) { goto done; }
    status = GMON_SENSOR_INIT_FN_SOIL_MOIST();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_INIT_FN_LIGHT();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_INIT_FN_AIR_TEMP();
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_INIT_FN_PUMP(&gmon->outdev.pump);
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_INIT_FN_FAN(&gmon->outdev.fan);
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_INIT_FN_BULB(&gmon->outdev.bulb);
    if(status < 0) { goto done; }
    staAdjustSensorReadInterval(gmon);
    gmon->msgpipe.sensor2display = staSysMsgBoxCreate( GMON_CFG_NUM_SENSOR_RECORDS_KEEP );
    XASSERT(gmon->msgpipe.sensor2display != NULL);
    gmon->msgpipe.sensor2net = staSysMsgBoxCreate( GMON_CFG_NUM_SENSOR_RECORDS_KEEP );
    XASSERT(gmon->msgpipe.sensor2net != NULL);
    XMEMSET(&gmon_avail_record_list[0], 0x00, sizeof(gmonEvent_t) * GMON_SENSOR_RECORD_LIST_SZ);
done:
    return status;
} // end of stationIOinit


gMonStatus  stationIOdeinit(gardenMonitor_t *gmon) {
    gMonStatus status = GMON_RESP_OK;
    gmonEvent_t  *event_to_free = NULL;

    // Free any remaining events in the message queues
    while(staSysMsgBoxGet(gmon->msgpipe.sensor2display, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(event_to_free);
        event_to_free = NULL;
    }
    while(staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(event_to_free);
        event_to_free = NULL;
    }

    staSysMsgBoxDelete( &gmon->msgpipe.sensor2display );
    XASSERT(gmon->msgpipe.sensor2display == NULL);
    staSysMsgBoxDelete( &gmon->msgpipe.sensor2net );
    XASSERT(gmon->msgpipe.sensor2net == NULL);

    // Deinitialize output devices and sensors
    status = GMON_OUTDEV_DEINIT_FN_BULB();
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_DEINIT_FN_FAN();
    if(status < 0) { goto done; }
    status = GMON_OUTDEV_DEINIT_FN_PUMP();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_DEINIT_FN_SOIL_MOIST();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_DEINIT_FN_LIGHT();
    if(status < 0) { goto done; }
    status = GMON_SENSOR_DEINIT_FN_AIR_TEMP();
done:
    return status;
} // end of stationIOdeinit


void  stationSensorReaderTaskFn(void* params) {
    const uint32_t  block_time = 0;
    unsigned int    soil_moist = 0, lightness = 0,  curr_ticks = 0,  curr_days  = 0;
    float  air_temp   = 0.f,  air_humid  = 0.f;
    gMonStatus status = GMON_RESP_OK;
    gmonEvent_t        *event = NULL;

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
        }

        // 2. Read Lightness and create an event (based on the instruction, lightness is valid for THRESHOLD_EXCEEDED)
        status = staDaylightTrackRefreshSensorData(&lightness);
        if(status == GMON_RESP_OK) {
            event = staAllocSensorEvent();
            if (event != NULL) {
                event->event_type      = GMON_EVENT_LIGHTNESS_UPDATED;
                event->data.lightness  = lightness;
                event->curr_ticks      = curr_ticks;
                event->curr_days       = curr_days;
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
        }

        // 3. Read Air Temperature and Humidity and create an event
        status = staAirCondTrackRefreshSensorData(&air_temp, &air_humid);
        if(status == GMON_RESP_OK) {
            event = staAllocSensorEvent();
            if (event != NULL) {
                event->event_type      = GMON_EVENT_AIR_TEMP_UPDATED;
                event->data.air_temp   = air_temp ;
                event->data.air_humid  = air_humid;
                event->curr_ticks      = curr_ticks;
                event->curr_days       = curr_days;
                staNotifyOthersWithEvent(gmon, event, block_time);
            }
        }

        stationSysDelayMs(gmon->sensor_read_interval.curr_ms); // sleep for a while
        // check any of output device is working, then reduce the daley time of this task
        // e.g. pump is working, so it needs to refresh data from soil moisture sensor more frequently.
        staAdjustSensorReadInterval(gmon);
    }
} // end of stationSensorReaderTaskFn

