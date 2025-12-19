#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

// Test fixtures
static gardenMonitor_t test_gmon; // Global gardenMonitor_t for tests

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

// New test group for staUpdateLastRecord
TEST_GROUP(UpdateLastRecord);

static unsigned int  ut_mockmem_soilmoist[3];
static unsigned int  ut_mockmem_lightness[3];
static gmonAirCond_t ut_mockmem_aircond[3];
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

TEST(UpdateLastRecord, FirstSoilMoistEvent) {
    gmonEvent_t         evt = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 500, 0.f, 0.f, 0, 100, 1);
    gmonSensorRecord_t  discarded_record = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt);
    gmonSensorRecord_t *rec0 = &test_gmon.sensors.latest_records[0];
    TEST_ASSERT_EQUAL(500, rec0->soil_moist);
    TEST_ASSERT_EQUAL(1, rec0->flgs.soil_val_written);
    TEST_ASSERT_EQUAL(100, rec0->curr_ticks);
    TEST_ASSERT_EQUAL(1, rec0->curr_days);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.humidity);
    TEST_ASSERT_EQUAL(0, rec0->flgs.air_val_written);
    TEST_ASSERT_EQUAL(0, discarded_record.soil_moist); // No record discarded yet
    TEST_ASSERT_EQUAL_FLOAT(0.0f, discarded_record.air_cond.temporature);
}

TEST(UpdateLastRecord, AccumulateEventsInFirstRecord) {
    // Add soil moisture
    gmonEvent_t        evt1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 600, 0.f, 0.f, 0, 200, 2);
    gmonSensorRecord_t discarded1 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt1);
    // Add air temp/humid
    gmonEvent_t        evt2 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.5f, 70.0f, 0, 200, 2);
    gmonSensorRecord_t discarded2 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt2);
    // Add lightness
    gmonEvent_t        evt3 = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0.f, 0.f, 750, 200, 2);
    gmonSensorRecord_t discarded3 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt3);

    gmonSensorRecord_t *rec0 = &test_gmon.sensors.latest_records[0];
    TEST_ASSERT_EQUAL(600, rec0->soil_moist);
    TEST_ASSERT_EQUAL(1, rec0->flgs.soil_val_written);
    TEST_ASSERT_EQUAL_FLOAT(25.5f, rec0->air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(70.0f, rec0->air_cond.humidity);
    TEST_ASSERT_EQUAL(1, rec0->flgs.air_val_written);
    TEST_ASSERT_EQUAL(750, rec0->lightness);
    TEST_ASSERT_EQUAL(1, rec0->flgs.light_val_written);
    TEST_ASSERT_EQUAL(200, rec0->curr_ticks);
    TEST_ASSERT_EQUAL(2, rec0->curr_days);
    TEST_ASSERT_EQUAL(0, discarded1.soil_moist);
    TEST_ASSERT_EQUAL(0, discarded2.soil_moist);
    TEST_ASSERT_EQUAL(0, discarded3.soil_moist);
}

