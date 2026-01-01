#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

static unsigned int  ut_mockmem_soilmoist[21];
static unsigned int  ut_mockmem_lightness[18];
static gmonAirCond_t ut_mockmem_aircond[15];

static int ut_mockidx_soilmoist;
static int ut_mockidx_lightness;
static int ut_mockidx_aircond;
// Similar defines would be needed for AIR and LIGHT if not already in station_include.h
static gardenMonitor_t test_gmon; // Global gardenMonitor_t for tests

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

// New test group for staGetAppMsgOutflight
TEST_GROUP(GenerateMsgOutflight);

TEST_SETUP(GenerateMsgOutflight) {
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    staAppMsgInit(&test_gmon);
}

TEST_TEAR_DOWN(GenerateMsgOutflight) { staAppMsgDeinit(&test_gmon); }

TEST(GenerateMsgOutflight, EmptyLogEvt) {
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Initially, all records should be zeroed out by staAppMsgInit and staAppMsgOutResetAllRecords
    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t   *out_msg = of_res.msg;
    const char  *expected_json = "{\"soilmoist\":{\"ticks\":0,\"days\":0,\"qty\":0,\"corruption\":[],"
                                 "\"values\":[]},"
                                 "\"airtemp\":{\"ticks\":0,\"days\":0,\"qty\":0,\"corruption\":[],"
                                 "\"values\":{\"temp\":[],\"humid\":[]}},"
                                 "\"light\":{\"ticks\":0,\"days\":0,\"qty\":0,\"corruption\":[],"
                                 "\"values\":[]}}";
    unsigned int expected_json_sz = sizeof(expected_json) - 1;
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->len);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);
    // Verify records are reset after retrieval
    for (int i = 0; i < test_gmon.latest_logs.soilmoist.num_refs; i++)
        TEST_ASSERT_NULL(test_gmon.latest_logs.soilmoist.events[i]);
    TEST_ASSERT_EQUAL(0, test_gmon.latest_logs.soilmoist.inner_wr_ptr);
    for (int i = 0; i < test_gmon.latest_logs.aircond.num_refs; i++)
        TEST_ASSERT_NULL(test_gmon.latest_logs.aircond.events[i]);
    TEST_ASSERT_EQUAL(0, test_gmon.latest_logs.aircond.inner_wr_ptr);
    for (int i = 0; i < test_gmon.latest_logs.light.num_refs; i++)
        TEST_ASSERT_NULL(test_gmon.latest_logs.light.events[i]);
    TEST_ASSERT_EQUAL(0, test_gmon.latest_logs.light.inner_wr_ptr);
}

TEST(GenerateMsgOutflight, SingleLogEvtPerSensor) {
    gmonEvent_t evt_s = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 800, 0, 0, 0, 1234567, 10);
    evt_s.num_active_sensors = 1;
    gmonEvent_t evt_a = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.5f, 75.25f, 0, 1234567, 10);
    evt_a.num_active_sensors = 1;
    gmonEvent_t evt_l = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0, 0, 500, 1234567, 10);
    evt_l.num_active_sensors = 1;
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;
    test_gmon.sensors.light.num_items = 1;
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &evt_s);
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &evt_a);
    staUpdateLastRecord(&test_gmon.latest_logs.light, &evt_l);

    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t  *out_msg = of_res.msg;
    const char *expected_json =
        "{\"soilmoist\":{\"ticks\":1234567,\"days\":10,\"qty\":1,\"corruption\":[0],\"values\":[[800]]},"
        "\"airtemp\":{\"ticks\":1234567,\"days\":10,\"qty\":1,\"corruption\":[0],\"values\":{\"temp\":[[25."
        "5]],\"humid\":[[75.2]]}},"
        "\"light\":{\"ticks\":1234567,\"days\":10,\"qty\":1,\"corruption\":[0],\"values\":[[500]]}}";
    unsigned int expected_json_sz = sizeof(expected_json) - 1;
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->len);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);

    // Verify records are reset after retrieval
    TEST_ASSERT_EQUAL_PTR(&evt_s, test_gmon.latest_logs.soilmoist.events[0]);
    TEST_ASSERT_EQUAL_PTR(&evt_a, test_gmon.latest_logs.aircond.events[0]);
    for (int i = 1; i < test_gmon.latest_logs.soilmoist.num_refs; i++)
        TEST_ASSERT_NULL(test_gmon.latest_logs.soilmoist.events[i]);
    for (int i = 1; i < test_gmon.latest_logs.aircond.num_refs; i++)
        TEST_ASSERT_NULL(test_gmon.latest_logs.aircond.events[i]);
    for (int i = 1; i < test_gmon.latest_logs.light.num_refs; i++)
        TEST_ASSERT_NULL(test_gmon.latest_logs.light.events[i]);
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.soilmoist.inner_wr_ptr);
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.aircond.inner_wr_ptr);
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.light.inner_wr_ptr);
}

