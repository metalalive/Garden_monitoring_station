#include "station_include.h"

gmonSensorSamples_t
staAllocSensorSampleBuffer(gmonSensorSamples_t old, gMonSensorMeta_t *sensor, gmonSensorDataType_t dtype) {
    gmonSensorSamples_t out = {0};
    void               *original_old_entries = old.entries;
    if (sensor == NULL || sensor->num_items == 0 || sensor->num_resamples == 0)
        goto err_return;

    unsigned char num_items = sensor->num_items;
    unsigned char num_resamples = sensor->num_resamples;
    // Size of a single data element (e.g., unsigned int, gmonAirCond_t)
    unsigned short data_element_size = 0;
    // Check for potential overflow before multiplication, if num_items, num_resamples
    // or data_element_size are very large, they might exceed unsigned short or unsigned int.
    // However, the current sizes are small enough for typical embedded contexts.
    // This comment is for self-reflection and not part of the requested change.

    switch (dtype) {
    case GMON_SENSOR_DATA_TYPE_U32:
        data_element_size = sizeof(unsigned int);
        break;
    case GMON_SENSOR_DATA_TYPE_AIRCOND:
        data_element_size = sizeof(gmonAirCond_t);
        break;
    case GMON_SENSOR_DATA_TYPE_UNKNOWN:
    default:
        goto err_return; // Invalid or unsupported data type
    }

    unsigned short outlier_flgs_per_sample_bytes = 0;
    // Calculate bytes for num_resamples bits
    if (num_resamples > 0)
        outlier_flgs_per_sample_bytes = (num_resamples + 7) >> 3;

    unsigned short structs_size = num_items * sizeof(gmonSensorSample_t);
    unsigned short data_pool_size = num_items * num_resamples * data_element_size;
    // This can be 0 if num_resamples is 0
    unsigned short total_outlier_pool_size = num_items * outlier_flgs_per_sample_bytes;
    unsigned short total_size = structs_size + data_pool_size + total_outlier_pool_size;

    if (total_size == 0) // No memory needed if num_items or num_resamples is 0
        goto err_return; // Return {0} as no allocation is necessary, but free old if it existed

    // Use staEnsureStrBufferSize for (re)allocation
    gmonStr_t tmpbuf_mgr = {
        .data = (unsigned char *)old.entries,
        .len = old.total_nbytes,
        .nbytes_written = 0,
    };
    gMonStatus status = staEnsureStrBufferSize(&tmpbuf_mgr, total_size);
    if (status != GMON_RESP_OK)
        goto err_return;
    // Zero-initialize the allocated memory block
    XMEMSET(tmpbuf_mgr.data, 0, tmpbuf_mgr.len);
    void *mem_block = tmpbuf_mgr.data;
    // The array of gmonSensorSample_t structs starts at the beginning of the block
    out.entries = (gmonSensorSample_t *)mem_block;
    out.total_nbytes = total_size;
    // The data pool starts immediately after the structs; use void* for generic pointer arithmetic
    void *curr_data_ptr = (void *)((unsigned char *)mem_block + structs_size);
    // The corrupted flags pool starts immediately after the data pool
    void *curr_outlier_ptr = (void *)((unsigned char *)curr_data_ptr + data_pool_size);

    for (unsigned char i = 0; i < num_items; ++i) {
        out.entries[i].id = i + 1; // ID starts from 1
        out.entries[i].len = num_resamples;
        out.entries[i].dtype = dtype;
        out.entries[i].data = (num_resamples > 0) ? curr_data_ptr : NULL;
        out.entries[i].outlier = (num_resamples > 0) ? (unsigned char *)curr_outlier_ptr : NULL;
        if (num_resamples > 0) {
            // Advance the pointer by the calculated size for the current block of samples
            curr_data_ptr = (void *)((unsigned char *)curr_data_ptr + num_resamples * data_element_size);
            curr_outlier_ptr = (void *)((unsigned char *)curr_outlier_ptr + outlier_flgs_per_sample_bytes);
        }
    }
    // staEnsureStrBufferSize managed the `old.entries` memory.
    // If it reused `old.entries`, then `tmpbuf_mgr.data` is `original_old_entries`.
    // If it reallocated, it freed `original_old_entries` and allocated new memory.
    // So, no explicit freeing of `original_old_entries` is needed here.
    return out;

err_return:
    if (original_old_entries != NULL)
        XMEMFREE(original_old_entries);
    return (gmonSensorSamples_t){0};
} // end of staAllocSensorSampleBuffer