TEST(UpdateLastRecord, ShiftRecordsWhenFirstIsFull_OldestEmpty) {
    // Prepare records[0] to be "full" (all flags set)
    gmonSensorRecord_t *rec0 = &test_gmon.sensors.latest_records[0];
    gmonSensorRecord_t  expected_rec0_before_shift = {
         .soil_moist = 100,
         .air_cond = {.temporature = 20.0f, .humidity = 60.0f},
         .lightness = 1000,
         .curr_ticks = 1000,
         .curr_days = 10,
         .flgs = {.soil_val_written = 1, .air_val_written = 1, .light_val_written = 1}
    };
    *rec0 = expected_rec0_before_shift;

    // Partially fill records[1] (this will shift to records[2])
    gmonSensorRecord_t *rec1_pre_shift = &test_gmon.sensors.latest_records[1];
    rec1_pre_shift->soil_moist = 110;
    rec1_pre_shift->curr_ticks = 1010;
    rec1_pre_shift->curr_days = 10;
    rec1_pre_shift->flgs.soil_val_written = 1;

    // records[GMON_APPMSG_NUM_RECORDS - 1] (i.e., records[4]) is empty.
    gmonEvent_t        new_evt = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0.f, 0.f, 2000, 1020, 10);
    gmonSensorRecord_t discarded_record = staUpdateLastRecord(test_gmon.sensors.latest_records, &new_evt);

    // Verify new record[0]
    TEST_ASSERT_EQUAL(0, rec0->soil_moist);                    // Cleared by shift
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.temporature); // Cleared by shift
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.humidity);    // Cleared by shift
    TEST_ASSERT_EQUAL(2000, rec0->lightness);                  // Updated by new event
    TEST_ASSERT_EQUAL(1, rec0->flgs.light_val_written);
    TEST_ASSERT_EQUAL(0, rec0->flgs.air_val_written);
    TEST_ASSERT_EQUAL(1020, rec0->curr_ticks);
    TEST_ASSERT_EQUAL(10, rec0->curr_days);

    // Verify records shifted: Old records[0] moved to records[1]
    gmonSensorRecord_t *rec1_after_shift = &test_gmon.sensors.latest_records[1];
    TEST_ASSERT_EQUAL(expected_rec0_before_shift.soil_moist, rec1_after_shift->soil_moist);
    TEST_ASSERT_EQUAL_FLOAT(
        expected_rec0_before_shift.air_cond.temporature, rec1_after_shift->air_cond.temporature
    );
    TEST_ASSERT_EQUAL_FLOAT(
        expected_rec0_before_shift.air_cond.humidity, rec1_after_shift->air_cond.humidity
    );
    TEST_ASSERT_EQUAL(expected_rec0_before_shift.lightness, rec1_after_shift->lightness);
    TEST_ASSERT_EQUAL(expected_rec0_before_shift.curr_ticks, rec1_after_shift->curr_ticks);
    TEST_ASSERT_EQUAL(expected_rec0_before_shift.curr_days, rec1_after_shift->curr_days);
    TEST_ASSERT_EQUAL(
        expected_rec0_before_shift.flgs.soil_val_written, rec1_after_shift->flgs.soil_val_written
    );
    TEST_ASSERT_EQUAL(
        expected_rec0_before_shift.flgs.air_val_written, rec1_after_shift->flgs.air_val_written
    );
    TEST_ASSERT_EQUAL(
        expected_rec0_before_shift.flgs.light_val_written, rec1_after_shift->flgs.light_val_written
    );

    TEST_ASSERT_EQUAL(
        110, test_gmon.sensors.latest_records[2].soil_moist
    ); // Old records[1] moved to records[2]
    TEST_ASSERT_EQUAL(
        0, test_gmon.sensors.latest_records[3].soil_moist
    ); // Old records[2] (empty) moved to records[3]
    TEST_ASSERT_EQUAL(
        0, test_gmon.sensors.latest_records[4].soil_moist
    ); // Old records[3] (empty) moved to records[4]

    // Verify discarded record (should be empty as records[4] was empty before shift)
    TEST_ASSERT_EQUAL(0, discarded_record.soil_moist);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, discarded_record.air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, discarded_record.air_cond.humidity);
    TEST_ASSERT_EQUAL(0, discarded_record.lightness);
    TEST_ASSERT_EQUAL(0, discarded_record.curr_ticks);
    TEST_ASSERT_EQUAL(0, discarded_record.curr_days);
}