TEST(GenerateMsgOutflight, MultiLogEvtPerSensor) {
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    ut_mockidx_lightness = 0;
    test_gmon.sensors.soil_moist.super.num_items = 2;
    test_gmon.sensors.air_temp.num_items = 2;
    test_gmon.sensors.light.num_items = 3;
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

    gmonEvent_t s1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 3310, 0, 0, 0, 1000, 1);
    ut_mockmem_soilmoist[ut_mockidx_soilmoist++] = 1020;
    s1.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    s1.flgs.corruption = 0x01;
    gmonEvent_t s2 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 1011, 0, 0, 0, 2000, 1);
    ut_mockmem_soilmoist[ut_mockidx_soilmoist++] = 1021;
    s2.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    gmonEvent_t s3 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 1012, 0, 0, 0, 2600, 1);
    ut_mockmem_soilmoist[ut_mockidx_soilmoist++] = 1023;
    s3.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s1);
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s2);
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s3);

    gmonEvent_t a1 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.5f, 70.5f, 0, 3000, 1);
    ut_mockmem_aircond[ut_mockidx_aircond++] = (gmonAirCond_t){-26.5f, 71.5f};
    a1.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    a1.flgs.corruption = 0x02;
    gmonEvent_t a2 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 27.f, 72.5f, 0, 4000, 1);
    ut_mockmem_aircond[ut_mockidx_aircond++] = (gmonAirCond_t){27.5f, 72.f};
    a2.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a1);
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a2);

    gmonEvent_t l1 = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0, 0, 500, 5000, 1);
    ut_mockmem_lightness[ut_mockidx_lightness++] = 600;
    ut_mockmem_lightness[ut_mockidx_lightness++] = 616;
    l1.num_active_sensors = test_gmon.sensors.light.num_items;
    l1.flgs.corruption = 0x03;
    gmonEvent_t l2 = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0, 0, 510, 6000, 1);
    ut_mockmem_lightness[ut_mockidx_lightness++] = 610;
    ut_mockmem_lightness[ut_mockidx_lightness++] = 637;
    l2.num_active_sensors = test_gmon.sensors.light.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.light, &l1);
    staUpdateLastRecord(&test_gmon.latest_logs.light, &l2);

    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t  *out_msg = of_res.msg;
    const char *expected_json =
        "{\"soilmoist\":{\"ticks\":2600,\"days\":1,\"qty\":3,\"corruption\":[1,0,0],\"values\":[[3310,1020],["
        "1011,1021],[1012,1023]]},"
        "\"airtemp\":{\"ticks\":4000,\"days\":1,\"qty\":2,\"corruption\":[2,0],\"values\":{\"temp\":[[25.5,"
        "-26.5],[27,27.5]],\"humid\":[[70.5,71.5],[72.5,72]]}},"
        "\"light\":{\"ticks\":6000,\"days\":1,\"qty\":2,\"corruption\":[3,0],\"values\":[[500,600,616]"
        ",[510,610,637]]}}";
    unsigned int expected_json_sz = sizeof(expected_json) - 1;
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->len);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);

    TEST_ASSERT_EQUAL_HEX8(0x01, test_gmon.latest_logs.soilmoist.events[0]->flgs.corruption);
    TEST_ASSERT_EQUAL_HEX8(0x02, test_gmon.latest_logs.aircond.events[0]->flgs.corruption);
    TEST_ASSERT_EQUAL_HEX8(0x03, test_gmon.latest_logs.light.events[0]->flgs.corruption);
}

TEST(GenerateMsgOutflight, FullLogEvt) {
    // insert event references to each `gmonSensorRecord_t` instance until it is full,
    // in such case the internal write pointer wraps around back to index zero,
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    ut_mockidx_lightness = 0;
    // Set sensor configurations (consistent with the other tests and expected JSON)
    test_gmon.sensors.soil_moist.super.num_items = 2;
    test_gmon.sensors.air_temp.num_items = 2;
    test_gmon.sensors.light.num_items = 2;
    // Reallocate buffer with new sensor counts (necessary after changing num_items)
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

#define UT_NUM_SOIL_EVTS (GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP + 1)
    // --- Fill Soil Moisture Record (num_items=2, num_refs=GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP) ---
    gmonEvent_t soil_evts[UT_NUM_SOIL_EVTS] = {0};
    for (unsigned char i = 0; i < UT_NUM_SOIL_EVTS; ++i) {
        soil_evts[i] =
            create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 100 + i * 2, 0.f, 0.f, 0, 1000 + i * 1000, 1);
        // Manually add the second sensor value as per create_test_event's behavior
        ut_mockmem_soilmoist[ut_mockidx_soilmoist++] = 101 + i * 2;
        soil_evts[i].num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
        soil_evts[i].flgs.corruption = i & 0x3; // e.g., 0, 1, 2, 3, 0 ...
        staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &soil_evts[i]);
    }
    // After filling, inner_wr_ptr should wrap around to 1
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.soilmoist.inner_wr_ptr);

    // --- Fill Air Condition Record (num_items=2, num_refs=GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP) ---
    gmonEvent_t air_evts[GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP];
    for (unsigned char i = 0; i < GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP; ++i) {
        float temp1 = 20.0f + i * 2.5f;
        float humid1 = 70.0f + i * 2.5f;
        air_evts[i] = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, temp1, humid1, 0, 1050 + i * 1000, 1);
        // Manually add the second sensor value
        ut_mockmem_aircond[ut_mockidx_aircond++] = (gmonAirCond_t){temp1 + 1.0f, humid1 + 1.0f};
        air_evts[i].num_active_sensors = test_gmon.sensors.air_temp.num_items;
        air_evts[i].flgs.corruption = (i & 0x1) << 1; // e.g., 0, 2, 0, 2, ...
        staUpdateLastRecord(&test_gmon.latest_logs.aircond, &air_evts[i]);
    }
    // After filling, inner_wr_ptr should wrap around to 0
    TEST_ASSERT_EQUAL(0, test_gmon.latest_logs.aircond.inner_wr_ptr);

