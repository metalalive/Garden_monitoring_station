#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

// Test fixtures
static gardenMonitor_t test_gmon; // Global gardenMonitor_t for tests

#if 0  // temporary mask
// New test group for staGetAppMsgOutflight
TEST_GROUP(GenerateMsgOutflight);

TEST_SETUP(GenerateMsgOutflight) {
    // Reset test_gmon and initialize app messages
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    staAppMsgInit(&test_gmon);
    // After staAppMsgInit, the latest_records are reset and the outflight buffer is initialized
}

TEST_TEAR_DOWN(GenerateMsgOutflight) { staAppMsgDeinit(&test_gmon); }

// Helper function to set a single sensor record for testing output generation
static void set_test_sensor_record(
    gmonSensorRecord_t *record, float air_temp, float air_humid, unsigned int soil_moist,
    unsigned int lightness, unsigned int ticks, unsigned int days
) {
    record->air_cond.temporature = air_temp;
    record->air_cond.humidity = air_humid;
    record->soil_moist = soil_moist;
    record->lightness = lightness;
    record->curr_ticks = ticks;
    record->curr_days = days;
    // For staGetAppMsgOutflight, these flags don't affect output, but typically would be set
    record->flgs.air_val_written = 1;
    record->flgs.soil_val_written = 1;
    record->flgs.light_val_written = 1;
}

TEST(GenerateMsgOutflight, EmptyRecordsOutput) {
    // Initially, all records should be zeroed out by staAppMsgInit and staAppMsgOutResetAllRecords
    gmonStr_t  *out_msg = staGetAppMsgOutflight(&test_gmon);
    const char *expected_json =
        "{\"records\":[{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0 "
        "      ,\"days\":0     },{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   "
        ",\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     "
        ",\"light\":0   ,\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   ,\"airtemp\":0     "
        ",\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   "
        ",\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     }]}";
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, out_msg->len);

    // Verify records are reset after retrieval
    for (int i = 0; i < GMON_CFG_NUM_SENSOR_RECORDS_KEEP; i++) {
        gmonSensorRecord_t *rec = &test_gmon.sensors.latest_records[i];
        TEST_ASSERT_EQUAL(0, rec->soil_moist);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, rec->air_cond.temporature);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, rec->air_cond.humidity);
        TEST_ASSERT_EQUAL(0, rec->lightness);
        TEST_ASSERT_EQUAL(0, rec->curr_ticks);
        TEST_ASSERT_EQUAL(0, rec->curr_days);
    }
}

TEST(GenerateMsgOutflight, SinglePopulatedRecord) {
    set_test_sensor_record(&test_gmon.sensors.latest_records[0], 25.5f, 75.25f, 800, 500, 1234567, 10);

    gmonStr_t  *out_msg = staGetAppMsgOutflight(&test_gmon);
    const char *expected_json =
        "{\"records\":[{\"soilmoist\":800 ,\"airtemp\":25.50 ,\"airhumid\":75.25 ,\"light\":500 "
        ",\"ticks\":1234567 ,\"days\":10    },{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     "
        ",\"light\":0   ,\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   ,\"airtemp\":0     "
        ",\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   "
        ",\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     "
        "},{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       "
        ",\"days\":0     }]}";
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, out_msg->len);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.latest_records[0].soil_moist); // Verify records are reset
}

TEST(GenerateMsgOutflight, MultiplePopulatedRecords) {
    set_test_sensor_record(&test_gmon.sensors.latest_records[0], 22.0f, 60.0f, 700, 300, 1000000, 5);
    set_test_sensor_record(&test_gmon.sensors.latest_records[1], 23.1f, 65.5f, 750, 400, 1000100, 5);

    gmonStr_t  *out_msg = staGetAppMsgOutflight(&test_gmon);
    const char *expected_json =
        "{\"records\":[{\"soilmoist\":700 ,\"airtemp\":22    ,\"airhumid\":60    ,\"light\":300 "
        ",\"ticks\":1000000 ,\"days\":5     },{\"soilmoist\":750 ,\"airtemp\":23.10 ,\"airhumid\":65.50 "
        ",\"light\":400 ,\"ticks\":1000100 ,\"days\":5     },{\"soilmoist\":0   ,\"airtemp\":0     "
        ",\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   "
        ",\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     "
        "},{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       "
        ",\"days\":0     }]}";
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, out_msg->len);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.latest_records[0].soil_moist); // Verify records are reset
}

