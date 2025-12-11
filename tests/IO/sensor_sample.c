#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

static gardenMonitor_t     gmon; // Global instance for setup/teardown
static gmonSensorSample_t *mock_samples = NULL;
static gmonEvent_t        *mock_event = NULL;

TEST_GROUP(SensorSampleAlloc);
TEST_GROUP(SensorNoiseDetection);

TEST_SETUP(SensorSampleAlloc) {}

TEST_TEAR_DOWN(SensorSampleAlloc) {}

TEST_SETUP(SensorNoiseDetection) {
    XMEMFREE(mock_event); // Ensure clean slate if previous test failed to clean up
    mock_event = NULL;
    gMonStatus status = stationIOinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_TEAR_DOWN(SensorNoiseDetection) {
    gMonStatus status = stationIOdeinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    XMEMFREE(mock_samples);
    mock_samples = NULL;
}

TEST_GROUP(SensorSampleToEvent);

TEST_SETUP(SensorSampleToEvent) {
    gMonStatus status = stationIOinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_TEAR_DOWN(SensorSampleToEvent) {
    gMonStatus status = stationIOdeinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    XMEMFREE(mock_samples);
    mock_samples = NULL;
    if (mock_event) {
        if (mock_event->flgs.alloc && mock_event->data) {
            XMEMFREE(mock_event->data);
        }
        XMEMFREE(mock_event);
        mock_event = NULL;
    }
}

// Helper function to create and initialize a gmonEvent_t
static gmonEvent_t *
createAndInitEvent(gmonEventType_t event_type, unsigned char num_active_sensors, size_t data_element_size) {
    gmonEvent_t *evt = XCALLOC(1, sizeof(gmonEvent_t));
    TEST_ASSERT_NOT_NULL(evt);

    evt->event_type = event_type;
    evt->num_active_sensors = num_active_sensors;
    evt->flgs.alloc = 1;      // Indicate memory is allocated for event data
    evt->flgs.corruption = 0; // Initialize

    if (num_active_sensors > 0 && data_element_size > 0) {
        evt->data = XCALLOC(num_active_sensors, data_element_size);
        TEST_ASSERT_NOT_NULL(evt->data);
    } else {
        evt->data = NULL;
    }
    return evt;
}

TEST(SensorSampleAlloc, initOk) {
    gMonSensor_t sensor = {
        .lowlvl = (void *)0xDEADBEEF, // Dummy pointer, not used by staAllocSensorSampleBuffer
        .read_interval_ms = 1000,
        .num_items = 2,     // Allocate for two sensor items
        .num_resamples = 3, // Each item has three resamples
    };
    gmonSensorDataType_t dtype = GMON_SENSOR_DATA_TYPE_U32;
    gmonSensorSample_t  *samples = staAllocSensorSampleBuffer(&sensor, dtype);

    // 1. Assert that memory was successfully allocated
    TEST_ASSERT_NOT_NULL(samples);

    // 2. Verify properties of the first sensor sample struct
    TEST_ASSERT_EQUAL(1, samples[0].id);
    TEST_ASSERT_EQUAL(sensor.num_resamples, samples[0].len);
    TEST_ASSERT_EQUAL(dtype, samples[0].dtype);
    TEST_ASSERT_NOT_NULL(samples[0].data);
    // 3. Verify properties of the second sensor sample struct
    TEST_ASSERT_EQUAL(2, samples[1].id);
    TEST_ASSERT_EQUAL(sensor.num_resamples, samples[1].len);
    TEST_ASSERT_EQUAL(dtype, samples[1].dtype);
    TEST_ASSERT_NOT_NULL(samples[1].data);
    // 4. Verify that the data pointer for the second sample is correctly offset from the first
    // The offset should be 'num_resamples' times the size of a single data element (unsigned int)
    size_t expected_offset = sensor.num_resamples * sizeof(unsigned int);
    TEST_ASSERT_EQUAL_PTR((void *)((unsigned char *)samples[0].data + expected_offset), samples[1].data);

    // 5. Free the allocated memory to prevent leaks
    XMEMFREE(samples);
}

TEST(SensorNoiseDetection, ErrMissingArgs) {
    gMonSensor_t *s = &gmon.sensors.soil_moist;
    s->num_items = 4;
    s->num_resamples = 3;
    s->outlier_threshold = 2.2f;
    mock_samples = staAllocSensorSampleBuffer(s, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    gMonStatus status = staSensorDetectNoise(4.5f, NULL, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staSensorDetectNoise(0.0f, mock_samples, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staSensorDetectNoise(4.5f, mock_samples, 0);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    void *data_ptr_bak = mock_samples[2].data;
    mock_samples[2].data = NULL;
    status = staSensorDetectNoise(3.5f, mock_samples, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    mock_samples[2].data = data_ptr_bak;
    mock_samples[0].dtype = GMON_SENSOR_DATA_TYPE_UNKNOWN;
    status = staSensorDetectNoise(3.5f, mock_samples, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(SensorNoiseDetection, U32FewOutliers) {
    gMonSensor_t *s = &gmon.sensors.soil_moist;
    s->num_items = 4;
    s->num_resamples = 3;
    s->outlier_threshold = 2.2f;
    mock_samples = staAllocSensorSampleBuffer(s, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    unsigned int   expectlist[] = {10, 11, 12, 98, 1, 9, 11, 11, 57, 12, 9, 11};
    unsigned short data_len = s->num_items * s->num_resamples;
    unsigned short rs_len = s->num_items;
    XMEMCPY(mock_samples[0].data, expectlist, sizeof(unsigned int) * data_len);
    unsigned char expected_result[] = {0x0, 0x3, 0x4, 0x0};
    gMonStatus    status = staSensorDetectNoise(s->outlier_threshold, mock_samples, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_result, mock_samples[0].outlier, rs_len);
    // the result flags indicate following samples are outliers :
    TEST_ASSERT_EQUAL(98, ((unsigned int *)mock_samples[1].data)[0]);
    TEST_ASSERT_EQUAL(57, ((unsigned int *)mock_samples[2].data)[2]);
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[1].outlier, 0));
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[1].outlier, 1));
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[2].outlier, 2));
}

TEST(SensorNoiseDetection, AirCondFewOutliers) {
    gMonSensor_t *s = &gmon.sensors.air_temp;
    s->num_items = 4;
    s->num_resamples = 5;
    s->outlier_threshold = 2.7f;
    mock_samples = staAllocSensorSampleBuffer(s, GMON_SENSOR_DATA_TYPE_AIRCOND);
    TEST_ASSERT_NOT_NULL(mock_samples);
    // clang-format off
    // Prepare test data (4 sensors, 5 resamples each)
    gmonAirCond_t  test_air_cond_data[] = {
        // Sensor 0 (idx=0)
        {20.0f, 50.0f}, {21.0f, 51.0f}, {22.0f, 52.0f}, {23.0f, 53.0f}, {24.0f, 54.0f},
        // Sensor 1 (idx=1)
        {21.0f, 51.0f}, {22.0f, 100.0f}, {101.0f, 53.0f}, {23.0f, 54.0f}, {24.0f, 55.0f},
        // temp outlier (100.0f) at index 2, hum outlier (100.0f) at index 1
        // Sensor 2 (idx=2)
        {22.0f, 52.0f}, {23.0f, 53.0f}, {24.0f, 10.0f}, {25.0f, 55.0f}, {26.0f, 56.0f},
        // hum outlier (10.0f) at index 2
        // Sensor 3 (idx=3)
        {23.0f, 53.0f}, {24.0f, 54.0f}, {25.0f, 55.0f}, {26.0f, 56.0f}, {27.0f, 57.0f}
    };
    // clang-format on
    unsigned short data_len = s->num_items * s->num_resamples;
    unsigned short rs_len = s->num_items; // For checking each sensor's first outlier byte

    // Copy the prepared data into the allocated buffer
    XMEMCPY(mock_samples[0].data, test_air_cond_data, sizeof(gmonAirCond_t) * data_len);

    // Expected outlier flags for each sensor's first outlier byte
    // Sensor 0: No outliers (0x0)
    // Sensor 1: temp at data[2] (bit 2 = 0x4), humidity at data[1] (bit 1 = 0x2). Combined: 0x4 | 0x2 = 0x6
    // Sensor 2: humidity at data[2] (bit 2 = 0x4). Combined: 0x4
    // Sensor 3: No outliers (0x0)
    unsigned char expected_result_aircond[] = {0x0, 0x6, 0x4, 0x0};

    // Perform noise detection
    gMonStatus status = staSensorDetectNoise(s->outlier_threshold, mock_samples, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

    for (int i = 0; i < rs_len; ++i)
        TEST_ASSERT_EQUAL_HEX8(expected_result_aircond[i], mock_samples[i].outlier[0]);

    // Verify specific outlier values and their flags
    // Hum outlier at index 1
    TEST_ASSERT_EQUAL_FLOAT(100.0f, ((gmonAirCond_t *)mock_samples[1].data)[1].humidity);
    // Check bit 1 (0x2)
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[1].outlier, 1));
    // Temp outlier at index 2
    TEST_ASSERT_EQUAL_FLOAT(101.0f, ((gmonAirCond_t *)mock_samples[1].data)[2].temporature);
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[1].outlier, 2));
    // Check bit 2 (0x4)
    // Hum outlier at index 2
    TEST_ASSERT_EQUAL_FLOAT(10.0f, ((gmonAirCond_t *)mock_samples[2].data)[2].humidity);
    // Check bit 2 (0x4)
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[2].outlier, 2));
} // end of AirCondFewOutliers