#define UT_NUM_LIGHT_EVTS (GMON_CFG_NUM_LIGHT_SENSOR_RECORDS_KEEP + 2)
    // --- Fill Lightness Record (num_items=2, num_refs=GMON_CFG_NUM_LIGHT_SENSOR_RECORDS_KEEP) ---
    gmonEvent_t light_evts[UT_NUM_LIGHT_EVTS];
    for (unsigned char i = 0; i < UT_NUM_LIGHT_EVTS; ++i) {
        light_evts[i] =
            create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0.f, 0.f, 500 + i * 2, 1100 + i * 1000, 1);
        // Manually add the second sensor value
        ut_mockmem_lightness[ut_mockidx_lightness++] = 501 + i * 2;
        light_evts[i].num_active_sensors = test_gmon.sensors.light.num_items;
        light_evts[i].flgs.corruption = (i % 3);
        staUpdateLastRecord(&test_gmon.latest_logs.light, &light_evts[i]);
    }
    // After filling, inner_wr_ptr should wrap around to 0
    TEST_ASSERT_EQUAL(2, test_gmon.latest_logs.light.inner_wr_ptr);

    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t *out_msg = of_res.msg;
    // Construct the expected JSON string based on the filled circular buffers.
    // Records are ordered from events[0] to events[num_refs-1] when inner_wr_ptr is 0.
    const char *expected_json =
        "{\"soilmoist\":{\"ticks\":7000,\"days\":1,\"qty\":2,\"corruption\":[1,2,3,0,1,2],"
        "\"values\":[[102,103],[104,105],[106,107],[108,109],[110,111],[112,113]]},"
        "\"airtemp\":{\"ticks\":5050,\"days\":1,\"qty\":2,\"corruption\":[0,2,0,2,0],"
        "\"values\":{\"temp\":[[20,21],[22.5,23.5],[25,26],[27.5,28.5],[30,31]],"
        "\"humid\":[[70,71],[72.5,73.5],[75,76],[77.5,78.5],[80,81]]}},"
        "\"light\":{\"ticks\":6100,\"days\":1,\"qty\":2,\"corruption\":[2,0,1,2],"
        "\"values\":[[504,505],[506,507],[508,509],[510,511]]}}";

    unsigned int expected_json_sz = strlen(expected_json);
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->len);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);
    // Verify records state after retrieval: they should not be reset.
    // The events are still referenced in the circular buffers.
    // Soil moisture record check
    TEST_ASSERT_EQUAL_PTR(&soil_evts[UT_NUM_SOIL_EVTS - 1], test_gmon.latest_logs.soilmoist.events[0]);
    TEST_ASSERT_EQUAL_PTR(&soil_evts[1], test_gmon.latest_logs.soilmoist.events[1]);
    TEST_ASSERT_EQUAL_PTR(&soil_evts[2], test_gmon.latest_logs.soilmoist.events[2]);
    TEST_ASSERT_EQUAL(1, test_gmon.latest_logs.soilmoist.inner_wr_ptr);
    // Air condition record check
    TEST_ASSERT_EQUAL_PTR(&air_evts[0], test_gmon.latest_logs.aircond.events[0]);
    TEST_ASSERT_EQUAL_PTR(&air_evts[1], test_gmon.latest_logs.aircond.events[1]);
    TEST_ASSERT_EQUAL_PTR(&air_evts[2], test_gmon.latest_logs.aircond.events[2]);
    TEST_ASSERT_EQUAL(0, test_gmon.latest_logs.aircond.inner_wr_ptr);
    // Light record check
    TEST_ASSERT_EQUAL_PTR(&light_evts[UT_NUM_LIGHT_EVTS - 2], test_gmon.latest_logs.light.events[0]);
    TEST_ASSERT_EQUAL_PTR(&light_evts[UT_NUM_LIGHT_EVTS - 1], test_gmon.latest_logs.light.events[1]);
    TEST_ASSERT_EQUAL_PTR(&light_evts[2], test_gmon.latest_logs.light.events[2]);
    TEST_ASSERT_EQUAL(2, test_gmon.latest_logs.light.inner_wr_ptr);