TEST(GenerateMsgOutflight, RecordsWithExtremeValues) {
    set_test_sensor_record(&test_gmon.sensors.latest_records[0], -10.75f, 999.25f, 0, 999, 99999999, 365000);
    set_test_sensor_record(&test_gmon.sensors.latest_records[1], 987.50f, -99.75f, 9999, 0, 99999998, 365002);

    gmonStr_t  *out_msg = staGetAppMsgOutflight(&test_gmon);
    const char *expected_json =
        "{\"records\":[{\"soilmoist\":0   ,\"airtemp\":-10.75,\"airhumid\":999.25,\"light\":999 "
        ",\"ticks\":99999999,\"days\":365000},{\"soilmoist\":9999,\"airtemp\":987.50,\"airhumid\":-99.75,"
        "\"light\":0   ,\"ticks\":99999998,\"days\":365002},{\"soilmoist\":0   ,\"airtemp\":0     "
        ",\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     },{\"soilmoist\":0   "
        ",\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       ,\"days\":0     "
        "},{\"soilmoist\":0   ,\"airtemp\":0     ,\"airhumid\":0     ,\"light\":0   ,\"ticks\":0       "
        ",\"days\":0     }]}";
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, out_msg->len);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.latest_records[0].soil_moist); // Verify records are reset
}
#endif // temporary mask

// New test group for staUpdateLastRecord
TEST_GROUP(UpdateLastRecord);

static unsigned int  ut_mockmem_soilmoist[5];
static unsigned int  ut_mockmem_lightness[5];
static gmonAirCond_t ut_mockmem_aircond[5];
static int           ut_mockidx_soilmoist;
static int           ut_mockidx_lightness;
static int           ut_mockidx_aircond;

static gmonEvent_t create_test_event(
    gmonEventType_t type, unsigned int soil_moist, float air_temp, float air_humid, unsigned int lightness,
    unsigned int ticks, unsigned int days
) {
    gmonEvent_t evt = {
        .event_type = type,
        .curr_ticks = ticks,
        .curr_days = days,
    };
    switch (evt.event_type) {
    case GMON_EVENT_SOIL_MOISTURE_UPDATED:
        ut_mockmem_soilmoist[ut_mockidx_soilmoist] = soil_moist;
        evt.data = &ut_mockmem_soilmoist[ut_mockidx_soilmoist++];
        break;
    case GMON_EVENT_LIGHTNESS_UPDATED:
        ut_mockmem_lightness[ut_mockidx_lightness] = lightness;
        evt.data = &ut_mockmem_lightness[ut_mockidx_lightness++];
        break;
    case GMON_EVENT_AIR_TEMP_UPDATED:
        ut_mockmem_aircond[ut_mockidx_aircond] = (gmonAirCond_t){air_temp, air_humid};
        evt.data = &ut_mockmem_aircond[ut_mockidx_aircond++];
        break;
    default:
        break;
    }
    return evt;
}

TEST_SETUP(UpdateLastRecord) {
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    ut_mockidx_soilmoist = 0;
    ut_mockidx_lightness = 0;
    ut_mockidx_aircond = 0;
    staAppMsgInit(&test_gmon);
}

TEST_TEAR_DOWN(UpdateLastRecord) { staAppMsgDeinit(&test_gmon); }