TEST(SensorNoiseDetection, U32HalfOutliers) {
    gMonSensor_t *s = &gmon.sensors.soil_moist;
    s->num_items = 3;
    s->num_resamples = 7;
    s->outlier_threshold = 2.9f;
    mock_samples = staAllocSensorSampleBuffer(s, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    unsigned int expectlist[] = {
        11, 12, 11,  12,  11,  12,  11, // First sensor's 7 samples
        13, 13, 13,  11,  1,   2,   3,  // Second sensor's 7 samples
        4,  5,  100, 101, 102, 103, 104 // Third sensor's 7 samples
    };
    unsigned short data_len = s->num_items * s->num_resamples; // 3 * 7 = 21
    unsigned short rs_len = s->num_items;                      // Number of sensors is 3
    XMEMCPY(mock_samples[0].data, expectlist, sizeof(unsigned int) * data_len);

    // Expected outlier flags (bitmask for each sensor's samples)
    // Assuming GMON_STATS_SD2MAD_RATIO = 1.4826f, median=10, MAD=1, threshold=2.9f
    // |value - 10| > (2.9 / 1.4826) * 1  => |value - 10| > 1.956
    // Outliers are values < 8.044 or > 11.956
    //
    // mock_samples[0] data: {10, 10, 10, 10, 10, 10, 11}
    // All diffs are 0 or 1, so none are outliers (0x00)
    //
    // mock_samples[1] data: {11, 11, 11, 11, 1, 2, 3}
    // 11 (diff 1) - not outlier.
    // 1 (diff 9) - outlier (bit 4 set).
    // 2 (diff 8) - outlier (bit 5 set).
    // 3 (diff 7) - outlier (bit 6 set).
    // Expected: (1<<4)|(1<<5)|(1<<6) = 0x10|0x20|0x40 = 0x70
    //
    // mock_samples[2] data: {4, 5, 100, 101, 102, 103, 104}
    // All values (diffs 6,5,90,91,92,93,94) are outliers.
    // Expected: (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6) = 0x7F
    unsigned char expected_result[] = {0x00, 0x70, 0x7F};
    gMonStatus    status = staSensorDetectNoise(s->outlier_threshold, mock_samples, s->num_items);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_result, mock_samples[0].outlier, rs_len);

    // Verify specific points
    TEST_ASSERT_FALSE(staGetBitFlag(mock_samples[0].outlier, 0)); // 10 is not outlier
    TEST_ASSERT_FALSE(staGetBitFlag(mock_samples[0].outlier, 6)); // 11 is not outlier
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[1].outlier, 4));  // 1 is outlier
    TEST_ASSERT_FALSE(staGetBitFlag(mock_samples[1].outlier, 0)); // 11 is not outlier
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[2].outlier, 0));  // 4 is outlier
    TEST_ASSERT_TRUE(staGetBitFlag(mock_samples[2].outlier, 6));  // 104 is outlier
} // end of U32HalfOutliers