TEST(UpdateLastRecord, ShiftRecordsWhenFirstIsFull_OldestFull) {
    // Fill all records from 0 to GMON_CFG_NUM_SENSOR_RECORDS_KEEP - 1 (i.e., 4) completely
    for (int i = 0; i < GMON_CFG_NUM_SENSOR_RECORDS_KEEP; i++) {
        gmonSensorRecord_t *rec = &test_gmon.sensors.latest_records[i];
        rec->soil_moist = 100 + i;
        rec->air_cond.temporature = 20.0f + i;
        rec->air_cond.humidity = 60.0f + i;
        rec->lightness = 1000 + i;
        rec->curr_ticks = 1000 + i * 10;
        rec->curr_days = 10 + i;
        rec->flgs.soil_val_written = 1;
        rec->flgs.air_val_written = 1;
        rec->flgs.light_val_written = 1;
    }
    gmonSensorRecord_t expected_discarded_record =
        test_gmon.sensors.latest_records[GMON_CFG_NUM_SENSOR_RECORDS_KEEP - 1];
    gmonEvent_t new_evt =
        create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 999, 12.3f, 45.67f, 0, 2000, 20);
    gmonSensorRecord_t discarded_record = staUpdateLastRecord(test_gmon.sensors.latest_records, &new_evt);

    // Verify new record[0]
    gmonSensorRecord_t *rec0 = &test_gmon.sensors.latest_records[0];
    TEST_ASSERT_EQUAL(999, rec0->soil_moist);
    TEST_ASSERT_EQUAL(1, rec0->flgs.soil_val_written);
    TEST_ASSERT_EQUAL(2000, rec0->curr_ticks);
    TEST_ASSERT_EQUAL(20, rec0->curr_days);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.humidity);
    // Verify records shifted correctly
    TEST_ASSERT_EQUAL(
        100, test_gmon.sensors.latest_records[1].soil_moist
    ); // Old records[0] moved to records[1]
    TEST_ASSERT_EQUAL(
        101, test_gmon.sensors.latest_records[2].soil_moist
    ); // Old records[1] moved to records[2]
    TEST_ASSERT_EQUAL(
        102, test_gmon.sensors.latest_records[3].soil_moist
    ); // Old records[2] moved to records[3]
    TEST_ASSERT_EQUAL(
        103, test_gmon.sensors.latest_records[4].soil_moist
    ); // Old records[3] moved to records[4], old records[4] is lost

    // Verify discarded record
    TEST_ASSERT_EQUAL(expected_discarded_record.soil_moist, discarded_record.soil_moist);
    TEST_ASSERT_EQUAL_FLOAT(
        expected_discarded_record.air_cond.temporature, discarded_record.air_cond.temporature
    );
    TEST_ASSERT_EQUAL_FLOAT(expected_discarded_record.air_cond.humidity, discarded_record.air_cond.humidity);
    TEST_ASSERT_EQUAL(expected_discarded_record.lightness, discarded_record.lightness);
    TEST_ASSERT_EQUAL(expected_discarded_record.curr_ticks, discarded_record.curr_ticks);
    TEST_ASSERT_EQUAL(expected_discarded_record.curr_days, discarded_record.curr_days);
    TEST_ASSERT_EQUAL(
        expected_discarded_record.flgs.soil_val_written, discarded_record.flgs.soil_val_written
    );
    TEST_ASSERT_EQUAL(expected_discarded_record.flgs.air_val_written, discarded_record.flgs.air_val_written);
    TEST_ASSERT_EQUAL(
        expected_discarded_record.flgs.light_val_written, discarded_record.flgs.light_val_written
    );
}

TEST(UpdateLastRecord, UnknownEventType) {
    // Start with records[0] empty
    gmonEvent_t        unknown_evt = create_test_event((gmonEventType_t)99, 123, 45.6f, 78.9f, 987, 3000, 30);
    gmonSensorRecord_t discarded_record = staUpdateLastRecord(test_gmon.sensors.latest_records, &unknown_evt);
    gmonSensorRecord_t *rec0 = &test_gmon.sensors.latest_records[0];
    // Only curr_ticks and curr_days should be updated
    TEST_ASSERT_EQUAL(0, rec0->curr_ticks);
    TEST_ASSERT_EQUAL(0, rec0->curr_days);
    // All other fields should remain 0/default values as they were not explicitly handled
    TEST_ASSERT_EQUAL(0, rec0->soil_moist);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0->air_cond.humidity);
    TEST_ASSERT_EQUAL(0, rec0->lightness);
    TEST_ASSERT_EQUAL(0, rec0->flgs.soil_val_written);
    TEST_ASSERT_EQUAL(0, rec0->flgs.air_val_written);
    TEST_ASSERT_EQUAL(0, rec0->flgs.light_val_written);
    TEST_ASSERT_EQUAL(0, discarded_record.soil_moist); // No record discarded
}

TEST(UpdateLastRecord, ShiftRecordsOnSecondSoilMoistEvent) {
    // First event: populate records[0] with soil moisture data
    gmonEvent_t        evt1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 500, 0.f, 0.f, 0, 100, 1);
    gmonSensorRecord_t discarded1 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt1);
    gmonSensorRecord_t rec0_after_evt1 = test_gmon.sensors.latest_records[0]; // To be shifted to rec1

    // Second event (same type): should trigger a shift
    gmonEvent_t        evt2 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 550, 0.f, 0.f, 0, 110, 1);
    gmonSensorRecord_t discarded2 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt2);
    gmonSensorRecord_t rec0_after_evt2 = test_gmon.sensors.latest_records[0];
    gmonSensorRecord_t rec1_after_evt2 = test_gmon.sensors.latest_records[1];

    // Verify current record[0] has data from evt2 (new event)
    TEST_ASSERT_EQUAL(550, rec0_after_evt2.soil_moist);
    TEST_ASSERT_EQUAL(1, rec0_after_evt2.flgs.soil_val_written);
    TEST_ASSERT_EQUAL(110, rec0_after_evt2.curr_ticks);
    TEST_ASSERT_EQUAL(1, rec0_after_evt2.curr_days);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rec0_after_evt2.air_cond.temporature); // Not updated

    // Verify record[1] has data from evt1 (old records[0])
    TEST_ASSERT_EQUAL(rec0_after_evt1.soil_moist, rec1_after_evt2.soil_moist);
    TEST_ASSERT_EQUAL(rec0_after_evt1.flgs.soil_val_written, rec1_after_evt2.flgs.soil_val_written);
    TEST_ASSERT_EQUAL(rec0_after_evt1.curr_ticks, rec1_after_evt2.curr_ticks);
    TEST_ASSERT_EQUAL(rec0_after_evt1.curr_days, rec1_after_evt2.curr_days);
    TEST_ASSERT_EQUAL_FLOAT(rec0_after_evt1.air_cond.temporature, rec1_after_evt2.air_cond.temporature);

    // No records should have been discarded yet (all records after index 1 were empty)
    TEST_ASSERT_EQUAL(0, discarded1.soil_moist);
    TEST_ASSERT_EQUAL(0, discarded2.soil_moist);
}

