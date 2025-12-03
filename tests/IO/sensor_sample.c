#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

TEST_GROUP(SensorSampleAlloc);

TEST_SETUP(SensorSampleAlloc) {}

TEST_TEAR_DOWN(SensorSampleAlloc) {}

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

TEST_GROUP_RUNNER(gMonSensorSample) { RUN_TEST_CASE(SensorSampleAlloc, initOk); }
