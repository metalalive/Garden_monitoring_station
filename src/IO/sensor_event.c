#include "station_include.h"

gmonEvent_t *staAllocSensorEvent(gMonEvtPool_t *epool, gmonEventType_t etyp, unsigned char num_sensors) {
    gmonEvent_t *out = NULL;
    uint16_t     idx = 0;
    unsigned int nbytes_per_sensor = 0;
    if (epool == NULL || num_sensors == 0)
        return NULL;
    switch (etyp) {
    case GMON_EVENT_SOIL_MOISTURE_UPDATED:
    case GMON_EVENT_LIGHTNESS_UPDATED:
        nbytes_per_sensor = sizeof(unsigned int);
        break;
    case GMON_EVENT_AIR_TEMP_UPDATED:
        nbytes_per_sensor = sizeof(gmonAirCond_t);
        break;
    default:
        return NULL;
    }
    stationSysEnterCritical();
    for (idx = 0; idx < epool->len; idx++) {
        if (epool->pool[idx].flgs.alloc == 0) {
            out = &epool->pool[idx];
            XMEMSET(out, 0x00, sizeof(gmonEvent_t));
            out->flgs.alloc = 1;
            break;
        }
    }
    stationSysExitCritical();
    if (out) {
        out->data = XCALLOC(num_sensors, nbytes_per_sensor);
        out->event_type = etyp;
        out->num_active_sensors = num_sensors;
    }
    return out;
}

gMonStatus staFreeSensorEvent(gMonEvtPool_t *epool, gmonEvent_t *record) {
    gmonEvent_t *addr_start = NULL, *addr_end = NULL;
    gMonStatus   status = GMON_RESP_OK;
    uint16_t     idx = 0;
    stationSysEnterCritical();
    addr_start = &epool->pool[0];
    addr_end = &epool->pool[epool->len];
    if ((record < addr_start) || (addr_end <= record)) {
        status = GMON_RESP_ERRMEM;
        goto done;
    }
    for (idx = 0; idx < epool->len; idx++) {
        addr_start = &epool->pool[idx];
        addr_end = &epool->pool[idx + 1];
        if ((record > addr_start) && (record < addr_end)) {
            status = GMON_RESP_ERRMEM; // memory not aligned
            break;
        } else if (record == addr_start) {
            if (record->flgs.alloc != 0)
                record->flgs.alloc = 0;
            break;
        }
    }
done:
    stationSysExitCritical();
    if (status == GMON_RESP_OK) {
        XMEMFREE(record->data);
        record->data = NULL;
    }
    return status;
} // end of staFreeSensorEvent

gMonStatus staCpySensorEvent(gmonEvent_t *dst, gmonEvent_t *src) {
    if (dst == NULL || src == NULL)
        return GMON_RESP_ERRARGS;
    if (dst->flgs.alloc == 0 || src->flgs.alloc == 0)
        return GMON_RESP_ERRMEM;
    void *dst_data_buffer = dst->data;
    XMEMCPY(dst, src, sizeof(gmonEvent_t));
    dst->data = dst_data_buffer;

    if (src->data != NULL && dst->data != NULL && src->num_active_sensors > 0) {
        size_t nbytes_per_sensor = 0;
        switch (src->event_type) {
        case GMON_EVENT_SOIL_MOISTURE_UPDATED:
        case GMON_EVENT_LIGHTNESS_UPDATED:
            nbytes_per_sensor = sizeof(unsigned int);
            break;
        case GMON_EVENT_AIR_TEMP_UPDATED:
            nbytes_per_sensor = sizeof(gmonAirCond_t);
            break;
        default:
            return GMON_RESP_MALFORMED_DATA;
        }
        XMEMCPY(dst->data, src->data, src->num_active_sensors * nbytes_per_sensor);
    }
    return GMON_RESP_OK;
}

static gMonStatus staAddEventToMsgPipe(
    gardenMonitor_t *gmon, stationSysMsgbox_t msgbuf, gmonEvent_t *msg, uint32_t block_time
) {
    gMonStatus status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
    if (status != GMON_RESP_OK) {
        // if message box is full, abandon the oldest item of the message box, ensure all
        // available items in the message box are up-to-date.
        gmonEvent_t *oldest_record = NULL;
        // Use 0 for block_time to immediately get
        status = staSysMsgBoxGet(msgbuf, (void **)&oldest_record, 0);
        XASSERT((status == GMON_RESP_OK) && (oldest_record != NULL));
        staFreeSensorEvent(&gmon->sensors.event, oldest_record);
        // try adding new record again
        status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
        XASSERT(status == GMON_RESP_OK);
    }
    return status;
}

