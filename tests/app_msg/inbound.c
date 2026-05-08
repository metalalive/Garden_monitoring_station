#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"
#include "mocks.h"

TEST_GROUP(DecodeMsgInflight);

// Test fixtures
static gMonActuator_t  test_actuator[3] = {0};
static gardenMonitor_t test_gmon = {0};

TEST_SETUP(DecodeMsgInflight) {
    // Reset test_gmon and mock statuses before each test
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    XMEMSET(test_actuator, 0, 3 * sizeof(gMonActuator_t));
    test_actuator[0].id = 9;
    test_actuator[1].id = 14;
    test_actuator[2].id = 6;
    test_gmon.actuator.pump = (gMonActuators_t){.count = 1, .entries = &test_actuator[0]};
    test_gmon.actuator.fan = (gMonActuators_t){.count = 1, .entries = &test_actuator[1]};
    test_gmon.actuator.bulb = (gMonActuators_t){.count = 1, .entries = &test_actuator[2]};
    staAppMsgInit(&test_gmon);
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    XASSERT(GMON_RESP_OK == status);
    // ensure in-flight message reset, avoid uninitialized value
    (void)staGetAppMsgInflight(&test_gmon);
}

TEST_TEAR_DOWN(DecodeMsgInflight) { staAppMsgDeinit(&test_gmon); }

TEST(DecodeMsgInflight, EmptyJson) {
    const unsigned char *json_data = (const unsigned char *)"{}";
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, 2);
    test_gmon.rawmsg.inflight.nbytes_written = 2;
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, 2);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidIntervalNetconn) {
    const unsigned char *json_data = (const unsigned char *)"{\"netconn\":{\"itvl\":3600000}}";
    uint16_t             testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(3600000, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidSpareSensor) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"itvl\":10009,\"mad\":[8,3]},\"airtemp\":{"
                               "\"itvl\":20008,\"rsmp\":2},\"light\":{\"itvl\":30007,\"qty\":5}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10009, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(20008, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(30007, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(2, test_gmon.sensors.air_temp.num_resamples);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.light.num_items);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.666f, test_gmon.sensors.soil_moist.super.mad_threshold);
}

TEST(DecodeMsgInflight, ValidQtySensor) {
    // This test ensures `qty` is parsed even if `interval` is not present
    const unsigned char *json_data =
        (const unsigned char
             *)"{\"sensor\":{\"soilmoist\":{\"qty\":5},\"airtemp\":{\"qty\":3},\"light\":{\"qty\":2}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.soil_moist.super.num_items);
    TEST_ASSERT_EQUAL(3, test_gmon.sensors.air_temp.num_items);
    TEST_ASSERT_EQUAL(2, test_gmon.sensors.light.num_items);
}

TEST(DecodeMsgInflight, SensorQtyExceed) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"qty\":7},\"airtemp\":{\"qty\":8},"
                               "\"light\":{\"qty\":6}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(7, test_gmon.sensors.soil_moist.super.num_items);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.air_temp.num_items);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.light.num_items);
}

TEST(DecodeMsgInflight, SensorResampleExceed) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"rsmp\":6},"
                               "\"airtemp\":{\"rsmp\":5},\"light\":{\"rsmp\":7}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(6, test_gmon.sensors.soil_moist.super.num_resamples);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.air_temp.num_resamples);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.light.num_resamples);
}