TEST(SensorSampleToEvent, ErrArgsNullZero) {
    gMonSensor_t sensor_cfg = {.num_items = 1, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    // Zero active sensors
    mock_event = createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, 0, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_samples);
    TEST_ASSERT_NOT_NULL(mock_event);
    gMonStatus status = staSensorSampleToEvent(NULL, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staSensorSampleToEvent(mock_event, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
}

TEST(SensorSampleToEvent, U32HappyPathNoOutliers) {
    gMonSensor_t sensor_cfg = {.num_items = 2, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    ((unsigned int *)mock_samples[0].data)[0] = 10;
    ((unsigned int *)mock_samples[0].data)[1] = 11;
    ((unsigned int *)mock_samples[0].data)[2] = 12; // Avg = 11
    ((unsigned int *)mock_samples[1].data)[0] = 20;
    ((unsigned int *)mock_samples[1].data)[1] = 21;
    ((unsigned int *)mock_samples[1].data)[2] = 22; // Avg = 21

    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, mock_event->flgs.corruption);
    TEST_ASSERT_EQUAL(11, ((unsigned int *)mock_event->data)[0]);
    TEST_ASSERT_EQUAL(21, ((unsigned int *)mock_event->data)[1]);
}

TEST(SensorSampleToEvent, AirCondHappyPathNoOutliers) {
    gMonSensor_t sensor_cfg = {.num_items = 1, .num_resamples = 2};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_AIRCOND);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event = createAndInitEvent(GMON_EVENT_AIR_TEMP_UPDATED, sensor_cfg.num_items, sizeof(gmonAirCond_t));
    TEST_ASSERT_NOT_NULL(mock_event);

    gmonAirCond_t *s0_data = (gmonAirCond_t *)mock_samples[0].data;
    s0_data[0].temporature = 20.0f;
    s0_data[0].humidity = 50.0f;
    s0_data[1].temporature = 22.0f;
    s0_data[1].humidity = 52.0f; // Avg T=21.0, Avg H=51.0
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, mock_event->flgs.corruption);
    TEST_ASSERT_EQUAL_FLOAT(21.0f, ((gmonAirCond_t *)mock_event->data)[0].temporature);
    TEST_ASSERT_EQUAL_FLOAT(51.0f, ((gmonAirCond_t *)mock_event->data)[0].humidity);
}