#undef UT_NUM_SOIL_EVTS
#undef UT_NUM_LIGHT_EVTS
}

TEST(GenerateMsgOutflight, LogEvtDataMissing) {
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    ut_mockidx_lightness = 0;
    // Set sensor configurations (num_items = 1 for simplicity in this test)
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;
    test_gmon.sensors.light.num_items = 1;
    // Reallocate buffer with new sensor counts
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

    // --- Soil Moisture events ---
    // 1. Valid soil moisture event
    gmonEvent_t s1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 800, 0.f, 0.f, 0, 1000, 1);
    s1.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    s1.flgs.corruption = 0x00;
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s1);
    // 2. Soil moisture event with NULL data
    gmonEvent_t s2_null_data = {
        .event_type = GMON_EVENT_SOIL_MOISTURE_UPDATED,
        .num_active_sensors = test_gmon.sensors.soil_moist.super.num_items,
        .flgs.corruption = 0x01,
        .curr_ticks = 2000,
        .curr_days = 1,
        .data = NULL // Data field is explicitly NULL
    };
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s2_null_data);

    // --- Air Temperature events ---
    // 1. Valid air temperature event
    gmonEvent_t a1 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.5f, 70.0f, 0, 3000, 1);
    a1.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    a1.flgs.corruption = 0x00;
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a1);
    // 2. Air temperature event with NULL data
    gmonEvent_t a2_null_data = {
        .event_type = GMON_EVENT_AIR_TEMP_UPDATED,
        .num_active_sensors = test_gmon.sensors.air_temp.num_items,
        .flgs.corruption = 0x02,
        .curr_ticks = 4000,
        .curr_days = 1,
        .data = NULL // Data field is explicitly NULL
    };
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a2_null_data);

    // --- Lightness events ---
    // 1. Valid lightness event
    gmonEvent_t l1 = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0.f, 0.f, 500, 5000, 1);
    l1.num_active_sensors = test_gmon.sensors.light.num_items;
    l1.flgs.corruption = 0x00;
    staUpdateLastRecord(&test_gmon.latest_logs.light, &l1);
    // 2. Lightness event with NULL data
    gmonEvent_t l2_null_data = {
        .event_type = GMON_EVENT_LIGHTNESS_UPDATED,
        .num_active_sensors = test_gmon.sensors.light.num_items,
        .flgs.corruption = 0x01,
        .curr_ticks = 6000,
        .curr_days = 1,
        .data = NULL // Data field is explicitly NULL
    };
    staUpdateLastRecord(&test_gmon.latest_logs.light, &l2_null_data);
    // Verify internal state: event reference itself is not NULL, but its data field is NULL.
    TEST_ASSERT_EQUAL_PTR(&s2_null_data, test_gmon.latest_logs.soilmoist.events[1]);
    TEST_ASSERT_NULL(test_gmon.latest_logs.soilmoist.events[1]->data);
    TEST_ASSERT_EQUAL_PTR(&a2_null_data, test_gmon.latest_logs.aircond.events[1]);
    TEST_ASSERT_NULL(test_gmon.latest_logs.aircond.events[1]->data);
    TEST_ASSERT_EQUAL_PTR(&l2_null_data, test_gmon.latest_logs.light.events[1]);
    TEST_ASSERT_NULL(test_gmon.latest_logs.light.events[1]->data);
    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t *out_msg = of_res.msg;
    // Expected JSON string:
    // - ticks and days are from the LATEST event, which is the NULL data event
    // - corruption array includes all event corruptions in insertion order
    // - values for events with NULL data are serialized as "null"
    const char *expected_json =
        "{\"soilmoist\":{\"ticks\":2000,\"days\":1,\"qty\":1,\"corruption\":[0,1],\"values\":[[800],null]},"
        "\"airtemp\":{\"ticks\":4000,\"days\":1,\"qty\":1,\"corruption\":[0,2],\"values\":{"
        "\"temp\":[[25.5],null],\"humid\":[[70],null]}},"
        "\"light\":{\"ticks\":6000,\"days\":1,\"qty\":1,\"corruption\":[0,1],\"values\":[[500],null]}}";
    unsigned int expected_json_sz = strlen(expected_json);
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->len);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);
}