TEST(DecodeMsgInflight, ValidThresholds) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"bulb\":{\"id\":6,\"thre\":521},\"fan\":{\"id\":14,"
                               "\"thre\":35},\"pump\":{\"id\":9,\"thre\":1019}},\"daylength\":7200000}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(1019, test_gmon.actuator.pump.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(35, test_gmon.actuator.fan.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(521, test_gmon.actuator.bulb.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(7200000, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValid) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"itvl\":2100,\"qty\":3,\"outlier\":[32,11]},"
                               "\"airtemp\":{\"itvl\":7100,\"qty\":2,\"outlier\":[33,12]},"
                               "\"light\":{\"itvl\":11000,\"rsmp\":5,\"mad\":[38,13]}}"
                               ",\"actuators\":{\"pump\":{\"id\":9,\"thre\":934},\"fan\":{\"id\":14,"
                               "\"thre\":31},\"bulb\":{\"id\":6,\"thre\":609}},"
                               "\"netconn\":{\"itvl\":360095},\"daylength\":7200012}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(3, test_gmon.sensors.soil_moist.super.num_items);
    TEST_ASSERT_EQUAL(2, test_gmon.sensors.air_temp.num_items);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.light.num_resamples);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.90909f, test_gmon.sensors.soil_moist.super.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, 2.75f, test_gmon.sensors.air_temp.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.923f, test_gmon.sensors.light.mad_threshold);
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(609, test_gmon.actuator.bulb.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValidReordered) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"daylength\":7200012,\"netconn\":{\"itvl\":360095},"
                               "\"sensor\":{\"light\":{\"qty\":7,\"itvl\":11000},"
                               "\"airtemp\":{\"itvl\":7100,\"rsmp\":2,\"qty\":3},"
                               "\"soilmoist\":{\"qty\":6,\"itvl\":2100,\"rsmp\":3}},"
                               "\"actuators\":{\"pump\":{\"id\":9,\"thre\":934},\"fan\":{\"id\":14"
                               ",\"thre\":31},\"bulb\":{\"id\":6,\"thre\":819,\"work\":156700}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Assert sensor read intervals
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(6, test_gmon.sensors.soil_moist.super.num_items);
    TEST_ASSERT_EQUAL(3, test_gmon.sensors.air_temp.num_items);
    TEST_ASSERT_EQUAL(7, test_gmon.sensors.light.num_items);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.light.num_resamples);
    TEST_ASSERT_EQUAL(2, test_gmon.sensors.air_temp.num_resamples);
    TEST_ASSERT_EQUAL(3, test_gmon.sensors.soil_moist.super.num_resamples);
    // Assert netconn interval
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    // Assert output device thresholds
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(819, test_gmon.actuator.bulb.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(156700, test_gmon.actuator.bulb.entries[0].param.max_worktime);
    // Assert required daylight length
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, InvalidRootType) {
    const unsigned char *json_data = (const unsigned char *)"[1,2,3]"; // Array instead of object
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, strlen((const char *)json_data));
    // Explicitly set length here as it's part of the test data setup
    test_gmon.rawmsg.inflight.len = strlen((const char *)json_data);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_DECODE, status);
}

TEST(DecodeMsgInflight, NoTokens) {
    // For empty string, set len to 0
    test_gmon.rawmsg.inflight.len = 0;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_DECODE, status);
}

TEST(DecodeMsgInflight, MalformedIntervalObject) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"netconn\":123}"; // netconn expects an object, not a primitive
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_DECODE, status);
}

TEST(DecodeMsgInflight, MalformedThresholdObject) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"daylength\":\"invalid_string\"}"; // daylength expects integer, not string
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(DecodeMsgInflight, UnknownTopLevelKey) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"unknown_key\":\"some_value\", \"netconn\":{\"itvl\":100}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status); // Should skip unknown key and continue parsing
    TEST_ASSERT_EQUAL(100, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, NestedUnknownKeys) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"netconn\":{\"itvl\":146, \"junk\":{\"nested_junk\":1}}, \"sensor\":{"
                               "\"soilmoist\":{}}, \"actuators\":{\"pump\":{\"id\":9,\"thre\":291}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(146, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(291, test_gmon.actuator.pump.entries[0].param.threshold);
}

