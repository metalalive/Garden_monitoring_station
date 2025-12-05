#include "station_include.h"

gmonEvent_t *staAllocSensorEvent(gardenMonitor_t *gmon) {
    gmonEvent_t *out = NULL;
    uint16_t     idx = 0;
    stationSysEnterCritical();
    for (idx = 0; idx < gmon->sensors.event.len; idx++) {
        if (gmon->sensors.event.pool[idx].flgs.alloc == 0) {
            out = &gmon->sensors.event.pool[idx];
            XMEMSET(out, 0x00, sizeof(gmonEvent_t));
            out->flgs.alloc = 1;
            break;
        }
    }
    stationSysExitCritical();
    return out;
}

gMonStatus staFreeSensorEvent(gardenMonitor_t *gmon, gmonEvent_t *record) {
    gmonEvent_t *addr_start = NULL, *addr_end = NULL;
    gMonStatus   status = GMON_RESP_OK;
    uint16_t     idx = 0;
    stationSysEnterCritical();
    addr_start = &gmon->sensors.event.pool[0];
    addr_end = &gmon->sensors.event.pool[gmon->sensors.event.len];
    if ((record < addr_start) || (addr_end <= record)) {
        status = GMON_RESP_ERRMEM;
        goto done;
    }
    for (idx = 0; idx < gmon->sensors.event.len; idx++) {
        addr_start = &gmon->sensors.event.pool[idx];
        addr_end = &gmon->sensors.event.pool[idx + 1];
        if ((record > addr_start) && (record < addr_end)) {
            status = GMON_RESP_ERRMEM; // memory not aligned
            break;
        } else if (record == addr_start) {
            if (addr_start->flgs.alloc != 0) {
                addr_start->flgs.alloc = 0;
            }
            break;
        }
    }
done:
    stationSysExitCritical();
    return status;
}

gMonStatus staCpySensorEvent(gmonEvent_t *dst, gmonEvent_t *src, size_t sz) {
    unsigned int idx = 0;
    gMonStatus   status = GMON_RESP_OK;
    if (dst == NULL || src == NULL || sz == 0) {
        return GMON_RESP_ERRARGS;
    }
    for (idx = 0; idx < sz; idx++) {
        if (dst[idx].flgs.alloc == 0 || src[idx].flgs.alloc == 0) {
            status = GMON_RESP_ERRMEM;
            break;
        }
    }
    if (status == GMON_RESP_OK) {
        XMEMCPY(dst, src, sizeof(gmonEvent_t) * sz);
    }
    return status;
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
        staFreeSensorEvent(gmon, oldest_record);
        // try adding new record again
        status = staSysMsgBoxPut(msgbuf, (void *)msg, block_time);
        XASSERT(status == GMON_RESP_OK);
    }
    return status;
}

gMonStatus staNotifyOthersWithEvent(gardenMonitor_t *gmon, gmonEvent_t *event, uint32_t block_time) {
    gmonEvent_t *event_copy = staAllocSensorEvent(gmon);
    gMonStatus   status = staCpySensorEvent(event_copy, event, (size_t)0x1);
    staAddEventToMsgPipe(gmon, gmon->msgpipe.sensor2display, event, block_time);
    staAddEventToMsgPipe(gmon, gmon->msgpipe.sensor2net, event_copy, block_time);
    return status;
}