TEST(UpdateLastRecord, AddFirstEventRef) {
    // Create a soil moisture event with specific data
    gmonEvent_t evt = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 500, 0.f, 0.f, 0, 100, 1);
    // Call staUpdateLastRecord for soil moisture events, expecting no discard initially
    gmonEvent_t *discarded_event = staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &evt);
    // Verify that no event was discarded since the buffer is initially empty
    TEST_ASSERT_NULL(discarded_event);
    // Get a pointer to the gmonSensorRecord_t for soil moisture from the global gardenMonitor_t
    gmonSensorRecord_t *soil_record = &test_gmon.latest_logs.soilmoist;
    // Verify that the inner_wr_ptr (next insertion point) has advanced to 1
    TEST_ASSERT_EQUAL(1, soil_record->inner_wr_ptr);
    // Verify that the newly added event is stored at index 0 of the events array
    gmonEvent_t *stored_event = soil_record->events[0];
    TEST_ASSERT_NOT_NULL(stored_event);
    // Assert on the properties of the stored event
    TEST_ASSERT_EQUAL(GMON_EVENT_SOIL_MOISTURE_UPDATED, stored_event->event_type);
    TEST_ASSERT_EQUAL(100, stored_event->curr_ticks);
    TEST_ASSERT_EQUAL(1, stored_event->curr_days);
    TEST_ASSERT_NOT_NULL(stored_event->data);
    TEST_ASSERT_EQUAL(500, *((unsigned int *)stored_event->data));
}

TEST(UpdateLastRecord, AccumulateEventRefs) {
    // Add soil-sensor event
    gmonEvent_t  evt1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 600, 0.f, 0.f, 0, 200, 2);
    gmonEvent_t *discarded = staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &evt1);
    TEST_ASSERT_NULL(discarded);
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.soilmoist.inner_wr_ptr);
    // Add air temp/humid
    gmonEvent_t evt2 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.5f, 70.0f, 0, 200, 2);
    discarded = staUpdateLastRecord(&test_gmon.latest_logs.aircond, &evt2);
    TEST_ASSERT_NULL(discarded);
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.aircond.inner_wr_ptr);
    gmonEvent_t *stored_evt2 = test_gmon.latest_logs.aircond.events[0];
    TEST_ASSERT_NOT_NULL(stored_evt2);
    TEST_ASSERT_EQUAL_PTR(&evt2, stored_evt2);
    TEST_ASSERT_EQUAL(GMON_EVENT_AIR_TEMP_UPDATED, stored_evt2->event_type);
    TEST_ASSERT_EQUAL(200, stored_evt2->curr_ticks);
    TEST_ASSERT_EQUAL(2, stored_evt2->curr_days);
    TEST_ASSERT_EQUAL_FLOAT(25.5f, ((gmonAirCond_t *)stored_evt2->data)->temporature);
    TEST_ASSERT_EQUAL_FLOAT(70.0f, ((gmonAirCond_t *)stored_evt2->data)->humidity);
    // Add another soil sensor event
    gmonEvent_t evt3 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 619, 0.f, 0.f, 0, 300, 2);
    discarded = staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &evt3);
    TEST_ASSERT_NULL(discarded);
    TEST_ASSERT_EQUAL(2, test_gmon.latest_logs.soilmoist.inner_wr_ptr);

    gmonEvent_t *stored_evt1 = test_gmon.latest_logs.soilmoist.events[0];
    TEST_ASSERT_EQUAL_PTR(&evt1, stored_evt1);
    TEST_ASSERT_EQUAL(GMON_EVENT_SOIL_MOISTURE_UPDATED, stored_evt1->event_type);
    TEST_ASSERT_EQUAL(200, stored_evt1->curr_ticks);
    TEST_ASSERT_EQUAL(2, stored_evt1->curr_days);
    TEST_ASSERT_EQUAL(600, *((unsigned int *)stored_evt1->data));
    gmonEvent_t *stored_evt3 = test_gmon.latest_logs.soilmoist.events[1];
    TEST_ASSERT_EQUAL_PTR(&evt3, stored_evt3);
    TEST_ASSERT_EQUAL(GMON_EVENT_SOIL_MOISTURE_UPDATED, stored_evt3->event_type);
    TEST_ASSERT_EQUAL(300, stored_evt3->curr_ticks);
    TEST_ASSERT_EQUAL(2, stored_evt3->curr_days);
    TEST_ASSERT_EQUAL(619, *((unsigned int *)stored_evt3->data));
}

