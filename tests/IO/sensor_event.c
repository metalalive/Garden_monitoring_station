#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"
#include "mocks.h"

static gardenMonitor_t     gmon; // Global instance for setup/teardown
static gmonSensorSample_t *mock_samples = NULL;

// `stationIOinit` and `stationIOdeinit` require a full `gardenMonitor_t` instance.
// It is already included via `station_include.h`.

TEST_GROUP(SensorEvtPool);
TEST_GROUP(cpySensorEvent);
TEST_GROUP(SensorNoiseDetection);

TEST_SETUP(SensorEvtPool) {
    gMonStatus status = stationIOinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_TEAR_DOWN(SensorEvtPool) {
    gMonStatus status = stationIOdeinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_SETUP(cpySensorEvent) {
    gMonStatus status = stationIOinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_TEAR_DOWN(cpySensorEvent) {
    gMonStatus status = stationIOdeinit(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

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

// Test cases for staAllocSensorEvent
TEST(SensorEvtPool, AllocatesFromEmptyPool) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // It should be empty at the start of this test.
    gmonEvent_t *event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[0], event);
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    // Verify memory is zeroed out (except for the alloc flag which is set to 1)
    TEST_ASSERT_EQUAL_UINT32(0, event->event_type);
    TEST_ASSERT_EQUAL_UINT32(0, event->data.soil_moist); // Checking one union member is sufficient
    TEST_ASSERT_EQUAL_UINT32(0, event->curr_ticks);
}

TEST(SensorEvtPool, AllocatesFromPartiallyFilledPool) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Allocate first event
    gmonEvent_t *event1 = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(event1);
    TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[0], event1);
    TEST_ASSERT_EQUAL(1, event1->flgs.alloc);
    // Allocate second event
    gmonEvent_t *event2 = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(event2);
    TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[1], event2);
    TEST_ASSERT_EQUAL(1, event2->flgs.alloc);
    TEST_ASSERT_NOT_EQUAL(event1, event2); // Ensure different events are returned
}

TEST(SensorEvtPool, PoolFullTriggersAssertion) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Its size is GMON_NUM_SENSOR_EVENTS (defined as GMON_CFG_NUM_SENSOR_RECORDS_KEEP * 3 = 15).
    // Fill the entire event pool
    gmonEvent_t *allocated_events[GMON_NUM_SENSOR_EVENTS];
    for (size_t i = 0; i < GMON_NUM_SENSOR_EVENTS; i++) {
        allocated_events[i] = staAllocSensorEvent(&gmon);
        TEST_ASSERT_NOT_NULL(allocated_events[i]);
        TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[i], allocated_events[i]);
        TEST_ASSERT_EQUAL(1, allocated_events[i]->flgs.alloc);
    }
    // Attempt to allocate another event when pool is full.
    // The function `staAllocSensorEvent` should return NULL
    gmonEvent_t *nxt_evt = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NULL(nxt_evt);
}

// Test cases for staFreeSensorEvent
TEST(SensorEvtPool, FreesValidAllocatedEvent) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[0], event);
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    gMonStatus status = staFreeSensorEvent(&gmon, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, event->flgs.alloc); // Verify alloc flag is cleared
}

TEST(SensorEvtPool, FreesEventAlreadyFreedSafely) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    gMonStatus status1 = staFreeSensorEvent(&gmon, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status1);
    TEST_ASSERT_EQUAL(0, event->flgs.alloc);
    // Free again
    gMonStatus status2 = staFreeSensorEvent(&gmon, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status2);
    TEST_ASSERT_EQUAL(0, event->flgs.alloc); // Should still be 0
}

TEST(SensorEvtPool, ReturnsErrorForNullRecord) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gMonStatus status = staFreeSensorEvent(&gmon, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status); // NULL record is outside range [addr_start, addr_end)
}

TEST(SensorEvtPool, ReturnsErrorForRecordOutsidePoolBounds_BeforeStart) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Create a pointer just before the pool starts
    gmonEvent_t *invalid_record = gmon.sensors.event.pool - 1;
    gMonStatus   status = staFreeSensorEvent(&gmon, invalid_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
}

TEST(SensorEvtPool, ReturnsErrorForRecordOutsidePoolBounds_AfterEnd) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Create a pointer just after the pool ends
    // (gmon.sensors.event.pool + gmon.sensors.event.len is one past the last valid element)
    gmonEvent_t *invalid_record = gmon.sensors.event.pool + gmon.sensors.event.len;
    gMonStatus   status = staFreeSensorEvent(&gmon, invalid_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
}

TEST(SensorEvtPool, ReturnsErrorForMisalignedRecord) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(event);
    // Create a misaligned pointer (e.g., one byte into the event struct)
    gmonEvent_t *misaligned_record = (gmonEvent_t *)((char *)event + 1);
    gMonStatus   status = staFreeSensorEvent(&gmon, misaligned_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
}

// Test cases for staCpySensorEvent
TEST(cpySensorEvent, ReturnsErrorForNullDst) {
    gmonEvent_t *src_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(src_event);
    gMonStatus status = staCpySensorEvent(NULL, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(&gmon, src_event);
}

TEST(cpySensorEvent, ReturnsErrorForNullSrc) {
    gmonEvent_t *dst_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, NULL, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(&gmon, dst_event);
}

TEST(cpySensorEvent, ReturnsErrorForZeroSize) {
    gmonEvent_t *src_event = staAllocSensorEvent(&gmon);
    gmonEvent_t *dst_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(src_event);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 0);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(&gmon, src_event);
    staFreeSensorEvent(&gmon, dst_event);
}