TEST(GenerateMsgOutflight, LogEvtInterleavedNullRef) {
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    // Set sensor configurations to 1 item per sensor for simplicity in this test
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;

    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    gmonSensorRecord_t *soil_record = &test_gmon.latest_logs.soilmoist;
    gmonSensorRecord_t *air_record = &test_gmon.latest_logs.aircond;
    // --- Populate Soil Moisture Record: [s1, NULL, s2, NULL, s3, NULL] ---
    // (GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP = 6)
    gmonEvent_t soil_evts[GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP] = {0};
    for (unsigned int idx = 0; idx < GMON_CFG_NUM_SOIL_SENSOR_RECORDS_KEEP; idx++) {
        soil_evts[idx] = create_test_event(
            GMON_EVENT_SOIL_MOISTURE_UPDATED, 100 * (idx + 1), 0.f, 0.f, 0, 1000 * (idx + 1), 1
        );
        soil_evts[idx].num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
        soil_evts[idx].flgs.corruption = idx + 1;
        staUpdateLastRecord(soil_record, &soil_evts[idx]); // increment inner write pointer
    }
    // assume the event references are set to NULL due to some unexpected issues
    soil_record->events[1] = NULL;
    soil_record->events[3] = NULL;
    TEST_ASSERT_EQUAL(0, soil_record->inner_wr_ptr);

    // --- Populate Air Condition Record: [a1, a2, NULL, a3, NULL] ---
    // (GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP = 5)
    gmonEvent_t air_evts[GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP] = {0};
    for (unsigned int idx = 0; idx < GMON_CFG_NUM_AIR_SENSOR_RECORDS_KEEP; idx++) {
        air_evts[idx] = create_test_event(
            GMON_EVENT_AIR_TEMP_UPDATED, 0, 20.0f + idx, 70.0f + idx, 0, 1000 + 100 * (idx + 1), 1
        );
        air_evts[idx].num_active_sensors = test_gmon.sensors.air_temp.num_items;
        air_evts[idx].flgs.corruption = 10 + idx;
        staUpdateLastRecord(air_record, &air_evts[idx]); // inner_wr_ptr = 1, events[0] = &a1
    }
    // assume the event references are set to NULL due to some unexpected issues
    air_record->events[0] = NULL;
    air_record->events[3] = NULL;
    TEST_ASSERT_EQUAL(0, air_record->inner_wr_ptr);

    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t *out_msg = of_res.msg;
    // Expected JSON string:
    // - Ticks/Days are from the LATEST non-NULL event.
    // - Corruption/Values arrays ONLY include data from non-NULL event pointers, in their logical order.
    const char *expected_json =
        "{\"soilmoist\":{\"ticks\":6000,\"days\":1,\"qty\":1,\"corruption\":[1,3,5,6],"
        "\"values\":[[100],[300],[500],[600]]},"
        "\"airtemp\":{\"ticks\":1500,\"days\":1,\"qty\":1,\"corruption\":[11,12,14],"
        "\"values\":{\"temp\":[[21],[22],[24]],\"humid\":[[71],[72],[74]]}},"
        "\"light\":{\"ticks\":0,\"days\":0,\"qty\":0,\"corruption\":[],\"values\":[]}}";

    unsigned int expected_json_sz = strlen(expected_json);
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->nbytes_written);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);
    // Verify records are NOT reset after retrieval, staGetAppMsgOutflight only serializes.
}