TEST(UpdateLastRecord, SensorEvtRefsFull_DiscardOldest) {
    gmonSensorRecord_t *soil_record = &test_gmon.latest_logs.soilmoist;
    unsigned short      idx = 0;
#define NUM_TEST_EVTS (GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP + 2)
    gmonEvent_t *discarded = NULL;
    gmonEvent_t  ut_evts[NUM_TEST_EVTS] = {0};
    for (idx = 0; idx < NUM_TEST_EVTS; idx++) {
        unsigned int soilmoist = 101 + idx;
        unsigned int ticks = 1000 * (idx + 1);
        ut_evts[idx] = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, soilmoist, 0.f, 0.f, 0, ticks, 10);
    }
    // Override num_refs to match mock array capacity for this test
    // This allows testing the circular buffer behavior properly with limited mock memory.
    TEST_ASSERT_EQUAL(GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP, soil_record->num_refs);
    // 1. Add event references until full
    for (idx = 0; idx < soil_record->num_refs; idx++) {
        TEST_ASSERT_NULL(soil_record->events[idx]);
        TEST_ASSERT_EQUAL(idx, soil_record->inner_wr_ptr);
        discarded = staUpdateLastRecord(soil_record, &ut_evts[idx]);
        TEST_ASSERT_NULL(discarded);
        TEST_ASSERT_EQUAL_PTR(&ut_evts[idx], soil_record->events[idx]);
        if (idx < (soil_record->num_refs - 1))
            TEST_ASSERT_NULL(soil_record->events[idx + 1]);
    }
    TEST_ASSERT_EQUAL(0, soil_record->inner_wr_ptr);
    // 2. discard of the oldest - evt1 , and add one new event
    discarded = staUpdateLastRecord(soil_record, &ut_evts[NUM_TEST_EVTS - 2]);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[0], discarded);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[NUM_TEST_EVTS - 2], soil_record->events[0]);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[1], soil_record->events[1]); // Second oldest remains
    TEST_ASSERT_EQUAL_PTR(&ut_evts[2], soil_record->events[2]); // Third oldest remains
    TEST_ASSERT_EQUAL(1, soil_record->inner_wr_ptr);
    // 2. discard of the oldest - evt2 , and add one new event
    discarded = staUpdateLastRecord(soil_record, &ut_evts[NUM_TEST_EVTS - 1]);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[1], discarded);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[NUM_TEST_EVTS - 2], soil_record->events[0]);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[NUM_TEST_EVTS - 1], soil_record->events[1]);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[2], soil_record->events[2]); // Third oldest remains
#undef NUM_TEST_EVTS
}

TEST(UpdateLastRecord, AddNullEventToEmptyRecord) {
    gmonSensorRecord_t *soil_record = &test_gmon.latest_logs.soilmoist;
    // Assume GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP is > 0
    TEST_ASSERT_GREATER_THAN(0, soil_record->num_refs);
    TEST_ASSERT_EQUAL(GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP, soil_record->num_refs);
    gmonEvent_t *discarded = staUpdateLastRecord(soil_record, NULL);
    TEST_ASSERT_NULL(discarded);              // No event should be discarded from an empty record
    TEST_ASSERT_NULL(soil_record->events[0]); // The first slot should now be NULL
    // inner_wr_ptr should remain unchanged
    TEST_ASSERT_EQUAL(0, soil_record->inner_wr_ptr);
}