TEST(UpdateLastRecord, ShiftRecordsOnSecondAirTempEvent) {
    // First event: populate records[0] with air temp data
    gmonEvent_t        evt1 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.0f, 60.0f, 0, 100, 1);
    gmonSensorRecord_t discarded1 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt1);
    gmonSensorRecord_t rec0_after_evt1 = test_gmon.sensors.latest_records[0]; // To be shifted to rec1

    // Second event (same type): should trigger a shift
    gmonEvent_t         evt2 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 26.5f, 62.0f, 0, 110, 1);
    gmonSensorRecord_t  discarded2 = staUpdateLastRecord(test_gmon.sensors.latest_records, &evt2);
    gmonSensorRecord_t *rec0_after_evt2 = &test_gmon.sensors.latest_records[0];
    gmonSensorRecord_t *rec1_after_evt2 = &test_gmon.sensors.latest_records[1];

    // Verify current record[0] has data from evt2 (new event)
    TEST_ASSERT_EQUAL_FLOAT(26.5f, rec0_after_evt2->air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(62.0f, rec0_after_evt2->air_cond.humidity);
    TEST_ASSERT_EQUAL(1, rec0_after_evt2->flgs.air_val_written);
    TEST_ASSERT_EQUAL(110, rec0_after_evt2->curr_ticks);
    TEST_ASSERT_EQUAL(1, rec0_after_evt2->curr_days);
    TEST_ASSERT_EQUAL(0, rec0_after_evt2->soil_moist); // Not updated

    // Verify record[1] has data from evt1 (old records[0])
    TEST_ASSERT_EQUAL_FLOAT(rec0_after_evt1.air_cond.temporature, rec1_after_evt2->air_cond.temporature);
    TEST_ASSERT_EQUAL_FLOAT(rec0_after_evt1.air_cond.humidity, rec1_after_evt2->air_cond.humidity);
    TEST_ASSERT_EQUAL(rec0_after_evt1.flgs.air_val_written, rec1_after_evt2->flgs.air_val_written);
    TEST_ASSERT_EQUAL(rec0_after_evt1.curr_ticks, rec1_after_evt2->curr_ticks);
    TEST_ASSERT_EQUAL(rec0_after_evt1.curr_days, rec1_after_evt2->curr_days);
    TEST_ASSERT_EQUAL(rec0_after_evt1.soil_moist, rec1_after_evt2->soil_moist);

    // No records should have been discarded yet (all records after index 1 were empty)
    TEST_ASSERT_EQUAL(0, discarded1.soil_moist);
    TEST_ASSERT_EQUAL(0, discarded2.soil_moist);
}

TEST_GROUP_RUNNER(gMonAppMsgOutbound) {
    RUN_TEST_CASE(GenerateMsgOutflight, EmptyRecordsOutput);
    RUN_TEST_CASE(GenerateMsgOutflight, SinglePopulatedRecord);
    RUN_TEST_CASE(GenerateMsgOutflight, MultiplePopulatedRecords);
    RUN_TEST_CASE(GenerateMsgOutflight, RecordsWithExtremeValues);
    RUN_TEST_CASE(UpdateLastRecord, FirstSoilMoistEvent);
    RUN_TEST_CASE(UpdateLastRecord, AccumulateEventsInFirstRecord);
    RUN_TEST_CASE(UpdateLastRecord, ShiftRecordsWhenFirstIsFull_OldestEmpty);
    RUN_TEST_CASE(UpdateLastRecord, ShiftRecordsWhenFirstIsFull_OldestFull);
    RUN_TEST_CASE(UpdateLastRecord, ShiftRecordsOnSecondSoilMoistEvent);
    RUN_TEST_CASE(UpdateLastRecord, ShiftRecordsOnSecondAirTempEvent);
    RUN_TEST_CASE(UpdateLastRecord, UnknownEventType);
}
