#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"
#include "mocks.h"

static gardenMonitor_t gmon; // Global instance for setup/teardown

// `stationIOinit` and `stationIOdeinit` require a full `gardenMonitor_t` instance.
// It is already included via `station_include.h`.

TEST_GROUP(SensorEvtPool);
TEST_GROUP(cpySensorEvent);

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

TEST(SensorEvtPool, AllocatesFromEmptyPool) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // It should be empty at the start of this test.
    gmonEvent_t *event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_PTR(&epool->pool[0], event);
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    // Verify memory is zeroed out (except for the alloc flag which is set to 1)
    TEST_ASSERT_EQUAL_UINT32(0, event->event_type);
    TEST_ASSERT_EQUAL_UINT32(0, event->data.soil_moist); // Checking one union member is sufficient
    TEST_ASSERT_EQUAL_UINT32(0, event->curr_ticks);
}

TEST(SensorEvtPool, AllocatesFromPartiallyFilledPool) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Allocate first event
    gmonEvent_t *event1 = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(event1);
    TEST_ASSERT_EQUAL_PTR(&epool->pool[0], event1);
    TEST_ASSERT_EQUAL(1, event1->flgs.alloc);
    // Allocate second event
    gmonEvent_t *event2 = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(event2);
    TEST_ASSERT_EQUAL_PTR(&epool->pool[1], event2);
    TEST_ASSERT_EQUAL(1, event2->flgs.alloc);
    TEST_ASSERT_NOT_EQUAL(event1, event2); // Ensure different events are returned
}

TEST(SensorEvtPool, PoolFullTriggersAssertion) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Its size is GMON_NUM_SENSOR_EVENTS (defined as GMON_CFG_NUM_SENSOR_RECORDS_KEEP * 3 = 15).
    // Fill the entire event pool
    gmonEvent_t *allocated_events[GMON_NUM_SENSOR_EVENTS];
    for (size_t i = 0; i < GMON_NUM_SENSOR_EVENTS; i++) {
        allocated_events[i] = staAllocSensorEvent(epool);
        TEST_ASSERT_NOT_NULL(allocated_events[i]);
        TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[i], allocated_events[i]);
        TEST_ASSERT_EQUAL(1, allocated_events[i]->flgs.alloc);
    }
    // Attempt to allocate another event when pool is full.
    // The function `staAllocSensorEvent` should return NULL
    gmonEvent_t *nxt_evt = staAllocSensorEvent(epool);
    TEST_ASSERT_NULL(nxt_evt);
}

// Test cases for staFreeSensorEvent
TEST(SensorEvtPool, FreesValidAllocatedEvent) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_PTR(&epool->pool[0], event);
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    gMonStatus status = staFreeSensorEvent(epool, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, event->flgs.alloc); // Verify alloc flag is cleared
}

TEST(SensorEvtPool, FreesEventAlreadyFreedSafely) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    gMonStatus status1 = staFreeSensorEvent(epool, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status1);
    TEST_ASSERT_EQUAL(0, event->flgs.alloc);
    // Free again
    gMonStatus status2 = staFreeSensorEvent(epool, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status2);
    TEST_ASSERT_EQUAL(0, event->flgs.alloc); // Should still be 0
}

TEST(SensorEvtPool, ReturnsErrorForNullRecord) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gMonStatus status = staFreeSensorEvent(&gmon.sensors.event, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status); // NULL record is outside range [addr_start, addr_end)
}

TEST(SensorEvtPool, ReturnsErrorForRecordOutsidePoolBounds_BeforeStart) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Create a pointer just before the pool starts
    gmonEvent_t *invalid_record = gmon.sensors.event.pool - 1;
    gMonStatus   status = staFreeSensorEvent(&gmon.sensors.event, invalid_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
}

