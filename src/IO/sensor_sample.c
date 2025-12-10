#include "station_include.h"

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
}
