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
    gmonEvent_t *event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 3);
    TEST_ASSERT_NOT_NULL(event);
    TEST_ASSERT_EQUAL_PTR(&epool->pool[0], event);
    TEST_ASSERT_NOT_NULL(event->data); // Ensure data buffer is allocated
    TEST_ASSERT_EQUAL(1, event->flgs.alloc);
    TEST_ASSERT_EQUAL(3, event->num_active_sensors);
    TEST_ASSERT_EQUAL(GMON_EVENT_SOIL_MOISTURE_UPDATED, event->event_type);
    // Verify allocated data buffer is zeroed out.
    TEST_ASSERT_EQUAL_UINT32(0, ((unsigned int *)event->data)[0]);
    TEST_ASSERT_EQUAL_UINT32(0, event->curr_ticks);
    gMonStatus status1 = staFreeSensorEvent(epool, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status1);
}

TEST(SensorEvtPool, AllocatesFromPartiallyFilledPool) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event1 = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 4);
    TEST_ASSERT_NOT_NULL(event1);
    // Allocate second event
    gmonEvent_t *event2 = staAllocSensorEvent(epool, GMON_EVENT_AIR_TEMP_UPDATED, 5);
    TEST_ASSERT_NOT_NULL(event2);
    unsigned int  *evtdata1 = event1->data;
    gmonAirCond_t *evtdata2 = event2->data;
    evtdata1[3] = 377;
    evtdata2[4] = (gmonAirCond_t){.temporature = 15.5, .humidity = 78.5};
    TEST_ASSERT_EQUAL_PTR(&epool->pool[0], event1);
    TEST_ASSERT_EQUAL(1, event1->flgs.alloc);
    TEST_ASSERT_NOT_NULL(event1->data);
    TEST_ASSERT_EQUAL_PTR(&epool->pool[1], event2);
    TEST_ASSERT_EQUAL(1, event2->flgs.alloc);
    TEST_ASSERT_NOT_EQUAL(event1, event2); // Ensure different events are returned
    TEST_ASSERT_EQUAL_FLOAT(15.5, evtdata2[4].temporature);
    TEST_ASSERT_EQUAL_FLOAT(78.5, evtdata2[4].humidity);
    gMonStatus status1 = staFreeSensorEvent(epool, event1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status1);
    status1 = staFreeSensorEvent(epool, event2);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status1);
}

TEST(SensorEvtPool, PoolFullTriggersAssertion) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    unsigned int   i = 0;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    // Its size is GMON_NUM_SENSOR_EVENTS (defined as GMON_CFG_NUM_SENSOR_RECORDS_KEEP * 3 = 15).
    // Fill the entire event pool
    gmonEvent_t *allocated_events[GMON_NUM_SENSOR_EVENTS]; // Use unsigned int for loop counter
    for (i = 0; i < GMON_NUM_SENSOR_EVENTS; i++) {
        allocated_events[i] = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
        TEST_ASSERT_NOT_NULL(allocated_events[i]);
        TEST_ASSERT_EQUAL_PTR(&gmon.sensors.event.pool[i], allocated_events[i]);
        TEST_ASSERT_EQUAL(1, allocated_events[i]->flgs.alloc);
    }
    // Attempt to allocate another event when pool is full.
    // The function `staAllocSensorEvent` should return NULL
    gmonEvent_t *nxt_evt = staAllocSensorEvent(epool, GMON_EVENT_AIR_TEMP_UPDATED, 3);
    TEST_ASSERT_NULL(nxt_evt);
    for (i = 0; i < GMON_NUM_SENSOR_EVENTS; i++) {
        gMonStatus status = staFreeSensorEvent(epool, allocated_events[i]);
        TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    }
}

// Test cases for staFreeSensorEvent
TEST(SensorEvtPool, FreesValidAllocatedEvent) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    // The pool is initialized by stationIOinit in TEST_SETUP.
    gmonEvent_t *event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
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
    gmonEvent_t *event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
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
    gmonEvent_t *event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(event);
    // Create a misaligned pointer (e.g., one byte into the event struct)
    gmonEvent_t *misaligned_record = (gmonEvent_t *)((char *)event + 1);
    gMonStatus   status = staFreeSensorEvent(epool, misaligned_record);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    status = staFreeSensorEvent(epool, event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(cpySensorEvent, ReturnsErrorForNullDst) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(src_event);
    gMonStatus status = staCpySensorEvent(NULL, src_event);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(epool, src_event);
}