static void staSensorU32DetectNoise(
    gMonSensorMeta_t *s_meta, const gmonSensorSample_t *s_samples, unsigned short tot_len
) {
    const float    threshold_hi = s_meta->outlier_threshold, threshold_lo = threshold_hi * -1.f;
    unsigned short idx = 0, jdx = 0;
    unsigned int   samples_cloned[tot_len];
    unsigned int  *flattened_samples = s_samples[0].data;
    XMEMCPY(samples_cloned, flattened_samples, sizeof(unsigned int) * tot_len);
    // implement modified Z-score at here for outlier detection
    unsigned int median = staFindMedian(samples_cloned, tot_len);
    float        mad = (float)staMedianAbsDeviation(median, samples_cloned, tot_len);
    if (mad < s_meta->mad_threshold)
        mad = s_meta->mad_threshold;
    for (idx = 0; idx < s_meta->num_items; idx++) {
        unsigned int *d = s_samples[idx].data;
        for (jdx = 0; jdx < s_samples[idx].len; jdx++) {
            int   diff = (int)d[jdx] - (int)median;
            float zscore = GMON_STATS_SD2MAD_RATIO * diff / mad;
            char  beyondscope = (zscore < threshold_lo) || (threshold_hi < zscore);
            staSetBitFlag(s_samples[idx].outlier, jdx, beyondscope);
        }
    }
}
static void
staSensorAirCondDetectNoise(gMonSensorMeta_t *s_meta, gmonSensorSample_t *s_samples, unsigned short tot_len) {
    const float    threshold_hi = s_meta->outlier_threshold, threshold_lo = threshold_hi * -1.f;
    unsigned int   samples_cloned[tot_len], median = 0;
    unsigned short idx = 0, jdx = 0, kdx = 0;
    // -------------------
    for (idx = 0, kdx = 0; idx < s_meta->num_items; kdx += s_samples[idx++].len) {
        gmonAirCond_t *d = s_samples[idx].data;
        for (jdx = 0; jdx < s_samples[idx].len; jdx++) {
            samples_cloned[kdx + jdx] = (unsigned int)d[jdx].temporature;
        }
    }
    median = staFindMedian(samples_cloned, tot_len);
    float mad = (float)staMedianAbsDeviation(median, samples_cloned, tot_len);
    if (mad < s_meta->mad_threshold)
        mad = s_meta->mad_threshold;
    for (idx = 0; idx < s_meta->num_items; idx++) {
        gmonAirCond_t *d = s_samples[idx].data;
        for (jdx = 0; jdx < s_samples[idx].len; jdx++) {
            float diff = d[jdx].temporature - (float)median;
            float zscore = GMON_STATS_SD2MAD_RATIO * diff / mad;
            char  beyondscope = (zscore < threshold_lo) || (threshold_hi < zscore);
            staSetBitFlag(s_samples[idx].outlier, jdx, beyondscope);
        }
    }
    // -------------------
    for (idx = 0, kdx = 0; idx < s_meta->num_items; kdx += s_samples[idx++].len) {
        gmonAirCond_t *d = s_samples[idx].data;
        for (jdx = 0; jdx < s_samples[idx].len; jdx++) {
            samples_cloned[kdx + jdx] = (unsigned int)d[jdx].humidity;
        }
    }
    median = staFindMedian(samples_cloned, tot_len);
    mad = (float)staMedianAbsDeviation(median, samples_cloned, tot_len);
    if (mad < s_meta->mad_threshold)
        mad = s_meta->mad_threshold;
    for (idx = 0; idx < s_meta->num_items; idx++) {
        gmonAirCond_t *d = s_samples[idx].data;
        for (jdx = 0; jdx < s_samples[idx].len; jdx++) {
            float diff = d[jdx].humidity - (float)median;
            float zscore = GMON_STATS_SD2MAD_RATIO * diff / mad;
            char  beyondscope = (zscore < threshold_lo) || (threshold_hi < zscore);
            if (staGetBitFlag(s_samples[idx].outlier, jdx) == 0x0)
                staSetBitFlag(s_samples[idx].outlier, jdx, beyondscope);
        }
    }
} // end of staSensorAirCondDetectNoise