TEST(UpdateLastRecord, AddNullEventToPartiallyFilledRecord) {
    gmonSensorRecord_t *soil_record = &test_gmon.latest_logs.soilmoist;
    // This test needs at least 2 slots to show partial fill behavior
    TEST_ASSERT_GREATER_THAN(1, soil_record->num_refs);
    // Add one valid event to partially fill the record
    gmonEvent_t evt1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 100, 0.f, 0.f, 0, 1000, 10);
    staUpdateLastRecord(soil_record, &evt1);
    // Verify state after first event
    TEST_ASSERT_EQUAL_PTR(&evt1, soil_record->events[0]);
    TEST_ASSERT_EQUAL(1, soil_record->inner_wr_ptr);
    // Add a NULL event reference
    gmonEvent_t *discarded = staUpdateLastRecord(soil_record, NULL);
    TEST_ASSERT_NULL(discarded); // No event should be discarded as buffer isn't full yet
    TEST_ASSERT_EQUAL_PTR(&evt1, soil_record->events[0]); // The first event should still be present
    TEST_ASSERT_NULL(soil_record->events[1]);             // The newly "inserted" slot should be NULL
    // inner_wr_ptr should still be 1
    TEST_ASSERT_EQUAL(1, soil_record->inner_wr_ptr);
}

TEST(UpdateLastRecord, AddNullEventToFullRecord) {
    gmonSensorRecord_t *soil_record = &test_gmon.latest_logs.soilmoist;
    unsigned short      num_refs = soil_record->num_refs;
    // Ensure buffer has capacity
    TEST_ASSERT_GREATER_THAN(0, num_refs);
    // Fill the record with actual events up to its capacity
    gmonEvent_t ut_evts[num_refs]; // Use larger mock array for events
    for (unsigned short i = 0; i < num_refs; i++) {
        // Ensure we don't exceed the mock memory allocated in TEST_GROUP for event data
        TEST_ASSERT_LESS_OR_EQUAL(sizeof(ut_mockmem_soilmoist) / sizeof(ut_mockmem_soilmoist[0]), i);
        ut_evts[i] = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 100 + i, 0.f, 0.f, 0, 1000 + i, 10);
        staUpdateLastRecord(soil_record, &ut_evts[i]);
    }
    // After filling, inner_wr_ptr should wrap around to 0
    TEST_ASSERT_EQUAL(0, soil_record->inner_wr_ptr);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[0], soil_record->events[0]);
    // Try adding a NULL event reference to the full record
    gmonEvent_t *discarded = staUpdateLastRecord(soil_record, NULL);
    TEST_ASSERT_NULL(discarded);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[0], soil_record->events[0]);
    TEST_ASSERT_EQUAL_PTR(&ut_evts[1], soil_record->events[1]); // The next event remains
    // inner_wr_ptr should still be 0
    TEST_ASSERT_EQUAL(0, soil_record->inner_wr_ptr);
}

TEST_GROUP_RUNNER(gMonAppMsgOutbound) {
    // RUN_TEST_CASE(GenerateMsgOutflight, EmptyRecordsOutput);
    // RUN_TEST_CASE(GenerateMsgOutflight, SinglePopulatedRecord);
    // RUN_TEST_CASE(GenerateMsgOutflight, MultiplePopulatedRecords);
    // RUN_TEST_CASE(GenerateMsgOutflight, RecordsWithExtremeValues);
    RUN_TEST_CASE(UpdateLastRecord, AddFirstEventRef);
    RUN_TEST_CASE(UpdateLastRecord, AccumulateEventRefs);
    RUN_TEST_CASE(UpdateLastRecord, SensorEvtRefsFull_DiscardOldest);
    RUN_TEST_CASE(UpdateLastRecord, AddNullEventToEmptyRecord);
    RUN_TEST_CASE(UpdateLastRecord, AddNullEventToPartiallyFilledRecord);
    RUN_TEST_CASE(UpdateLastRecord, AddNullEventToFullRecord);
}