TEST(SensorSampleToEvent, U32SomeOutliersBelowThreshold) {
    gMonSensor_t sensor_cfg = {.num_items = 1, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    ((unsigned int *)mock_samples[0].data)[0] = 10;
    ((unsigned int *)mock_samples[0].data)[1] = 100; // Outlier
    ((unsigned int *)mock_samples[0].data)[2] = 12;
    staSetBitFlag(mock_samples[0].outlier, 1, 1); // Mark 100 as outlier

    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, mock_event->flgs.corruption); // Only 1 outlier, (3+1)/2 = 2. So not corrupted.
    TEST_ASSERT_EQUAL(11, ((unsigned int *)mock_event->data)[0]); // (10+12)/2 = 11
}

TEST(SensorSampleToEvent, AirCondSomeOutliersBelowThreshold) {
    gMonSensor_t sensor_cfg = {.num_items = 1, .num_resamples = 6};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_AIRCOND);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event = createAndInitEvent(GMON_EVENT_AIR_TEMP_UPDATED, sensor_cfg.num_items, sizeof(gmonAirCond_t));
    TEST_ASSERT_NOT_NULL(mock_event);

    gmonAirCond_t *s0_data = (gmonAirCond_t *)mock_samples[0].data;
    s0_data[0].temporature = 20.0f;
    s0_data[0].humidity = 50.0f;
    s0_data[1].temporature = 100.0f;
    s0_data[1].humidity = 51.0f; // Temp Outlier
    s0_data[2].temporature = 22.0f;
    s0_data[2].humidity = 10.0f; // Humidity Outlier
    s0_data[3].temporature = 24.0f;
    s0_data[3].humidity = 54.0f;
    s0_data[4].temporature = 22.0f;
    s0_data[4].humidity = 51.0f;
    s0_data[5].temporature = 1.23f; // both are outliers
    s0_data[5].humidity = 4.56f;

    staSetBitFlag(mock_samples[0].outlier, 1, 1); // Mark index 1 as outlier (temp)
    staSetBitFlag(mock_samples[0].outlier, 2, 1); // Mark index 2 as outlier (humidity)
    staSetBitFlag(mock_samples[0].outlier, 5, 1); // Mark index 5 as outlier

    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // (20+24+22)/3 = 22
    TEST_ASSERT_EQUAL_FLOAT(22.0f, ((gmonAirCond_t *)mock_event->data)[0].temporature);
    // (50+54+51)/3 = 51.666667
    TEST_ASSERT_FLOAT_WITHIN(0.01, 51.6666f, ((gmonAirCond_t *)mock_event->data)[0].humidity);
    // 3 outliers (from 6 resamples), (6+1)/2 = 3. So if >=3 outliers, it's corrupted.
    // Both 1st, 2nd, 5th samples are outliers due to the way `staSensorAirCondDetectNoise` combines.
    // This should result in corruption
    char actual_corruption = staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1);
    TEST_ASSERT_TRUE(actual_corruption);
}

