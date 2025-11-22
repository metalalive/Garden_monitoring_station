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
    (void) staGetAppMsgInflight(&test_gmon);
}

TEST_TEAR_DOWN(DecodeMsgInflight) {
    staAppMsgDeinit(&test_gmon);
}

TEST(DecodeMsgInflight, EmptyJson) {
    const unsigned char *json_data = (const unsigned char*)"{}";
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, 2);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, 2);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(0, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidIntervalNetconn) {
    const unsigned char *json_data = (const unsigned char*)"{\"interval\":{\"netconn\":3600000}}";
    uint16_t testdata_sz = strlen((const char*)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(3600000, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidIntervalSensor) {
    const unsigned char *json_data = (const unsigned char*)"{\"interval\":{\"sensor\":{\"soil_moist\":10009,\"air_temp\":20008,\"light\":30007}}}";
    uint16_t testdata_sz = strlen((const char*)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10009, test_gmon.sensors.soil_moist.read_interval_ms);
    TEST_ASSERT_EQUAL(20008, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(30007, test_gmon.sensors.light.read_interval_ms);
}

TEST(DecodeMsgInflight, ValidThresholds) {
    const unsigned char *json_data = (const unsigned char*)"{\"threshold\":{\"soilmoist\":1234,\"airtemp\":35,\"light\":4321,\"daylength\":7200000}}";
    uint16_t testdata_sz = strlen((const char*)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(1234, test_gmon.outdev.pump.threshold);
    TEST_ASSERT_EQUAL(35, test_gmon.outdev.fan.threshold);
    TEST_ASSERT_EQUAL(4321, test_gmon.outdev.bulb.threshold);
    TEST_ASSERT_EQUAL(7200000, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValid) {
    const unsigned char *json_data = (const unsigned char*)"{\"interval\":{\"sensor\": {\"soil_moist\": 2100, \"air_temp\": 7100, \"light\": 11000}, \"netconn\": 360095}, \"threshold\": {\"soilmoist\": 934, \"airtemp\": 31, \"light\": 1099, \"daylength\":7200012 }}";
    uint16_t testdata_sz = strlen((const char*)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(2100, test_gmon.sensors.soil_moist.read_interval_ms);
    TEST_ASSERT_EQUAL(7100, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(11000, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(360095, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(934, test_gmon.outdev.pump.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.outdev.fan.threshold);
    TEST_ASSERT_EQUAL(1099, test_gmon.outdev.bulb.threshold);
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, InvalidRootType) {
    const unsigned char *json_data = (const unsigned char*)"[1,2,3]"; // Array instead of object
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, strlen((const char*)json_data));
    test_gmon.rawmsg.inflight.len = strlen((const char*)json_data);
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
    const unsigned char *json_data = (const unsigned char*)"{\"interval\":123}";
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, strlen((const char*)json_data));
    test_gmon.rawmsg.inflight.len = strlen((const char*)json_data);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_DECODE, status); // staDecodeIntervalObject expects OBJECT
}

TEST(DecodeMsgInflight, MalformedThresholdObject) {
    const unsigned char *json_data = (const unsigned char*)"{\"threshold\":\"invalid\"}"; // String value for threshold
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, strlen((const char*)json_data));
    test_gmon.rawmsg.inflight.len = strlen((const char*)json_data);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_MSG_DECODE, status); // staDecodeThresholdObject expects OBJECT
}

TEST(DecodeMsgInflight, UnknownTopLevelKey) {
    const unsigned char *json_data = (const unsigned char*)"{\"unknown_key\":\"some_value\", \"interval\":{\"netconn\":100}}";
    uint16_t testdata_sz = strlen((const char*)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status); // Should skip unknown key and continue parsing
    TEST_ASSERT_EQUAL(100, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, NestedUnknownKeys) {
    const unsigned char *json_data = (const unsigned char*)"{\"interval\":{\"netconn\":146, \"junk\":{\"nested_junk\":1}}, \"threshold\":{\"soilmoist\":291}}";
    uint16_t testdata_sz = strlen((const char*)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(146, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(291, test_gmon.outdev.pump.threshold);
}

TEST_GROUP_RUNNER(gMonAppMsg) {
    RUN_TEST_CASE(DecodeMsgInflight, EmptyJson);
    RUN_TEST_CASE(DecodeMsgInflight, ValidIntervalNetconn);
    RUN_TEST_CASE(DecodeMsgInflight, ValidIntervalSensor);
    RUN_TEST_CASE(DecodeMsgInflight, ValidThresholds);
    RUN_TEST_CASE(DecodeMsgInflight, MixedValid);
    RUN_TEST_CASE(DecodeMsgInflight, InvalidRootType);
    RUN_TEST_CASE(DecodeMsgInflight, NoTokens);
    RUN_TEST_CASE(DecodeMsgInflight, MalformedIntervalObject);
    RUN_TEST_CASE(DecodeMsgInflight, MalformedThresholdObject);
    RUN_TEST_CASE(DecodeMsgInflight, UnknownTopLevelKey);
    RUN_TEST_CASE(DecodeMsgInflight, NestedUnknownKeys);
}
