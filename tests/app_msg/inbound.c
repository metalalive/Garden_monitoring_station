#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"
#include "mocks.h"

TEST_GROUP(DecodeMsgInflight);

// Test fixtures
static gardenMonitor_t test_gmon; // Global gardenMonitor_t for tests

TEST_SETUP(DecodeMsgInflight) {
    // Reset test_gmon and mock statuses before each test
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    staAppMsgInit(&test_gmon);
    // ensure in-flight message reset, avoid uninitialized value
    (void)staGetAppMsgInflight(&test_gmon);
}

TEST_TEAR_DOWN(DecodeMsgInflight) { staAppMsgDeinit(&test_gmon); }

TEST(DecodeMsgInflight, EmptyJson) {
    const unsigned char *json_data = (const unsigned char *)"{}";
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, 2);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, 2);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidIntervalNetconn) {
    const unsigned char *json_data = (const unsigned char *)"{\"netconn\":{\"interval\":3600000}}";
    uint16_t             testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(3600000, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidIntervalSensor) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"interval\":10009},\"airtemp\":{\"interval\":"
                               "20008},\"light\":{\"interval\":30007}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10009, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(20008, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(30007, test_gmon.sensors.light.read_interval_ms);
}

TEST(DecodeMsgInflight, ValidThresholds) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"threshold\":1019},\"airtemp\":{\"threshold\":"
                               "35},\"light\":{\"threshold\":521}},\"daylength\":7200000}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(1019, test_gmon.actuator.pump.threshold);
    TEST_ASSERT_EQUAL(35, test_gmon.actuator.fan.threshold);
    TEST_ASSERT_EQUAL(521, test_gmon.actuator.bulb.threshold);
    TEST_ASSERT_EQUAL(7200000, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValid) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"interval\":2100,\"threshold\":934},\"airtemp\":"
                               "{\"interval\":7100,\"threshold\":31},\"light\":{\"interval\":11000,"
                               "\"threshold\":609}},\"netconn\":{\"interval\":360095},\"daylength\":7200012}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.threshold);
    TEST_ASSERT_EQUAL(609, test_gmon.actuator.bulb.threshold);
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValidReordered) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"daylength\":7200012,\"netconn\":{\"interval\":360095},\"sensor\":{"
                               "\"light\":{\"interval\":11000,\"threshold\":819},\"airtemp\":{\"interval\":"
                               "7100,\"threshold\":31},\"soilmoist\":{\"interval\":2100,\"threshold\":934}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Assert sensor read intervals
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    // Assert netconn interval
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    // Assert output device thresholds
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.threshold);
    TEST_ASSERT_EQUAL(819, test_gmon.actuator.bulb.threshold);
    // Assert required daylight length
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, InvalidRootType) {
    const unsigned char *json_data = (const unsigned char *)"[1,2,3]"; // Array instead of object
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, strlen((const char *)json_data));
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
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(DecodeMsgInflight, UnknownTopLevelKey) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"unknown_key\":\"some_value\", \"netconn\":{\"interval\":100}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status); // Should skip unknown key and continue parsing
    TEST_ASSERT_EQUAL(100, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, NestedUnknownKeys) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"netconn\":{\"interval\":146, \"junk\":{\"nested_junk\":1}}, "
                               "\"sensor\":{\"soilmoist\":{\"threshold\":291}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(146, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(291, test_gmon.actuator.pump.threshold);
}

TEST(DecodeMsgInflight, ValidActuatorConfig) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{\"max_worktime\":10000,\"min_resttime\":1000},"
                               "\"fan\":{\"max_worktime\":20000,\"min_resttime\":2000},\"bulb\":{\"max_"
                               "worktime\":30000,\"min_resttime\":3000}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10000, test_gmon.actuator.pump.max_worktime);
    TEST_ASSERT_EQUAL(1000, test_gmon.actuator.pump.min_resttime);
    TEST_ASSERT_EQUAL(20000, test_gmon.actuator.fan.max_worktime);
    TEST_ASSERT_EQUAL(2000, test_gmon.actuator.fan.min_resttime);
    TEST_ASSERT_EQUAL(30000, test_gmon.actuator.bulb.max_worktime);
    TEST_ASSERT_EQUAL(3000, test_gmon.actuator.bulb.min_resttime);
}