gMonStatus staNotifyOthersWithEvent(gardenMonitor_t *gmon, gmonEvent_t *evt, uint32_t block_time) {
    gMonEvtPool_t *epool = &gmon->sensors.event;
    gmonEvent_t   *evt_copy = staAllocSensorEvent(epool, evt->event_type, evt->num_active_sensors);
    gMonStatus     status = staCpySensorEvent(evt_copy, evt);
    XASSERT(status == GMON_RESP_OK);
    XASSERT(evt->data != NULL);
    XASSERT(evt_copy->data != NULL);
    staAddEventToMsgPipe(gmon, gmon->msgpipe.sensor2display, evt, block_time);
    staAddEventToMsgPipe(gmon, gmon->msgpipe.sensor2net, evt_copy, block_time);
    return status;
}

gMonStatus stationIOinit(gardenMonitor_t *gmon) {
    gMonStatus status = GMON_RESP_OK;
    if (gmon == NULL) {
        status = GMON_RESP_ERRARGS;
        goto done;
    }
    // Initialize default sensor reading intervals
    gmon->sensors.event.pool = (gmonEvent_t *)XMALLOC(sizeof(gmonEvent_t) * GMON_NUM_SENSOR_EVENTS);
    if (gmon->sensors.event.pool == NULL) {
        status = GMON_RESP_ERRMEM;
        goto done;
    }
    gmon->sensors.event.len = GMON_NUM_SENSOR_EVENTS;
    XMEMSET(gmon->sensors.event.pool, 0x00, sizeof(gmonEvent_t) * GMON_NUM_SENSOR_EVENTS);

    status = GMON_SENSOR_INIT_FN_SOIL_MOIST(&gmon->sensors.soil_moist);
    if (status < 0)
        goto done;
    status = GMON_SENSOR_INIT_FN_LIGHT(&gmon->sensors.light);
    if (status < 0)
        goto done;
    status = GMON_SENSOR_INIT_FN_AIR_TEMP(&gmon->sensors.air_temp);
    if (status < 0)
        goto done;
    status = GMON_ACTUATOR_INIT_FN_PUMP(&gmon->actuator.pump);
    if (status < 0)
        goto done;
    status = GMON_ACTUATOR_INIT_FN_FAN(&gmon->actuator.fan);
    if (status < 0)
        goto done;
    status = GMON_ACTUATOR_INIT_FN_BULB(&gmon->actuator.bulb);
    if (status < 0)
        goto done;
#define NUM_EVTS_PIPE (GMON_NUM_SENSOR_EVENTS + 3)
    gmon->msgpipe.sensor2display = staSysMsgBoxCreate(NUM_EVTS_PIPE);
    XASSERT(gmon->msgpipe.sensor2display != NULL);
    gmon->msgpipe.sensor2net = staSysMsgBoxCreate(NUM_EVTS_PIPE);
    XASSERT(gmon->msgpipe.sensor2net != NULL);
#undef NUM_EVTS_PIPE
done:
    if (status != GMON_RESP_OK && gmon != NULL && gmon->sensors.event.pool != NULL) {
        XMEMFREE(gmon->sensors.event.pool);
        gmon->sensors.event.pool = NULL;
        gmon->sensors.event.len = 0;
    }
    return status;
} // end of stationIOinit

gMonStatus stationIOdeinit(gardenMonitor_t *gmon) {
    gMonStatus   status = GMON_RESP_OK;
    gmonEvent_t *event_to_free = NULL;

    // Free any remaining events in the message queues
    while (staSysMsgBoxGet(gmon->msgpipe.sensor2display, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(&gmon->sensors.event, event_to_free);
        event_to_free = NULL;
    }
    while (staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(&gmon->sensors.event, event_to_free);
        event_to_free = NULL;
    }

    staSysMsgBoxDelete(&gmon->msgpipe.sensor2display);
    XASSERT(gmon->msgpipe.sensor2display == NULL);
    staSysMsgBoxDelete(&gmon->msgpipe.sensor2net);
    XASSERT(gmon->msgpipe.sensor2net == NULL);

    // Deinitialize output devices and sensors
    status = GMON_ACTUATOR_DEINIT_FN_BULB();
    if (status < 0)
        goto done;
    status = GMON_ACTUATOR_DEINIT_FN_FAN();
    if (status < 0)
        goto done;
    status = GMON_ACTUATOR_DEINIT_FN_PUMP();
    if (status < 0)
        goto done;
    status = GMON_SENSOR_DEINIT_FN_SOIL_MOIST(&gmon->sensors.soil_moist);
    if (status < 0)
        goto done;
    status = GMON_SENSOR_DEINIT_FN_LIGHT(&gmon->sensors.light);
    if (status < 0)
        goto done;
    status = GMON_SENSOR_DEINIT_FN_AIR_TEMP(&gmon->sensors.air_temp);
done:
    if (gmon != NULL && gmon->sensors.event.pool != NULL) {
        XMEMFREE(gmon->sensors.event.pool);
        gmon->sensors.event.pool = NULL;
        gmon->sensors.event.len = 0;
    }
    return status;
} // end of stationIOdeinit
