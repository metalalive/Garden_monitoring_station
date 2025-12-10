#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

static gardenMonitor_t     gmon; // Global instance for setup/teardown
static gmonSensorSample_t *mock_samples = NULL;

TEST_GROUP(SensorSampleAlloc);
TEST_GROUP(SensorNoiseDetection);

TEST_SETUP(SensorSampleAlloc) {}

TEST_TEAR_DOWN(SensorSampleAlloc) {}

TEST_SETUP(SensorNoiseDetection) {
    gMonStatus status = stationIOinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_TEAR_DOWN(SensorNoiseDetection) {
    gMonStatus status = stationIOdeinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    XMEMFREE(mock_samples);
    mock_samples = NULL;
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

TEST_GROUP_RUNNER(gMonSensorSample) {
    RUN_TEST_CASE(SensorSampleAlloc, initOk);
    RUN_TEST_CASE(SensorNoiseDetection, ErrMissingArgs);
    RUN_TEST_CASE(SensorNoiseDetection, U32FewOutliers);
    RUN_TEST_CASE(SensorNoiseDetection, AirCondFewOutliers);
    RUN_TEST_CASE(SensorNoiseDetection, U32HalfOutliers);
}