TEST(SensorSampleToEvent, U32ManyOutliersSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 1, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);

    ((unsigned int *)mock_samples[0].data)[0] = 10;
    ((unsigned int *)mock_samples[0].data)[1] = 100;
    ((unsigned int *)mock_samples[0].data)[2] = 200;
    staSetBitFlag(mock_samples[0].outlier, 0, 1); // Mark as outlier
    staSetBitFlag(mock_samples[0].outlier, 1, 1); // Mark as outlier
    // Two outliers from 3 samples (2 >= (3+1)/2 = 2). Should be corrupted.

    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1));
    TEST_ASSERT_EQUAL(200, ((unsigned int *)mock_event->data)[0]); // The only non-outlier value
}

TEST(SensorSampleToEvent, AirCondManyOutliersSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 1, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_AIRCOND);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event = createAndInitEvent(GMON_EVENT_AIR_TEMP_UPDATED, sensor_cfg.num_items, sizeof(gmonAirCond_t));
    TEST_ASSERT_NOT_NULL(mock_event);

    gmonAirCond_t *s0_data = (gmonAirCond_t *)mock_samples[0].data;
    s0_data[0].temporature = 20.0f;
    s0_data[0].humidity = 50.0f;
    s0_data[1].temporature = 100.0f;
    s0_data[1].humidity = 101.0f;
    s0_data[2].temporature = 200.0f;
    s0_data[2].humidity = 201.0f;
    staSetBitFlag(mock_samples[0].outlier, 0, 1); // Mark all 3 as outliers
    staSetBitFlag(mock_samples[0].outlier, 1, 1);
    staSetBitFlag(mock_samples[0].outlier, 2, 1);

    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1));
    TEST_ASSERT_EQUAL_FLOAT(
        0.0f, ((gmonAirCond_t *)mock_event->data)[0].temporature
    ); // No valid samples, data should remain 0
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ((gmonAirCond_t *)mock_event->data)[0].humidity);
}

TEST(SensorSampleToEvent, MismatchedDataTypeSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 2, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    // Sensor 1 is U32 (expected). Sensor 2 is forced to be AIRCOND.
    mock_samples[1].dtype = GMON_SENSOR_DATA_TYPE_AIRCOND;
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_FALSE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1)); // Sensor 1 OK
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[1].id - 1)
    ); // Sensor 2 corrupted
}

TEST(SensorSampleToEvent, ZeroLenSampleSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 2, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    mock_samples[1].len = 0; // Force second sensor to have 0 length
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_FALSE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1)); // Sensor 1 OK
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[1].id - 1)
    ); // Sensor 2 corrupted
}

TEST(SensorSampleToEvent, NullDataPointerSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 2, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    mock_samples[1].data = NULL; // Force second sensor to have NULL data pointer
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_FALSE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1)); // Sensor 1 OK
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[1].id - 1)
    ); // Sensor 2 corrupted
}