gmonSensorSample_t *staAllocSensorSampleBuffer(gMonSensor_t *sensor, gmonSensorDataType_t dtype) {
    gmonSensorSample_t *out = NULL;
    if (sensor == NULL || sensor->num_items == 0)
        return NULL;

    unsigned char num_items = sensor->num_items;
    unsigned char num_resamples = sensor->num_resamples;

    size_t data_element_size = 0; // Size of a single data element (e.g., unsigned int, gmonAirCond_t)
    size_t outlier_flgs_per_sample_bytes = 0; // Bytes needed for corruption flags for one sample item

    switch (dtype) {
    case GMON_SENSOR_DATA_TYPE_U32:
        data_element_size = sizeof(unsigned int);
        break;
    case GMON_SENSOR_DATA_TYPE_AIRCOND:
        data_element_size = sizeof(gmonAirCond_t);
        break;
    case GMON_SENSOR_DATA_TYPE_UNKNOWN:
    default:
        return NULL; // Invalid or unsupported data type
    }
    // Calculate bytes for num_resamples bits
    if (num_resamples > 0)
        outlier_flgs_per_sample_bytes = (num_resamples + 7) >> 3;

    size_t structs_size = num_items * sizeof(gmonSensorSample_t);
    size_t data_pool_size = num_items * num_resamples * data_element_size;
    size_t total_outlier_pool_size = num_items * outlier_flgs_per_sample_bytes;
    size_t total_size = structs_size + data_pool_size + total_outlier_pool_size;

    void *mem_block = XCALLOC(1, total_size);
    if (mem_block == NULL)
        return NULL; // Memory allocation failed

    // The array of gmonSensorSample_t structs starts at the beginning of the block
    out = (gmonSensorSample_t *)mem_block;
    // The data pool starts immediately after the structs; use void* for generic pointer arithmetic
    void *curr_data_ptr = (void *)((unsigned char *)mem_block + structs_size);
    // The corrupted flags pool starts immediately after the data pool
    void *curr_outlier_ptr = (void *)((unsigned char *)curr_data_ptr + data_pool_size);

    for (unsigned char i = 0; i < num_items; ++i) {
        out[i].id = i + 1; // ID starts from 1
        out[i].len = num_resamples;
        out[i].dtype = dtype;
        out[i].data = (num_resamples > 0) ? curr_data_ptr : NULL;
        out[i].outlier = (num_resamples > 0) ? (unsigned char *)curr_outlier_ptr : NULL;
        if (num_resamples > 0) {
            // Advance the pointer by the calculated size for the current block of samples
            curr_data_ptr = (void *)((unsigned char *)curr_data_ptr + num_resamples * data_element_size);
            curr_outlier_ptr = (void *)((unsigned char *)curr_outlier_ptr + outlier_flgs_per_sample_bytes);
        }
    }
    return out;
} // end of staAllocSensorSampleBuffer