TEST(DecodeMsgInflight, ValidActuatorConfig) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{\"id\":9,\"work\":10000,\"rest\":1000},"
                               "\"fan\":{\"id\":14,\"work\":20000,\"rest\":2000,\"srid\":0},"
                               "\"bulb\":{\"id\":6,\"work\":30000,\"rest\":3000,\"srid\":65}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10000, test_gmon.actuator.pump.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(1000, test_gmon.actuator.pump.entries[0].param.min_resttime);
    TEST_ASSERT_EQUAL(20000, test_gmon.actuator.fan.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(2000, test_gmon.actuator.fan.entries[0].param.min_resttime);
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.fan.entries[0].param.sensor_id_mask);
    TEST_ASSERT_EQUAL(30000, test_gmon.actuator.bulb.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(3000, test_gmon.actuator.bulb.entries[0].param.min_resttime);
    TEST_ASSERT_EQUAL(0x41, test_gmon.actuator.bulb.entries[0].param.sensor_id_mask);
}

TEST(DecodeMsgInflight, ValidActuatorConfigPartial) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{"
                               "\"id\":9,\"work\":15000},\"fan\":{\"id\":14,\"srid\":19}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(9, test_gmon.actuator.pump.entries[0].id);
    TEST_ASSERT_EQUAL(14, test_gmon.actuator.fan.entries[0].id);
    TEST_ASSERT_EQUAL(15000, test_gmon.actuator.pump.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(0x13, test_gmon.actuator.fan.entries[0].param.sensor_id_mask);
    // Should be 0 as not specified
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.pump.entries[0].param.min_resttime);
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.pump.entries[0].param.sensor_id_mask);
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.fan.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.fan.entries[0].param.min_resttime);
}

TEST(DecodeMsgInflight, ValidActuatorConfigUnknownKey) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{\"id\":9,\"work\":11000,"
                               "\"unknown_act_key\":\"ignored_val\",\"rest\":1100}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status); // Should skip unknown key and continue parsing
    TEST_ASSERT_EQUAL(11000, test_gmon.actuator.pump.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(1100, test_gmon.actuator.pump.entries[0].param.min_resttime);
}

TEST(DecodeMsgInflight, ComprehensiveConfigWithActuators) {
    const char *json_data =
        "{\"sensor\":{"
        "\"soilmoist\":{\"itvl\":2100,\"qty\":3,\"rsmp\":5,\"outlier\":[35,13],\"mad\":[40,17]},"
        "\"airtemp\":{\"itvl\":7100,\"qty\":4,\"rsmp\":2,\"outlier\":[35,14],\"mad\":[42,19]},"
        "\"light\":{\"itvl\":11000,\"qty\":6,\"rsmp\":3,\"outlier\":[36,13],\"mad\":[43,23]}},"
        "\"netconn\":{\"itvl\":360095},\"daylength\":7200012,\"actuators\":{"
        "\"pump\":{\"id\":9,\"work\":50000,\"rest\":5000,\"srid\":255,\"thre\":934},"
        "\"fan\":{\"id\":14,\"work\":60000,\"rest\":6000,\"srid\":201,\"thre\":31},"
        "\"bulb\":{\"id\":6,\"work\":70000,\"rest\":7000,\"srid\":172,\"thre\":189}}}";
    uint16_t testdata_sz = strlen(json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Sensor atttributes
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(3, test_gmon.sensors.soil_moist.super.num_items);
    TEST_ASSERT_EQUAL(4, test_gmon.sensors.air_temp.num_items);
    TEST_ASSERT_EQUAL(6, test_gmon.sensors.light.num_items);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.soil_moist.super.num_resamples);
    TEST_ASSERT_EQUAL(2, test_gmon.sensors.air_temp.num_resamples);
    TEST_ASSERT_EQUAL(3, test_gmon.sensors.light.num_resamples);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.6923f, test_gmon.sensors.soil_moist.super.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.3529f, test_gmon.sensors.soil_moist.super.mad_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.5f, test_gmon.sensors.air_temp.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.2105f, test_gmon.sensors.air_temp.mad_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.7692f, test_gmon.sensors.light.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.8695f, test_gmon.sensors.light.mad_threshold);
    // Netconn interval
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    // Output device identities
    TEST_ASSERT_EQUAL(9, test_gmon.actuator.pump.entries[0].id);
    TEST_ASSERT_EQUAL(14, test_gmon.actuator.fan.entries[0].id);
    TEST_ASSERT_EQUAL(6, test_gmon.actuator.bulb.entries[0].id);
    // Output device thresholds / corresponding sensor ids
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(189, test_gmon.actuator.bulb.entries[0].param.threshold);
    TEST_ASSERT_EQUAL(255, test_gmon.actuator.pump.entries[0].param.sensor_id_mask);
    TEST_ASSERT_EQUAL(201, test_gmon.actuator.fan.entries[0].param.sensor_id_mask);
    TEST_ASSERT_EQUAL(172, test_gmon.actuator.bulb.entries[0].param.sensor_id_mask);
    // Required daylight length
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
    // Actuator work/rest times
    TEST_ASSERT_EQUAL(50000, test_gmon.actuator.pump.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(5000, test_gmon.actuator.pump.entries[0].param.min_resttime);
    TEST_ASSERT_EQUAL(60000, test_gmon.actuator.fan.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(6000, test_gmon.actuator.fan.entries[0].param.min_resttime);
    TEST_ASSERT_EQUAL(70000, test_gmon.actuator.bulb.entries[0].param.max_worktime);
    TEST_ASSERT_EQUAL(7000, test_gmon.actuator.bulb.entries[0].param.min_resttime);
}

TEST(DecodeMsgInflight, SensorOutlierDenominatorZero) {
    const unsigned char *json_data =
        (const unsigned char
             *)"{\"sensor\":{\"soilmoist\":{\"outlier\":[4,1]},\"airtemp\":{\"outlier\":[4,0]},"
               "\"light\":{\"outlier\":[10,4]}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    // If the denominator is zero, the outlier_threshold should not be updated from its initial value.
    // Also, subsequent sensor configurations should not be applied if the request is invalid.
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, test_gmon.sensors.soil_moist.super.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, test_gmon.sensors.air_temp.outlier_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, test_gmon.sensors.light.outlier_threshold);
}

TEST(DecodeMsgInflight, SensorMADdenominatorZero) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"mad\":[6,1]},\"airtemp\":{\"mad\":[6,0]},"
                               "\"light\":{\"mad\":[7,4]}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    // If the denominator is zero, the outlier_threshold should not be updated from its initial value.
    // Also, subsequent sensor configurations should not be applied if the request is invalid.
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, test_gmon.sensors.soil_moist.super.mad_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, test_gmon.sensors.air_temp.mad_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, test_gmon.sensors.light.mad_threshold);
}