TEST(SensorSampleToEvent, NullOutlierPointerSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 2, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    mock_samples[1].outlier = NULL; // Force second sensor to have NULL outlier pointer
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_FALSE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1)); // Sensor 1 OK
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[1].id - 1)
    ); // Sensor 2 corrupted
}

TEST(SensorSampleToEvent, UnsupportedDataTypeSetsCorruption) {
    gMonSensor_t sensor_cfg = {.num_items = 2, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    // Force second sensor to have unsupported data type
    mock_samples[1].dtype = GMON_SENSOR_DATA_TYPE_UNKNOWN;
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_FALSE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[0].id - 1)); // Sensor 1 OK
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, mock_samples[1].id - 1)
    ); // Sensor 2 corrupted
}

TEST(SensorSampleToEvent, MixedCorruptionFlags) {
    gMonSensor_t sensor_cfg = {.num_items = 4, .num_resamples = 3};
    mock_samples = staAllocSensorSampleBuffer(&sensor_cfg, GMON_SENSOR_DATA_TYPE_U32);
    TEST_ASSERT_NOT_NULL(mock_samples);
    mock_event =
        createAndInitEvent(GMON_EVENT_SOIL_MOISTURE_UPDATED, sensor_cfg.num_items, sizeof(unsigned int));
    TEST_ASSERT_NOT_NULL(mock_event);
    // Sensor 1 (ID 1): Good path
    ((unsigned int *)mock_samples[0].data)[0] = 10;
    ((unsigned int *)mock_samples[0].data)[1] = 11;
    ((unsigned int *)mock_samples[0].data)[2] = 12;
    // Sensor 2 (ID 2): More than half outliers (2 out of 3)
    ((unsigned int *)mock_samples[1].data)[0] = 10;
    ((unsigned int *)mock_samples[1].data)[1] = 100;
    ((unsigned int *)mock_samples[1].data)[2] = 200;
    staSetBitFlag(mock_samples[1].outlier, 1, 1);
    staSetBitFlag(mock_samples[1].outlier, 2, 1);
    // Sensor 3 (ID 3): Data type mismatch
    mock_samples[2].dtype = GMON_SENSOR_DATA_TYPE_AIRCOND;
    // Sensor 4 (ID 4): Null data pointer
    mock_samples[3].data = NULL;
    gMonStatus status = staSensorSampleToEvent(mock_event, mock_samples);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_FALSE(staGetBitFlag(&mock_event->flgs.corruption, 0)); // Sensor 1 (ID 1) OK
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, 1)
    ); // Sensor 2 (ID 2) corrupted (too many outliers)
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, 2)
    ); // Sensor 3 (ID 3) corrupted (dtype mismatch)
    TEST_ASSERT_TRUE(staGetBitFlag(&mock_event->flgs.corruption, 3)); // Sensor 4 (ID 4) corrupted (NULL data)
}

TEST_GROUP_RUNNER(gMonSensorSample) {
    RUN_TEST_CASE(SensorSampleAlloc, initOk);
    RUN_TEST_CASE(SensorNoiseDetection, ErrMissingArgs);
    RUN_TEST_CASE(SensorNoiseDetection, U32FewOutliers);
    RUN_TEST_CASE(SensorNoiseDetection, AirCondFewOutliers);
    RUN_TEST_CASE(SensorNoiseDetection, U32HalfOutliers);
    RUN_TEST_CASE(SensorSampleToEvent, ErrArgsNullZero);
    RUN_TEST_CASE(SensorSampleToEvent, U32HappyPathNoOutliers);
    RUN_TEST_CASE(SensorSampleToEvent, AirCondHappyPathNoOutliers);
    RUN_TEST_CASE(SensorSampleToEvent, U32SomeOutliersBelowThreshold);
    RUN_TEST_CASE(SensorSampleToEvent, AirCondSomeOutliersBelowThreshold);
    RUN_TEST_CASE(SensorSampleToEvent, U32ManyOutliersSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, AirCondManyOutliersSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, MismatchedDataTypeSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, ZeroLenSampleSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, NullDataPointerSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, NullOutlierPointerSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, UnsupportedDataTypeSetsCorruption);
    RUN_TEST_CASE(SensorSampleToEvent, MixedCorruptionFlags);
}