TEST(GenerateMsgOutflight, LogEvtExtremeValues) {
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    ut_mockidx_lightness = 0;
    test_gmon.sensors.soil_moist.super.num_items = 2;
    test_gmon.sensors.air_temp.num_items = 2;
    test_gmon.sensors.light.num_items = 2;
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

    // --- Soil Moisture with extreme values (max uint for 4 digits, min 0) ---
    gmonEvent_t s1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 9999, 0.f, 0.f, 0, 1, 1);
    ut_mockmem_soilmoist[ut_mockidx_soilmoist++] = 10;
    s1.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    s1.flgs.corruption = 255; // Max corruption value
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s1);
    gmonEvent_t s2 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 0, 0.f, 0.f, 0, 2, 1);
    ut_mockmem_soilmoist[ut_mockidx_soilmoist++] = 123;
    s2.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    s2.flgs.corruption = 0; // Min corruption value
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s2);

    // --- Air Temperature/Humidity with extreme float values (negative, positive, zero) ---
    gmonEvent_t a1 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, -102.5f, 98.5f, 0, 3, 1);
    ut_mockmem_aircond[ut_mockidx_aircond++] = (gmonAirCond_t){1.0f, -1.0f};
    a1.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    a1.flgs.corruption = 127; // Mid-range corruption
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a1);
    gmonEvent_t a2 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 9999.5f, -5.0f, 0, 4, 1);
    ut_mockmem_aircond[ut_mockidx_aircond++] = (gmonAirCond_t){-999.5f, 0.0f};
    a2.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    a2.flgs.corruption = 1;
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a2);

    // --- Lightness with extreme values (max uint for 4 digits, min 0) ---
    gmonEvent_t l1 = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0.f, 0.f, 9999, 5, 1);
    ut_mockmem_lightness[ut_mockidx_lightness++] = 10;
    l1.num_active_sensors = test_gmon.sensors.light.num_items;
    l1.flgs.corruption = 64;
    staUpdateLastRecord(&test_gmon.latest_logs.light, &l1);
    gmonEvent_t l2 = create_test_event(GMON_EVENT_LIGHTNESS_UPDATED, 0, 0.f, 0.f, 0, 6, 1);
    ut_mockmem_lightness[ut_mockidx_lightness++] = 9998;
    l2.num_active_sensors = test_gmon.sensors.light.num_items;
    l2.flgs.corruption = 2;
    staUpdateLastRecord(&test_gmon.latest_logs.light, &l2);

    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    gmonStr_t  *out_msg = of_res.msg;
    const char *expected_json =
        "{\"soilmoist\":{\"ticks\":2,\"days\":1,\"qty\":2,\"corruption\":[255,0],"
        "\"values\":[[9999,10],[0,123]]},"
        "\"airtemp\":{\"ticks\":4,\"days\":1,\"qty\":2,\"corruption\":[127,1],"
        "\"values\":{\"temp\":[[-102.5,1],[9999.5,-999.5]],\"humid\":[[98.5,-1],[-5,0]]}},"
        "\"light\":{\"ticks\":6,\"days\":1,\"qty\":2,\"corruption\":[64,2],"
        "\"values\":[[9999,10],[0,9998]]}}";

    unsigned int expected_json_sz = strlen(expected_json);
    TEST_ASSERT_GREATER_OR_EQUAL(expected_json_sz, out_msg->len);
    TEST_ASSERT_EQUAL_STRING_LEN(expected_json, (const char *)out_msg->data, expected_json_sz);

    // Verify records are NOT reset after retrieval, staGetAppMsgOutflight only serializes.
    TEST_ASSERT_EQUAL_PTR(&s1, test_gmon.latest_logs.soilmoist.events[0]);
    TEST_ASSERT_EQUAL_PTR(&s2, test_gmon.latest_logs.soilmoist.events[1]);
    TEST_ASSERT_EQUAL(2, test_gmon.latest_logs.soilmoist.inner_wr_ptr);
    TEST_ASSERT_EQUAL_PTR(&a1, test_gmon.latest_logs.aircond.events[0]);
    TEST_ASSERT_EQUAL_PTR(&a2, test_gmon.latest_logs.aircond.events[1]);
    TEST_ASSERT_EQUAL(2, test_gmon.latest_logs.aircond.inner_wr_ptr);
    TEST_ASSERT_EQUAL_PTR(&l1, test_gmon.latest_logs.light.events[0]);
    TEST_ASSERT_EQUAL_PTR(&l2, test_gmon.latest_logs.light.events[1]);
    TEST_ASSERT_EQUAL(2, test_gmon.latest_logs.light.inner_wr_ptr);
}

TEST(GenerateMsgOutflight, InsufficientMemory) {
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    ut_mockidx_lightness = 0;
    // Set sensor configurations to ensure some data will be generated, even if small
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;
    // Allocate the buffer normally first
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Add a single event for each sensor type. These events will attempt to be serialized.
    // The specific values don't matter as the buffer will be too small to write them fully.
    gmonEvent_t s1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 165, 0.f, 0.f, 0, 12345, 91);
    s1.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s1);
    gmonEvent_t a1 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 25.5f, 70.5f, 0, 23456, 192);
    a1.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a1);
#define UT_EXPECTED_JSON \
    "{\"soilmoist\":{\"ticks\":12345,\"days\":91,\"qty\":1,\"corruption\":[0]," \
    "\"values\":[[165]]}," \
    "\"airtemp\":{\"ticks\":23456,\"days\":192,\"qty\":1,\"corruption\":[0]," \
    "\"values\":{\"temp\":[[25.5]],\"humid\":[[70.5]]}}," \
    "\"light\":{\"ticks\":0,\"days\":0,\"qty\":0,\"corruption\":[],\"values\":[]}" \
    "}"
    // ---------- subcase 1 ----------
    // Manually reduce the effective buffer size of outflight message to simulate
    // insufficient memory. This will cause serialization functions to return GMON_RESP_ERRMEM.
    // A size of 5 bytes ensures that the serialization fails very early,
    // for example, after writing just the opening curly brace '{'.
    unsigned short insufficient_buffer_size = 5;
    test_gmon.rawmsg.outflight.len = insufficient_buffer_size;
    // Attempt to generate the outflight message. This should fail due to
    // the artificially reduced buffer length.
    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    // Assert that the status indicates a memory error
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, of_res.status);
    TEST_ASSERT_NOT_NULL(of_res.msg->data);
    // When insufficient_buffer_size takes effect:
    // - The opening bracket `{` (1 byte) and double quotes `"` (1 byte) are written
    //   successfully. `buf_ptr` advances, remaining_len = 3.
    // - The next string `soilmoist` (9 bytes) is too large for remaining_len (3 bytes).
    //   `serialize_append_str` returns GMON_RESP_ERRMEM without writing more bytes.
    TEST_ASSERT_EQUAL(2, of_res.msg->nbytes_written);
    TEST_ASSERT_EQUAL_STRING_LEN(UT_EXPECTED_JSON, (const char *)of_res.msg->data, 2);
    // ---------- subcase 2 ----------
    // expect to report memory errors when serializing different items
    // e.g. integer, floating-point, string tokens ... etc
    unsigned short insufficient_sizes[5] = {25, 34, 76, 163, 167};
    unsigned short expect_nb_written[5] = {22, 28, 74, 161, 166};
    for (int idx = 0; idx < 5; idx++) {
        test_gmon.rawmsg.outflight.len = insufficient_sizes[idx];
        of_res = staGetAppMsgOutflight(&test_gmon);
        TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, of_res.status);
        TEST_ASSERT_EQUAL(expect_nb_written[idx], of_res.msg->nbytes_written);
        TEST_ASSERT_EQUAL_STRING_LEN(
            UT_EXPECTED_JSON, (const char *)of_res.msg->data, expect_nb_written[idx]
        );
    }