TEST_GROUP_RUNNER(gMonAppMsgInbound) {
    RUN_TEST_CASE(DecodeMsgInflight, EmptyJson);
    RUN_TEST_CASE(DecodeMsgInflight, ValidIntervalNetconn);
    RUN_TEST_CASE(DecodeMsgInflight, ValidSpareSensor);
    RUN_TEST_CASE(DecodeMsgInflight, ValidQtySensor);
    RUN_TEST_CASE(DecodeMsgInflight, ValidThresholds);
    RUN_TEST_CASE(DecodeMsgInflight, MixedValid);
    RUN_TEST_CASE(DecodeMsgInflight, MixedValidReordered);
    RUN_TEST_CASE(DecodeMsgInflight, InvalidRootType);
    RUN_TEST_CASE(DecodeMsgInflight, SensorQtyExceed);
    RUN_TEST_CASE(DecodeMsgInflight, SensorResampleExceed);
    RUN_TEST_CASE(DecodeMsgInflight, NoTokens);
    RUN_TEST_CASE(DecodeMsgInflight, MalformedIntervalObject);
    RUN_TEST_CASE(DecodeMsgInflight, MalformedThresholdObject);
    RUN_TEST_CASE(DecodeMsgInflight, UnknownTopLevelKey);
    RUN_TEST_CASE(DecodeMsgInflight, NestedUnknownKeys);
    RUN_TEST_CASE(DecodeMsgInflight, ValidActuatorConfig);
    RUN_TEST_CASE(DecodeMsgInflight, ValidActuatorConfigPartial);
    RUN_TEST_CASE(DecodeMsgInflight, ValidActuatorConfigUnknownKey);
    RUN_TEST_CASE(DecodeMsgInflight, ComprehensiveConfigWithActuators);
    RUN_TEST_CASE(DecodeMsgInflight, SensorOutlierDenominatorZero);
    RUN_TEST_CASE(DecodeMsgInflight, SensorMADdenominatorZero);
}
