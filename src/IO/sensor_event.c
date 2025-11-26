#include "station_include.h"

gmonEvent_t* staAllocSensorEvent(gardenMonitor_t *gmon) {
    gmonEvent_t *out = NULL;
    uint16_t  idx = 0;
    stationSysEnterCritical();
    for(idx = 0; idx < gmon->sensors.event.len; idx++) {
        if(gmon->sensors.event.pool[idx].flgs.alloc == 0) {
            out = &gmon->sensors.event.pool[idx];
            XMEMSET(out, 0x00, sizeof(gmonEvent_t));
            out->flgs.alloc = 1;
            break;
        }
    }
    stationSysExitCritical();
    return out;
}

gMonStatus  staFreeSensorEvent(gardenMonitor_t *gmon, gmonEvent_t* record) {
    gmonEvent_t *addr_start = NULL, *addr_end  = NULL;
    gMonStatus status = GMON_RESP_OK;
    uint16_t  idx = 0;
    stationSysEnterCritical();
    addr_start = &gmon->sensors.event.pool[0];
    addr_end   = &gmon->sensors.event.pool[gmon->sensors.event.len];
    if((record < addr_start) || (addr_end <= record)) {
        status = GMON_RESP_ERRMEM;
        goto done;
    }
    for(idx = 0; idx < gmon->sensors.event.len; idx++) {
        addr_start = &gmon->sensors.event.pool[idx];
        addr_end   = &gmon->sensors.event.pool[idx + 1];
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
    gardenMonitor_t *gmon, stationSysMsgbox_t msgbuf, gmonEvent_t *msg, uint32_t block_time
) {
    gMonStatus status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
    if(status != GMON_RESP_OK) {
        // if message box is full, abandon the oldest item of the message box, ensure all
        // available items in the message box are up-to-date.
        gmonEvent_t  *oldest_record = NULL;
        status = staSysMsgBoxGet(msgbuf, (void **)&oldest_record, 0); // Use 0 for block_time to immediately get
        XASSERT((status == GMON_RESP_OK) && (oldest_record != NULL));
        staFreeSensorEvent(gmon, oldest_record);
        // try adding new record again
        status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
        XASSERT(status == GMON_RESP_OK);
    }
    return status;
}

gMonStatus  staNotifyOthersWithEvent(gardenMonitor_t *gmon, gmonEvent_t *event, uint32_t block_time) {
    gmonEvent_t  *event_copy = staAllocSensorEvent(gmon);
    gMonStatus status = staCpySensorEvent(event_copy, event, (size_t)0x1);
    staAddEventToMsgPipe(gmon, gmon->msgpipe.sensor2display, event, block_time);
    staAddEventToMsgPipe(gmon, gmon->msgpipe.sensor2net, event_copy, block_time);
    return status;
}

gMonStatus  stationIOinit(gardenMonitor_t *gmon) {
    gMonStatus status = GMON_RESP_OK;
    if(gmon == NULL) { status = GMON_RESP_ERRARGS; goto done;}
    // Initialize default sensor reading intervals
    gmon->sensors.event.pool = (gmonEvent_t *)XMALLOC(sizeof(gmonEvent_t) * GMON_NUM_SENSOR_EVENTS);
    if (gmon->sensors.event.pool == NULL) {
        status = GMON_RESP_ERRMEM;
        goto done;
    }
    gmon->sensors.event.len = GMON_NUM_SENSOR_EVENTS;
    XMEMSET(gmon->sensors.event.pool, 0x00, sizeof(gmonEvent_t) * GMON_NUM_SENSOR_EVENTS);

    gmon->sensors.soil_moist.read_interval_ms = GMON_CFG_SENSOR_READ_INTERVAL_MS;
    gmon->sensors.air_temp.read_interval_ms   = GMON_CFG_SENSOR_READ_INTERVAL_MS;
    gmon->sensors.light.read_interval_ms      = GMON_CFG_SENSOR_READ_INTERVAL_MS;
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
    gmon->msgpipe.sensor2display = staSysMsgBoxCreate( GMON_CFG_NUM_SENSOR_RECORDS_KEEP );
    XASSERT(gmon->msgpipe.sensor2display != NULL);
    gmon->msgpipe.sensor2net = staSysMsgBoxCreate( GMON_CFG_NUM_SENSOR_RECORDS_KEEP );
    XASSERT(gmon->msgpipe.sensor2net != NULL);
done:
    if (status != GMON_RESP_OK && gmon != NULL && gmon->sensors.event.pool != NULL) {
        XMEMFREE(gmon->sensors.event.pool);
        gmon->sensors.event.pool = NULL;
        gmon->sensors.event.len = 0;
    }
    return status;
} // end of stationIOinit


gMonStatus  stationIOdeinit(gardenMonitor_t *gmon) {
    gMonStatus status = GMON_RESP_OK;
    gmonEvent_t  *event_to_free = NULL;

    // Free any remaining events in the message queues
    while(staSysMsgBoxGet(gmon->msgpipe.sensor2display, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(gmon, event_to_free);
        event_to_free = NULL;
    }
    while(staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(gmon, event_to_free);
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
    if (gmon != NULL && gmon->sensors.event.pool != NULL) {
        XMEMFREE(gmon->sensors.event.pool);
        gmon->sensors.event.pool = NULL;
        gmon->sensors.event.len = 0;
    }
    return status;
} // end of stationIOdeinit
