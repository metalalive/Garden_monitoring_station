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

TEST(DecodeMsgInflight, ValidSpareSensor) {
    const unsigned char *json_data =
        (const unsigned char
             *)"{\"sensor\":{\"soilmoist\":{\"interval\":10009,\"mad\":[8,3]},\"airtemp\":{"
               "\"interval\":20008,\"resample\":2},\"light\":{\"interval\":30007,\"qty\":5}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
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
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(7, test_gmon.sensors.soil_moist.super.num_items);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.air_temp.num_items);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.light.num_items);
}

TEST(DecodeMsgInflight, SensorResampleExceed) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"sensor\":{\"soilmoist\":{\"resample\":6},"
                               "\"airtemp\":{\"resample\":5},\"light\":{\"resample\":7}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(6, test_gmon.sensors.soil_moist.super.num_resamples);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.air_temp.num_resamples);
    TEST_ASSERT_EQUAL(0, test_gmon.sensors.light.num_resamples);
}

TEST(DecodeMsgInflight, ValidThresholds) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"actuators\":{\"pump\":{\"threshold\":1019},\"fan\":{\"threshold\":"
                               "35},\"bulb\":{\"threshold\":521}},\"daylength\":7200000}";
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
        (const unsigned char
             *)"{\"sensor\":{\"soilmoist\":{\"interval\":2100,\"qty\":3,\"outlier\":[32,11]},"
               "\"airtemp\":{\"interval\":7100,\"qty\":2,\"outlier\":[33,12]},"
               "\"light\":{\"interval\":11000,\"resample\":5,\"mad\":[38,13]}}"
               ",\"actuators\":{\"pump\":{\"threshold\":934},\"fan\":{\"threshold\":31},\"bulb\":"
               "{\"threshold\":609}},\"netconn\":{\"interval\":360095},\"daylength\":7200012}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
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
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.threshold);
    TEST_ASSERT_EQUAL(609, test_gmon.actuator.bulb.threshold);
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValidReordered) {
    const unsigned char *json_data =
        (const unsigned char *)"{\"daylength\":7200012,\"netconn\":{\"interval\":360095},"
                               "\"sensor\":{\"light\":{\"qty\":7,\"interval\":11000},"
                               "\"airtemp\":{\"interval\":7100,\"resample\":2,\"qty\":3},"
                               "\"soilmoist\":{\"qty\":6,\"interval\":2100,\"resample\":3}},"
                               "\"actuators\":{\"pump\":{\"threshold\":934},\"fan\":{\"threshold\":31},"
                               "\"bulb\":{\"threshold\":819}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
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
    TEST_ASSERT_EQUAL(934, test_gmon.actuator.pump.threshold);
    TEST_ASSERT_EQUAL(31, test_gmon.actuator.fan.threshold);
    TEST_ASSERT_EQUAL(819, test_gmon.actuator.bulb.threshold);
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
        (const unsigned char *)"{\"netconn\":{\"interval\":146, \"junk\":{\"nested_junk\":1}}, \"sensor\":{"
                               "\"soilmoist\":{}}, \"actuators\":{\"pump\":{\"threshold\":291}}}";
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
    const char *json_data =
        "{\"sensor\":{"
        "\"soilmoist\":{\"interval\":2100,\"qty\":3,\"resample\":5,\"outlier\":[35,13],\"mad\":[40,17]},"
        "\"airtemp\":{\"interval\":7100,\"qty\":4,\"resample\":2,\"outlier\":[35,14],\"mad\":[42,19]},"
        "\"light\":{\"interval\":11000,\"qty\":6,\"resample\":3,\"outlier\":[36,13],\"mad\":[43,23]}},"
        "\"netconn\":{\"interval\":360095},\"daylength\":7200012,\"actuators\":{"
        "\"pump\":{\"max_worktime\":50000,\"min_resttime\":5000,\"threshold\":934},"
        "\"fan\":{\"max_worktime\":60000,\"min_resttime\":6000,\"threshold\":31},"
        "\"bulb\":{\"max_worktime\":70000,\"min_resttime\":7000,\"threshold\":189}}}";
    uint16_t testdata_sz = strlen(json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
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

TEST(DecodeMsgInflight, SensorOutlierDenominatorZero) {
    const unsigned char *json_data =
        (const unsigned char
             *)"{\"sensor\":{\"soilmoist\":{\"outlier\":[4,1]},\"airtemp\":{\"outlier\":[4,0]},"
               "\"light\":{\"outlier\":[10,4]}}}";
    uint16_t testdata_sz = strlen((const char *)json_data);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
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