TEST(SensorEvtPool, ReturnsErrorForRecordOutsidePoolBounds_AfterEnd) {
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Create a pointer just after the pool ends
    // (gmon.sensors.event.pool + gmon.sensors.event.len is one past the last valid element)
    gmonEvent_t *invalid_record = gmon.sensors.event.pool + gmon.sensors.event.len;
    gMonStatus   status = staFreeSensorEvent(&gmon.sensors.event, invalid_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
}

TEST(SensorEvtPool, ReturnsErrorForMisalignedRecord) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(event);
    // Create a misaligned pointer (e.g., one byte into the event struct)
    gmonEvent_t *misaligned_record = (gmonEvent_t *)((char *)event + 1);
    gMonStatus   status = staFreeSensorEvent(epool, misaligned_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
}

TEST(cpySensorEvent, ReturnsErrorForNullDst) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(src_event);
    gMonStatus status = staCpySensorEvent(NULL, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(epool, src_event);
}

TEST(cpySensorEvent, ReturnsErrorForNullSrc) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *dst_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, NULL, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(epool, dst_event);
}

TEST(cpySensorEvent, ReturnsErrorForZeroSize) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool), *dst_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(src_event);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 0);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(epool, src_event);
    staFreeSensorEvent(epool, dst_event);
}

TEST(cpySensorEvent, CopiesSingleEventSuccessfully) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(src_event);
    // Populate source event with test data
    src_event->event_type = GMON_EVENT_AIR_TEMP_UPDATED;
    src_event->data.air_cond.temporature = 25.5f;
    src_event->data.air_cond.humidity = 60.0f;
    src_event->curr_ticks = 1000;
    src_event->curr_days = 5;
    // flgs.alloc is already 1 due to staAllocSensorEvent
    gmonEvent_t *dst_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Verify destination event content
    TEST_ASSERT_EQUAL(src_event->event_type, dst_event->event_type);
    TEST_ASSERT_EQUAL_FLOAT(src_event->data.air_cond.temporature, dst_event->data.air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(src_event->data.air_cond.humidity, dst_event->data.air_cond.humidity);
    TEST_ASSERT_EQUAL(src_event->curr_ticks, dst_event->curr_ticks);
    TEST_ASSERT_EQUAL(src_event->curr_days, dst_event->curr_days);
    // Alloc flag should also be copied (value 1)
    TEST_ASSERT_EQUAL(src_event->flgs.alloc, dst_event->flgs.alloc);
    staFreeSensorEvent(epool, src_event);
    staFreeSensorEvent(epool, dst_event);
}

TEST(cpySensorEvent, CopiesMultipleEventsSuccessfully) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    const size_t   num_events_to_copy = 2;
    gmonEvent_t   *src_events[num_events_to_copy], *dst_events[num_events_to_copy];

    for (size_t i = 0; i < num_events_to_copy; i++) {
        src_events[i] = staAllocSensorEvent(epool);
        TEST_ASSERT_NOT_NULL(src_events[i]);
        src_events[i]->event_type = (gmonEventType_t)(GMON_EVENT_SOIL_MOISTURE_UPDATED + i);
        src_events[i]->data.soil_moist = 100 + (unsigned int)i;
        src_events[i]->curr_ticks = 2000 + (unsigned int)i;
        src_events[i]->curr_days = 10 + (unsigned int)i;
        dst_events[i] = staAllocSensorEvent(epool);
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
        staFreeSensorEvent(epool, src_events[i]);
        staFreeSensorEvent(epool, dst_events[i]);
    }
}

TEST(cpySensorEvent, ReturnsErrorIfDstNotAllocated) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(src_event);
    gmonEvent_t *dst_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(dst_event);
    dst_event->flgs.alloc = 0; // Simulate unallocated destination
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    dst_event->flgs.alloc = 1;
    staFreeSensorEvent(epool, src_event);
    staFreeSensorEvent(epool, dst_event);
}

TEST(cpySensorEvent, ReturnsErrorIfSrcNotAllocated) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool), *dst_event = staAllocSensorEvent(epool);
    TEST_ASSERT_NOT_NULL(src_event);
    TEST_ASSERT_NOT_NULL(dst_event);
    src_event->flgs.alloc = 0;
    gMonStatus status = staCpySensorEvent(dst_event, src_event, 1);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    src_event->flgs.alloc = 1;
    staFreeSensorEvent(epool, src_event);
    staFreeSensorEvent(epool, dst_event);
}

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
}