static unsigned short
staAggregateU32Samples(gmonEvent_t *evt, gmonSensorSample_t *ssample, unsigned char s_idx) {
    unsigned int  *raw_data = (unsigned int *)ssample->data;
    unsigned int   sum = 0;
    unsigned short valid_sample_count = 0, outlier_count = 0;

    for (unsigned short j = 0; j < ssample->len; ++j) {
        if (staGetBitFlag(ssample->outlier, j) == 0) { // Not an outlier
            sum += raw_data[j];
            valid_sample_count++;
        } else {
            outlier_count++;
        }
    }
    if (valid_sample_count > 0)
        ((unsigned int *)evt->data)[s_idx] = (unsigned int)(sum / valid_sample_count);
    return outlier_count;
}

static unsigned short
staAggregateAirCondSamples(gmonEvent_t *evt, gmonSensorSample_t *ssample, unsigned char s_idx) {
    gmonAirCond_t *raw_data = (gmonAirCond_t *)ssample->data;
    float          temp_sum = 0.0f, humid_sum = 0.0f;
    unsigned short valid_sample_count = 0, outlier_count = 0;

    for (unsigned short j = 0; j < ssample->len; ++j) {
        if (staGetBitFlag(ssample->outlier, j) == 0) { // Not an outlier
            temp_sum += raw_data[j].temporature;
            humid_sum += raw_data[j].humidity;
            valid_sample_count++;
        } else {
            outlier_count++;
        }
    }
    gmonAirCond_t *event_air_cond = &((gmonAirCond_t *)evt->data)[s_idx];
    if (valid_sample_count > 0) {
        event_air_cond->temporature = temp_sum / valid_sample_count;
        event_air_cond->humidity = humid_sum / valid_sample_count;
    }
    return outlier_count;
}

gMonStatus staSensorDetectNoise(gMonSensorMeta_t *s_meta, gmonSensorSample_t *s_samples) {
    if (s_samples == NULL || s_meta == NULL || s_meta->mad_threshold < 0.01f ||
        s_meta->outlier_threshold < 0.01f || s_meta->num_items == 0)
        return GMON_RESP_ERRARGS;
    gMonStatus     status = GMON_RESP_OK;
    unsigned short tot_len = 0, idx = 0;
    for (idx = 0; idx < s_meta->num_items; idx++) {
        if (s_samples[idx].data == NULL || s_samples[idx].outlier == NULL)
            return GMON_RESP_ERRMEM;
        tot_len += s_samples[idx].len;
    }
    switch (s_samples[0].dtype) {
    case GMON_SENSOR_DATA_TYPE_U32:
        staSensorU32DetectNoise(s_meta, s_samples, tot_len);
        break;
    case GMON_SENSOR_DATA_TYPE_AIRCOND:
        staSensorAirCondDetectNoise(s_meta, s_samples, tot_len);
        break;
    default:
        status = GMON_RESP_MALFORMED_DATA;
        break;
    }
    return status;
}

gMonStatus staSensorSampleToEvent(gmonEvent_t *evt, gmonSensorSample_t *samples) {
    if (evt == NULL || samples == NULL || evt->num_active_sensors == 0)
        return GMON_RESP_ERRARGS;

    gmonSensorDataType_t dtype0 = samples[0].dtype;
    evt->flgs.corruption = 0;

    for (unsigned char i = 0; i < evt->num_active_sensors; ++i) {
        gmonSensorSample_t  *current_sample = &samples[i];
        gmonSensorDataType_t dtype = current_sample->dtype;
        unsigned short       outlier_count = 0;

        if (dtype0 != dtype || current_sample->len == 0 || current_sample->data == NULL ||
            current_sample->outlier == NULL) {
            // If data types mismatch, no samples, no data, or no outlier info, consider this sensor corrupted
            staSetBitFlag(&evt->flgs.corruption, current_sample->id - 1, 1);
            continue; // Skip to processing the next sensor
        }
        switch (dtype) {
        case GMON_SENSOR_DATA_TYPE_U32:
            outlier_count = staAggregateU32Samples(evt, current_sample, i);
            break;
        case GMON_SENSOR_DATA_TYPE_AIRCOND:
            outlier_count = staAggregateAirCondSamples(evt, current_sample, i);
            break;
        default:
            // Unsupported data type for aggregation, mark sensor as corrupted
            outlier_count = current_sample->len;
            break;
        }
        // If more than half of samples were outliers, set corruption flag for the sensor
        if (outlier_count >= ((current_sample->len + 1) >> 1)) {
            staSetBitFlag(&evt->flgs.corruption, current_sample->id - 1, 1);
        }
    }
    return GMON_RESP_OK;
}