static void staSensorU32DetectNoise(
    float threshold, const gmonSensorSample_t *sensorsamples, unsigned char num_items, unsigned short tot_len
) {
    const float    threshold_hi = threshold, threshold_lo = threshold_hi * -1.f;
    unsigned short idx = 0, jdx = 0;
    unsigned int   samples_cloned[tot_len];
    unsigned int  *flattened_samples = sensorsamples[0].data;
    XMEMCPY(samples_cloned, flattened_samples, sizeof(unsigned int) * tot_len);
    // implement modified Z-score at here for outlier detection
    unsigned int median = staFindMedian(samples_cloned, tot_len);
    unsigned int mad = staMedianAbsDeviation(median, samples_cloned, tot_len);
    for (idx = 0; idx < num_items; idx++) {
        unsigned int *d = sensorsamples[idx].data;
        for (jdx = 0; jdx < sensorsamples[idx].len; jdx++) {
            int   diff = (int)d[jdx] - (int)median;
            float zscore = GMON_STATS_SD2MAD_RATIO * diff / mad;
            char  beyondscope = (zscore < threshold_lo) || (threshold_hi < zscore);
            staSetBitFlag(sensorsamples[idx].outlier, jdx, beyondscope);
        }
    }
}
static void staSensorAirCondDetectNoise(
    float threshold, const gmonSensorSample_t *sensorsamples, unsigned char num_items, unsigned short tot_len
) {
    const float    threshold_hi = threshold, threshold_lo = threshold_hi * -1.f;
    unsigned int   samples_cloned[tot_len], median = 0, mad = 0;
    unsigned short idx = 0, jdx = 0, kdx = 0;
    // -------------------
    for (idx = 0, kdx = 0; idx < num_items; kdx += sensorsamples[idx++].len) {
        gmonAirCond_t *d = sensorsamples[idx].data;
        for (jdx = 0; jdx < sensorsamples[idx].len; jdx++) {
            samples_cloned[kdx + jdx] = (unsigned int)d[jdx].temporature;
        }
    }
    median = staFindMedian(samples_cloned, tot_len);
    mad = staMedianAbsDeviation(median, samples_cloned, tot_len);
    for (idx = 0; idx < num_items; idx++) {
        gmonAirCond_t *d = sensorsamples[idx].data;
        for (jdx = 0; jdx < sensorsamples[idx].len; jdx++) {
            float diff = d[jdx].temporature - (float)median;
            float zscore = GMON_STATS_SD2MAD_RATIO * diff / mad;
            char  beyondscope = (zscore < threshold_lo) || (threshold_hi < zscore);
            staSetBitFlag(sensorsamples[idx].outlier, jdx, beyondscope);
        }
    }
    // -------------------
    for (idx = 0, kdx = 0; idx < num_items; kdx += sensorsamples[idx++].len) {
        gmonAirCond_t *d = sensorsamples[idx].data;
        for (jdx = 0; jdx < sensorsamples[idx].len; jdx++) {
            samples_cloned[kdx + jdx] = (unsigned int)d[jdx].humidity;
        }
    }
    median = staFindMedian(samples_cloned, tot_len);
    mad = staMedianAbsDeviation(median, samples_cloned, tot_len);
    for (idx = 0; idx < num_items; idx++) {
        gmonAirCond_t *d = sensorsamples[idx].data;
        for (jdx = 0; jdx < sensorsamples[idx].len; jdx++) {
            float diff = d[jdx].humidity - (float)median;
            float zscore = GMON_STATS_SD2MAD_RATIO * diff / mad;
            char  beyondscope = (zscore < threshold_lo) || (threshold_hi < zscore);
            if (staGetBitFlag(sensorsamples[idx].outlier, jdx) == 0x0)
                staSetBitFlag(sensorsamples[idx].outlier, jdx, beyondscope);
        }
    }
} // end of staSensorAirCondDetectNoise

gMonStatus
staSensorDetectNoise(float threshold, const gmonSensorSample_t *sensorsamples, unsigned char num_items) {
    if (sensorsamples == NULL || threshold < 0.01 || num_items == 0)
        return GMON_RESP_ERRARGS;
    gMonStatus     status = GMON_RESP_OK;
    unsigned short tot_len = 0, idx = 0;
    for (idx = 0; idx < num_items; idx++) {
        if (sensorsamples[idx].data == NULL || sensorsamples[idx].outlier == NULL)
            return GMON_RESP_ERRMEM;
        tot_len += sensorsamples[idx].len;
    }
    switch (sensorsamples[0].dtype) {
    case GMON_SENSOR_DATA_TYPE_U32:
        staSensorU32DetectNoise(threshold, sensorsamples, num_items, tot_len);
        break;
    case GMON_SENSOR_DATA_TYPE_AIRCOND:
        staSensorAirCondDetectNoise(threshold, sensorsamples, num_items, tot_len);
        break;
    default:
        status = GMON_RESP_MALFORMED_DATA;
        break;
    }
    return status;
} // end of staSensorDetectNoise

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
    gmon->msgpipe.sensor2display = staSysMsgBoxCreate(GMON_CFG_NUM_SENSOR_RECORDS_KEEP);
    XASSERT(gmon->msgpipe.sensor2display != NULL);
    gmon->msgpipe.sensor2net = staSysMsgBoxCreate(GMON_CFG_NUM_SENSOR_RECORDS_KEEP);
    XASSERT(gmon->msgpipe.sensor2net != NULL);
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
        staFreeSensorEvent(gmon, event_to_free);
        event_to_free = NULL;
    }
    while (staSysMsgBoxGet(gmon->msgpipe.sensor2net, (void **)&event_to_free, 0) == GMON_RESP_OK) {
        staFreeSensorEvent(gmon, event_to_free);
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