#undef UT_EXPECTED_JSON
}

TEST(GenerateMsgOutflight, NumDigitExceedLimit) {
    ut_mockidx_soilmoist = 0;
    ut_mockidx_aircond = 0;
    ut_mockidx_lightness = 0;
    // Set sensor configurations to ensure some data will be generated, even if small
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;
    // Allocate the buffer normally first
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Add a single event for each sensor type. These events will attempt to be serialized.
    // The specific values don't matter as the buffer will be too small to write them fully.
    gmonEvent_t s1 = create_test_event(GMON_EVENT_SOIL_MOISTURE_UPDATED, 10000, 0.f, 0.f, 0, 3456, 9);
    s1.num_active_sensors = test_gmon.sensors.soil_moist.super.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.soilmoist, &s1);
    gmonEvent_t a1 = create_test_event(GMON_EVENT_AIR_TEMP_UPDATED, 0, 10000.5f, 70.5f, 0, 4567, 9);
    a1.num_active_sensors = test_gmon.sensors.air_temp.num_items;
    staUpdateLastRecord(&test_gmon.latest_logs.aircond, &a1);
#define UT_EXPECTED_JSON \
    "{\"soilmoist\":{\"ticks\":3456,\"days\":9,\"qty\":1,\"corruption\":[0]," \
    "\"values\":[[9998]]}," \
    "\"airtemp\":{\"ticks\":4567,\"days\":9,\"qty\":1,\"corruption\":[0]," \
    "\"values\":{\"temp\":[[9999.5]],\"humid\":[[70.5]]}}," \
    "\"light\":{\"ticks\":0,\"days\":0,\"qty\":0,\"corruption\":[],\"values\":[]}" \
    "}"
    unsigned short expect_data_sz = sizeof(UT_EXPECTED_JSON) - 1, actual_data_sz = 0;

    gmonAppMsgOutflightResult_t of_res = staGetAppMsgOutflight(&test_gmon);
    actual_data_sz = of_res.msg->nbytes_written;
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_ENCODE, of_res.status);
    TEST_ASSERT_LESS_THAN(expect_data_sz, actual_data_sz);
    TEST_ASSERT_EQUAL_STRING_LEN(UT_EXPECTED_JSON, (const char *)of_res.msg->data, actual_data_sz);
    ((unsigned int *)s1.data)[0] = 9998;
    of_res = staGetAppMsgOutflight(&test_gmon);
    actual_data_sz = of_res.msg->nbytes_written;
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_ENCODE, of_res.status);
    TEST_ASSERT_LESS_THAN(expect_data_sz, actual_data_sz);
    TEST_ASSERT_EQUAL_STRING_LEN(UT_EXPECTED_JSON, (const char *)of_res.msg->data, actual_data_sz);
    ((gmonAirCond_t *)a1.data)[0].temporature = 9999.5;
    of_res = staGetAppMsgOutflight(&test_gmon);
    actual_data_sz = of_res.msg->nbytes_written;
    TEST_ASSERT_EQUAL(GMON_RESP_OK, of_res.status);
    TEST_ASSERT_EQUAL(expect_data_sz, actual_data_sz);
    TEST_ASSERT_EQUAL_STRING_LEN(UT_EXPECTED_JSON, (const char *)of_res.msg->data, actual_data_sz);
#undef UT_EXPECTED_JSON
}

// New test group for staUpdateLastRecord
TEST_GROUP(UpdateLastRecord);

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

// New test group for staAppMsgReallocBuffer
TEST_GROUP(ReallocBuffer);

TEST_SETUP(ReallocBuffer) {
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    staAppMsgInit(&test_gmon);
}

TEST_TEAR_DOWN(ReallocBuffer) { staAppMsgDeinit(&test_gmon); }