TEST(DecodeMsgInflight, ValidActuatorConfigPartial) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{\"max_worktime\":15000}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(15000, test_gmon.actuator.pump.max_worktime);
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.pump.min_resttime); // Should be 0 as not specified
    TEST_ASSERT_EQUAL(0, test_gmon.actuator.fan.max_worktime);  // Should be 0 as not specified
}

TEST(DecodeMsgInflight, ValidActuatorConfigUnknownKey) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{\"max_worktime\":11000,\"unknown_act_key\":"
                               "\"ignored_val\",\"min_resttime\":1100}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status); // Should skip unknown key and continue parsing
    TEST_ASSERT_EQUAL(11000, test_gmon.actuator.pump.max_worktime);
    TEST_ASSERT_EQUAL(1100, test_gmon.actuator.pump.min_resttime);
}

TEST(DecodeMsgInflight, ComprehensiveConfigWithActuators) {
    const unsigned char *json_data =
        (const unsigned char
             *)"{\"sensor\":{\"soilmoist\":{\"interval\":2100,\"threshold\":934},\"airtemp\":{\"interval\":"
               "7100,\"threshold\":31},\"light\":{\"interval\":11000,\"threshold\":189}},\"netconn\":{"
               "\"interval\":360095},\"daylength\":7200012,\"actuators\":{\"pump\":{\"max_worktime\":50000,"
               "\"min_resttime\":5000},\"fan\":{\"max_worktime\":60000,\"min_resttime\":6000},\"bulb\":{"
               "\"max_worktime\":70000,\"min_resttime\":7000}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Sensor intervals
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    // Netconn interval
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    // Output device thresholds
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.threshold);
    TEST_ASSERT_EQUAL(189, test_gmon.actuator.bulb.threshold);
    // Required daylight length
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
    // Actuator work/rest times
    TEST_ASSERT_EQUAL(50000, test_gmon.actuator.pump.max_worktime);
    TEST_ASSERT_EQUAL(5000, test_gmon.actuator.pump.min_resttime);
    TEST_ASSERT_EQUAL(60000, test_gmon.actuator.fan.max_worktime);
    TEST_ASSERT_EQUAL(6000, test_gmon.actuator.fan.min_resttime);
    TEST_ASSERT_EQUAL(70000, test_gmon.actuator.bulb.max_worktime);
    TEST_ASSERT_EQUAL(7000, test_gmon.actuator.bulb.min_resttime);
}

TEST_GROUP_RUNNER(gMonAppMsgInbound) {
    RUN_TEST_CASE(DecodeMsgInflight, EmptyJson);
    RUN_TEST_CASE(DecodeMsgInflight, ValidIntervalNetconn);
    RUN_TEST_CASE(DecodeMsgInflight, ValidIntervalSensor);
    RUN_TEST_CASE(DecodeMsgInflight, ValidThresholds);
    RUN_TEST_CASE(DecodeMsgInflight, MixedValid);
    RUN_TEST_CASE(DecodeMsgInflight, MixedValidReordered);
    RUN_TEST_CASE(DecodeMsgInflight, InvalidRootType);
    RUN_TEST_CASE(DecodeMsgInflight, NoTokens);
    RUN_TEST_CASE(DecodeMsgInflight, MalformedIntervalObject);
    RUN_TEST_CASE(DecodeMsgInflight, MalformedThresholdObject);
    RUN_TEST_CASE(DecodeMsgInflight, UnknownTopLevelKey);
    RUN_TEST_CASE(DecodeMsgInflight, NestedUnknownKeys);
    RUN_TEST_CASE(DecodeMsgInflight, ValidActuatorConfig);
    RUN_TEST_CASE(DecodeMsgInflight, ValidActuatorConfigPartial);
    RUN_TEST_CASE(DecodeMsgInflight, ValidActuatorConfigUnknownKey);
    RUN_TEST_CASE(DecodeMsgInflight, ComprehensiveConfigWithActuators);
}