TEST(cpySensorEvent, CopiesSingleEventSuccessfully) {
    gmonEvent_t *src_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(src_event);
    // Populate source event with test data
    src_event->event_type = GMON_EVENT_AIR_TEMP_UPDATED;
    src_event->data.air_cond.temporature = 25.5f;
    src_event->data.air_cond.humidity = 60.0f;
    src_event->curr_ticks = 1000;
    src_event->curr_days = 5;
    // flgs.alloc is already 1 due to staAllocSensorEvent
    gmonEvent_t *dst_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Verify destination event content
    TEST_ASSERT_EQUAL(src_event->event_type, dst_event->event_type);
    TEST_ASSERT_EQUAL_FLOAT(src_event->data.air_cond.temporature, dst_event->data.air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(src_event->data.air_cond.humidity, dst_event->data.air_cond.humidity);
    TEST_ASSERT_EQUAL(src_event->curr_ticks, dst_event->curr_ticks);
    TEST_ASSERT_EQUAL(src_event->curr_days, dst_event->curr_days);
    TEST_ASSERT_EQUAL(
        src_event->flgs.alloc, dst_event->flgs.alloc
    ); // Alloc flag should also be copied (value 1)
    staFreeSensorEvent(&gmon, src_event);
    staFreeSensorEvent(&gmon, dst_event);
}

TEST(cpySensorEvent, CopiesMultipleEventsSuccessfully) {
    const size_t num_events_to_copy = 2;
    gmonEvent_t *src_events[num_events_to_copy];
    gmonEvent_t *dst_events[num_events_to_copy];

    for (size_t i = 0; i < num_events_to_copy; i++) {
        src_events[i] = staAllocSensorEvent(&gmon);
        TEST_ASSERT_NOT_NULL(src_events[i]);
        src_events[i]->event_type = (gmonEventType_t)(GMON_EVENT_SOIL_MOISTURE_UPDATED + i);
        src_events[i]->data.soil_moist = 100 + (unsigned int)i;
        src_events[i]->curr_ticks = 2000 + (unsigned int)i;
        src_events[i]->curr_days = 10 + (unsigned int)i;
        dst_events[i] = staAllocSensorEvent(&gmon);
        TEST_ASSERT_NOT_NULL(dst_events[i]);
    }
    gMonStatus status = staCpySensorEvent(dst_events[0], src_events[0], num_events_to_copy);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    for (size_t i = 0; i < num_events_to_copy; i++) {
        TEST_ASSERT_EQUAL(src_events[i]->event_type, dst_events[i]->event_type);
        TEST_ASSERT_EQUAL(src_events[i]->data.soil_moist, dst_events[i]->data.soil_moist);
        TEST_ASSERT_EQUAL(src_events[i]->curr_ticks, dst_events[i]->curr_ticks);
        TEST_ASSERT_EQUAL(src_events[i]->curr_days, dst_events[i]->curr_days);
        TEST_ASSERT_EQUAL(src_events[i]->flgs.alloc, dst_events[i]->flgs.alloc);
    }
    for (size_t i = 0; i < num_events_to_copy; i++) {
        staFreeSensorEvent(&gmon, src_events[i]);
        staFreeSensorEvent(&gmon, dst_events[i]);
    }
}

TEST(cpySensorEvent, ReturnsErrorIfDstNotAllocated) {
    gmonEvent_t *src_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(src_event);
    gmonEvent_t *dst_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(dst_event);
    dst_event->flgs.alloc = 0; // Simulate unallocated destination
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    dst_event->flgs.alloc = 1;
    staFreeSensorEvent(&gmon, src_event);
    staFreeSensorEvent(&gmon, dst_event);
}

TEST(cpySensorEvent, ReturnsErrorIfSrcNotAllocated) {
    gmonEvent_t *src_event = staAllocSensorEvent(&gmon);
    gmonEvent_t *dst_event = staAllocSensorEvent(&gmon);
    TEST_ASSERT_NOT_NULL(src_event);
    TEST_ASSERT_NOT_NULL(dst_event);
    src_event->flgs.alloc = 0;
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    src_event->flgs.alloc = 1;
    staFreeSensorEvent(&gmon, src_event);
    staFreeSensorEvent(&gmon, dst_event);
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

TEST_GROUP_RUNNER(gMonSensorEvt) {
    RUN_TEST_CASE(SensorEvtPool, AllocatesFromEmptyPool);
    RUN_TEST_CASE(SensorEvtPool, AllocatesFromPartiallyFilledPool);
    RUN_TEST_CASE(SensorEvtPool, PoolFullTriggersAssertion);
    RUN_TEST_CASE(SensorEvtPool, FreesValidAllocatedEvent);
    RUN_TEST_CASE(SensorEvtPool, FreesEventAlreadyFreedSafely);
    RUN_TEST_CASE(SensorEvtPool, ReturnsErrorForNullRecord);
    RUN_TEST_CASE(SensorEvtPool, ReturnsErrorForRecordOutsidePoolBounds_BeforeStart);
    RUN_TEST_CASE(SensorEvtPool, ReturnsErrorForRecordOutsidePoolBounds_AfterEnd);
    RUN_TEST_CASE(SensorEvtPool, ReturnsErrorForMisalignedRecord);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorForNullDst);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorForNullSrc);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorForZeroSize);
    RUN_TEST_CASE(cpySensorEvent, CopiesSingleEventSuccessfully);
    RUN_TEST_CASE(cpySensorEvent, CopiesMultipleEventsSuccessfully);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorIfDstNotAllocated);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorIfSrcNotAllocated);
    RUN_TEST_CASE(SensorNoiseDetection, ErrMissingArgs);
    RUN_TEST_CASE(SensorNoiseDetection, U32FewOutliers);
    RUN_TEST_CASE(SensorNoiseDetection, AirCondFewOutliers);
    RUN_TEST_CASE(SensorNoiseDetection, U32HalfOutliers);
}