TEST(ReallocBuffer, SameSize_ReuseBuffer) {
    gMonStatus status = staAppMsgReallocBuffer(NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    // 1. Initial allocation with a specific configuration
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;
    test_gmon.sensors.light.num_items = 1;
    status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    void          *first_alloc_ptr = test_gmon.rawmsg.outflight.data;
    unsigned short first_alloc_len = test_gmon.rawmsg.outflight.len;
    TEST_ASSERT_NOT_NULL(first_alloc_ptr);
    TEST_ASSERT_TRUE(first_alloc_len > 0);

    // 2. Call again with the exact same sensor configuration.
    // staAppMsgReallocBuffer internally calls staEnsureStrBufferSize,
    // which should reuse the existing buffer if its allocated size matches the new required size.
    status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    void          *second_alloc_ptr = test_gmon.rawmsg.outflight.data;
    unsigned short second_alloc_len = test_gmon.rawmsg.outflight.len;
    TEST_ASSERT_NOT_NULL(second_alloc_ptr);
    TEST_ASSERT_EQUAL_PTR(second_alloc_ptr, test_gmon.rawmsg.inflight.data);
    TEST_ASSERT_EQUAL(first_alloc_len, second_alloc_len);
    // Expect pointer to be the same (buffer reused)
    TEST_ASSERT_EQUAL_PTR(first_alloc_ptr, second_alloc_ptr);
}

TEST(ReallocBuffer, GrowShrinkBuffer) {
    // 1. Initial allocation with a smaller configuration
    test_gmon.sensors.soil_moist.super.num_items = 1;
    test_gmon.sensors.air_temp.num_items = 1;
    test_gmon.sensors.light.num_items = 1;
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    void          *first_alloc_ptr = test_gmon.rawmsg.outflight.data;
    unsigned short first_alloc_len = test_gmon.rawmsg.outflight.len;
    TEST_ASSERT_NOT_NULL(first_alloc_ptr);
    TEST_ASSERT_GREATER_THAN(0, first_alloc_len);

    // 2. Modify sensor counts to require a larger buffer
    test_gmon.sensors.soil_moist.super.num_items = 2;
    test_gmon.sensors.air_temp.num_items = 2;
    test_gmon.sensors.light.num_items = 3;
    status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    void          *second_alloc_ptr = test_gmon.rawmsg.outflight.data;
    unsigned short second_alloc_len = test_gmon.rawmsg.outflight.len;
    TEST_ASSERT_NOT_NULL(second_alloc_ptr);
    TEST_ASSERT_EQUAL_PTR(second_alloc_ptr, test_gmon.rawmsg.inflight.data);
    TEST_ASSERT_GREATER_THAN(first_alloc_len, second_alloc_len);
    TEST_ASSERT_NOT_EQUAL(first_alloc_ptr, second_alloc_ptr);

    // 3. Modify sensor counts to require a smaller buffer
    test_gmon.sensors.soil_moist.super.num_items = 2;
    test_gmon.sensors.air_temp.num_items = 2;
    test_gmon.sensors.light.num_items = 2;
    status = staAppMsgReallocBuffer(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    void          *third_alloc_ptr = test_gmon.rawmsg.outflight.data;
    unsigned short third_alloc_len = test_gmon.rawmsg.outflight.len;
    TEST_ASSERT_NOT_NULL(third_alloc_ptr);
    TEST_ASSERT_EQUAL_PTR(third_alloc_ptr, test_gmon.rawmsg.inflight.data);
    TEST_ASSERT_NOT_EQUAL(first_alloc_ptr, third_alloc_ptr);
    TEST_ASSERT_NOT_EQUAL(second_alloc_ptr, third_alloc_ptr);
    TEST_ASSERT_GREATER_THAN(first_alloc_len, second_alloc_len);
    TEST_ASSERT_GREATER_THAN(third_alloc_len, second_alloc_len);
    TEST_ASSERT_GREATER_THAN(first_alloc_len, third_alloc_len);
}

TEST_GROUP_RUNNER(gMonAppMsgOutbound) {
    RUN_TEST_CASE(GenerateMsgOutflight, EmptyLogEvt);
    RUN_TEST_CASE(GenerateMsgOutflight, SingleLogEvtPerSensor);
    RUN_TEST_CASE(GenerateMsgOutflight, MultiLogEvtPerSensor);
    RUN_TEST_CASE(GenerateMsgOutflight, FullLogEvt);
    RUN_TEST_CASE(GenerateMsgOutflight, LogEvtDataMissing);
    RUN_TEST_CASE(GenerateMsgOutflight, LogEvtInterleavedNullRef);
    RUN_TEST_CASE(GenerateMsgOutflight, LogEvtExtremeValues);
    RUN_TEST_CASE(GenerateMsgOutflight, InsufficientMemory);
    RUN_TEST_CASE(GenerateMsgOutflight, NumDigitExceedLimit);
    RUN_TEST_CASE(UpdateLastRecord, AddFirstEventRef);
    RUN_TEST_CASE(UpdateLastRecord, AccumulateEventRefs);
    RUN_TEST_CASE(UpdateLastRecord, SensorEvtRefsFull_DiscardOldest);
    RUN_TEST_CASE(UpdateLastRecord, AddNullEventToEmptyRecord);
    RUN_TEST_CASE(UpdateLastRecord, AddNullEventToPartiallyFilledRecord);
    RUN_TEST_CASE(UpdateLastRecord, AddNullEventToFullRecord);
    RUN_TEST_CASE(ReallocBuffer, SameSize_ReuseBuffer);
    RUN_TEST_CASE(ReallocBuffer, GrowShrinkBuffer);
}