TEST(cpySensorEvent, ReturnsErrorForNullSrc) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *dst_event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    staFreeSensorEvent(epool, dst_event);
}

TEST(cpySensorEvent, CopySingleEventOk) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    unsigned char  num_sensors = 3;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool, GMON_EVENT_AIR_TEMP_UPDATED, num_sensors);
    TEST_ASSERT_NOT_NULL(src_event);
    // Populate source event with test data
    src_event->event_type = GMON_EVENT_AIR_TEMP_UPDATED;
    gmonAirCond_t *src_aircond_data = (gmonAirCond_t *)src_event->data;
    src_aircond_data[0] = (gmonAirCond_t){.temporature = 25.5f, .humidity = 60.0f};
    src_aircond_data[1] = (gmonAirCond_t){.temporature = 26.0f, .humidity = 62.5f};
    src_aircond_data[2] = (gmonAirCond_t){.temporature = 24.8f, .humidity = 58.2f};
    src_event->curr_ticks = 1000;
    src_event->curr_days = 5;
    // flgs.alloc is already 1 due to staAllocSensorEvent
    gmonEvent_t *dst_event = staAllocSensorEvent(epool, GMON_EVENT_AIR_TEMP_UPDATED, num_sensors);
    TEST_ASSERT_NOT_NULL(dst_event);
    gMonStatus status = staCpySensorEvent(dst_event, src_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Verify destination event content
    TEST_ASSERT_EQUAL(src_event->event_type, dst_event->event_type);
    TEST_ASSERT_EQUAL(src_event->num_active_sensors, dst_event->num_active_sensors);
    TEST_ASSERT_EQUAL(num_sensors, dst_event->num_active_sensors);
    TEST_ASSERT_EQUAL(src_event->curr_ticks, dst_event->curr_ticks);
    TEST_ASSERT_EQUAL(src_event->curr_days, dst_event->curr_days);
    // Alloc flag should also be copied (value 1)
    TEST_ASSERT_EQUAL(src_event->flgs.alloc, dst_event->flgs.alloc);
    // The data pointer itself is not copied, nor its content
    TEST_ASSERT_NOT_EQUAL(src_event->data, dst_event->data);
    // Verify dst_event's data buffer content is copied
    gmonAirCond_t *dst_aircond_data = (gmonAirCond_t *)dst_event->data;
    for (unsigned char i = 0; i < num_sensors; i++) {
        TEST_ASSERT_EQUAL_FLOAT(src_aircond_data[i].temporature, dst_aircond_data[i].temporature);
        TEST_ASSERT_EQUAL_FLOAT(src_aircond_data[i].humidity, dst_aircond_data[i].humidity);
    }
    staFreeSensorEvent(epool, src_event);
    staFreeSensorEvent(epool, dst_event);
}

TEST(cpySensorEvent, ReturnsErrorIfDstNotAllocated) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(src_event);
    gmonEvent_t *dst_event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(dst_event);
    dst_event->flgs.alloc = 0; // Simulate unallocated destination
    gMonStatus status = staCpySensorEvent(dst_event, src_event);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    dst_event->flgs.alloc = 1;
    staFreeSensorEvent(epool, src_event); // This will free the `data` member too
    staFreeSensorEvent(epool, dst_event); // This will free the `data` member too
}

TEST(cpySensorEvent, ReturnsErrorIfSrcNotAllocated) {
    gMonEvtPool_t *epool = &gmon.sensors.event;
    gmonEvent_t   *src_event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(src_event);
    gmonEvent_t *dst_event = staAllocSensorEvent(epool, GMON_EVENT_SOIL_MOISTURE_UPDATED, 1);
    TEST_ASSERT_NOT_NULL(dst_event);
    src_event->flgs.alloc = 0;
    gMonStatus status = staCpySensorEvent(dst_event, src_event);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    src_event->flgs.alloc = 1;
    staFreeSensorEvent(epool, src_event); // This will free the `data` member too
    staFreeSensorEvent(epool, dst_event); // This will free the `data` member too
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
    RUN_TEST_CASE(cpySensorEvent, CopySingleEventOk);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorIfDstNotAllocated);
    RUN_TEST_CASE(cpySensorEvent, ReturnsErrorIfSrcNotAllocated);
}
