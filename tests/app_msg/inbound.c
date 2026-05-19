#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"
#include "mocks.h"

TEST_GROUP(DecodeMsgInflight);

// Test fixtures
static gardenMonitor_t test_gmon = {0};

#define UT_SET_ACTUATORS_ID(ators, ema_lambda_fixpt, ...) \
    { \
        gMonActuatorId_t _ids[] = {__VA_ARGS__}; \
        for (int i = 0; i < (ators).count; i++) { \
            gMonActuator_t *ator = &(ators).entries[i]; \
            gMonStatus      status = staActuatorGenericInit(ator, _ids[i], ema_lambda_fixpt); \
            XASSERT(GMON_RESP_OK == status); \
        } \
    }

TEST_SETUP(DecodeMsgInflight) {
    // Reset test_gmon and mock statuses before each test
    XMEMSET(&test_gmon, 0, sizeof(gardenMonitor_t));
    XASSERT(GMON_RESP_OK == staActuatorGrowSize(&test_gmon.actuator.pump, 3));
    XASSERT(GMON_RESP_OK == staActuatorGrowSize(&test_gmon.actuator.fan, 3));
    XASSERT(GMON_RESP_OK == staActuatorGrowSize(&test_gmon.actuator.bulb, 3));
    UT_SET_ACTUATORS_ID(test_gmon.actuator.pump, GMON_CFG_ACTUATOR_EMA_LAMBDA_PUMP, 9, 5, 11);
    UT_SET_ACTUATORS_ID(test_gmon.actuator.fan, GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN, 14, 7, 12);
    UT_SET_ACTUATORS_ID(test_gmon.actuator.bulb, GMON_CFG_ACTUATOR_EMA_LAMBDA_BULB, 6, 10, 4);
    staAppMsgInit(&test_gmon);
    gMonStatus status = staAppMsgReallocBuffer(&test_gmon);
    XASSERT(GMON_RESP_OK == status);
    // ensure in-flight message reset, avoid uninitialized value
    (void)staGetAppMsgInflight(&test_gmon);
}

TEST_TEAR_DOWN(DecodeMsgInflight) {
    XASSERT(GMON_RESP_OK == staActuatorShrinkSize(&test_gmon.actuator.pump, 0, NULL));
    XASSERT(GMON_RESP_OK == staActuatorShrinkSize(&test_gmon.actuator.fan, 0, NULL));
    XASSERT(GMON_RESP_OK == staActuatorShrinkSize(&test_gmon.actuator.bulb, 0, NULL));
    staAppMsgDeinit(&test_gmon);
}

static void verifyActuator(
    gMonActuator_t *act, uint8_t id, int threshold, uint8_t srid, unsigned int worktime, unsigned int resttime
) {
    TEST_ASSERT_EQUAL(id, act->id);
    TEST_ASSERT_EQUAL(threshold, act->param.threshold);
    TEST_ASSERT_EQUAL(srid, act->param.sensor_id_mask);
    TEST_ASSERT_EQUAL(worktime, act->param.max_worktime);
    TEST_ASSERT_EQUAL(resttime, act->param.min_resttime);
}

static void DecodeInflightMsg(const unsigned char *json_data) {
    uint16_t testdata_sz = strlen((const char *)json_data);
    TEST_ASSERT_LESS_THAN_UINT16(test_gmon.rawmsg.inflight.len, testdata_sz);
    XMEMCPY(test_gmon.rawmsg.inflight.data, json_data, testdata_sz);
    test_gmon.rawmsg.inflight.nbytes_written = testdata_sz;
    gMonStatus status = staDecodeAppMsgInflight(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

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
#define JSONDATA "{\"netconn\":{\"itvl\":3600000}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    TEST_ASSERT_EQUAL(3600000, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, ValidSpareSensor) {
#define JSONDATA \
    "{\"sensor\":{\"soilmoist\":{\"itvl\":10009,\"mad\":[8,3]},\"airtemp\":{\"itvl\":20008,\"rsmp\":2}," \
    "\"light\":{\"itvl\":30007,\"qty\":5}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    TEST_ASSERT_EQUAL(10009, test_gmon.sensors.soil_moist.super.read_interval_ms);
    TEST_ASSERT_EQUAL(20008, test_gmon.sensors.air_temp.read_interval_ms);
    TEST_ASSERT_EQUAL(30007, test_gmon.sensors.light.read_interval_ms);
    TEST_ASSERT_EQUAL(2, test_gmon.sensors.air_temp.num_resamples);
    TEST_ASSERT_EQUAL(5, test_gmon.sensors.light.num_items);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.666f, test_gmon.sensors.soil_moist.super.mad_threshold);
}

TEST(DecodeMsgInflight, ValidQtySensor) {
    // This test ensures `qty` is parsed even if `interval` is not present
#define JSONDATA "{\"sensor\":{\"soilmoist\":{\"qty\":5},\"airtemp\":{\"qty\":3},\"light\":{\"qty\":2}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
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
#define JSONDATA \
    "{\"actuators\":{\"bulb\":{\"items\":[{\"id\":6,\"thre\":521}]},\"fan\":{\"items\":[{\"id\":14," \
    "\"thre\":35}]},\"pump\":{\"items\":[{\"id\":9,\"thre\":1019}]}},\"daylength\":7200000}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 1019, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[0], 14, 35, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.bulb.entries[0], 6, 521, 0, 0, 0);
    TEST_ASSERT_EQUAL(7200000, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValid) {
#define JSONDATA \
    "{\"sensor\":{\"soilmoist\":{\"itvl\":2100,\"qty\":3,\"outlier\":[32,11]},\"airtemp\":{\"itvl\":7100," \
    "\"qty\":2,\"outlier\":[33,12]},\"light\":{\"itvl\":11000,\"rsmp\":5,\"mad\":[38,13]}},\"actuators\":{" \
    "\"pump\":{\"items\":[{\"id\":9,\"thre\":934}]},\"fan\":{\"items\":[{\"id\":14,\"thre\":31}]},\"bulb\":" \
    "{\"items\":[{\"id\":6,\"thre\":609}]}},\"netconn\":{\"itvl\":360095},\"daylength\":7200012}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
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
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 934, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[0], 14, 31, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.bulb.entries[0], 6, 609, 0, 0, 0);
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
}

TEST(DecodeMsgInflight, MixedValidReordered) {
#define JSONDATA \
    "{\"daylength\":7200012,\"netconn\":{\"itvl\":360095},\"sensor\":{\"light\":{\"qty\":7,\"itvl\":11000}," \
    "\"airtemp\":{\"itvl\":7100,\"rsmp\":2,\"qty\":3},\"soilmoist\":{\"qty\":6,\"itvl\":2100,\"rsmp\":3}}," \
    "\"actuators\":{\"pump\":{\"items\":[{\"id\":9,\"thre\":934}]},\"fan\":{\"items\":[{\"id\":14,\"thre\":" \
    "31}]},\"bulb\":{\"items\":[{\"id\":6,\"thre\":819,\"work\":156700}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
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
    // Assert output device configuration
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 934, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[0], 14, 31, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.bulb.entries[0], 6, 819, 0, 156700, 0);
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
#define JSONDATA "{\"unknown_key\":\"some_value\", \"netconn\":{\"itvl\":100}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    TEST_ASSERT_EQUAL(100, test_gmon.netconn.interval_ms);
}

TEST(DecodeMsgInflight, NestedUnknownKeys) {
#define JSONDATA \
    "{\"netconn\":{\"itvl\":146, \"junk\":{\"nested_junk\":1}}, \"sensor\":{\"soilmoist\":{}}, " \
    "\"actuators\":{\"pump\":{\"items\":[{\"id\":9,\"thre\":291}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    TEST_ASSERT_EQUAL(146, test_gmon.netconn.interval_ms);
    TEST_ASSERT_EQUAL(291, test_gmon.actuator.pump.entries[0].param.threshold);
}

TEST(DecodeMsgInflight, ValidActuatorConfig) {
#define JSONDATA \
    "{\"actuators\":{\"pump\":{\"rm\":[5],\"items\":[{\"id\":9,\"work\":10000,\"rest\":1000}]},\"fan\":{" \
    "\"add\":[17],\"items\":[{\"id\":17,\"work\":62000,\"rest\":5000,\"srid\":29,\"thre\":36},{\"id\":14," \
    "\"work\":20000,\"rest\":2000,\"srid\":10}]},\"bulb\":{\"items\":[{\"id\":6,\"work\":30000,\"rest\":" \
    "3000,\"srid\":65}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    // Verify pump (index 0 is 9, index 1 is 5, index 2 is 11. 5 should be removed)
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 0, 0, 10000, 1000);
    verifyActuator(&test_gmon.actuator.pump.entries[1], 11, 0, 0, 0, 0);
    // Verify fan (index 0 is 14, ID = 7, 12 unchanged, index 3 is 17. 14 updated)
    verifyActuator(&test_gmon.actuator.fan.entries[0], 14, 0, 10, 20000, 2000);
    verifyActuator(&test_gmon.actuator.fan.entries[3], 17, 36, 29, 62000, 5000);
    // Verify bulb
    verifyActuator(&test_gmon.actuator.bulb.entries[0], 6, 0, 0x41, 30000, 3000);
    TEST_ASSERT_EQUAL(GMON_CFG_ACTUATOR_EMA_LAMBDA_PUMP, test_gmon.actuator.pump.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN, test_gmon.actuator.fan.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN, test_gmon.actuator.fan.entries[3].ema.lambda_fixp);
}

TEST(DecodeMsgInflight, ValidActuatorConfigPartial) {
#define JSONDATA \
    "{\"actuators\":{\"pump\":{\"items\":[{\"id\":9,\"work\":15000,\"rest\":500}," \
    "{\"id\":5,\"thre\":251}]},\"fan\":{\"add\":[15,17],\"items\":[{\"id\":14," \
    "\"srid\":19,\"work\":2000},{\"id\":17,\"work\":9500,\"srid\":13,\"thre\":38}" \
    ",{\"id\":15,\"srid\":26,\"thre\":39}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 0, 0, 15000, 500);
    verifyActuator(&test_gmon.actuator.pump.entries[1], 5, 251, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[0], 14, 0, 19, 2000, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[1], 7, 0, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[3], 15, 39, 26, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[4], 17, 38, 13, 9500, 0);
    TEST_ASSERT_EQUAL(GMON_CFG_ACTUATOR_EMA_LAMBDA_PUMP, test_gmon.actuator.pump.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN, test_gmon.actuator.fan.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(GMON_CFG_ACTUATOR_EMA_LAMBDA_FAN, test_gmon.actuator.fan.entries[3].ema.lambda_fixp);
#undef JSONDATA
#define JSONDATA \
    "{\"actuators\":{\"pump\":{\"items\":[{\"id\":9,\"work\":16500,\"srid\":33,\"thre\":252}," \
    "{\"id\":5,\"work\":1630,\"srid\":21}]},\"fan\":{\"rm\":[14],\"items\":[{\"id\":7," \
    "\"srid\":32,\"work\":12000},{\"id\":17,\"work\":15200,\"rest\":1800}" \
    ",{\"id\":15,\"srid\":26,\"thre\":39}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 252, 33, 16500, 500);
    verifyActuator(&test_gmon.actuator.pump.entries[1], 5, 251, 21, 1630, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[0], 7, 0, 32, 12000, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[2], 15, 39, 26, 0, 0);
    verifyActuator(&test_gmon.actuator.fan.entries[3], 17, 38, 13, 15200, 1800);
#undef JSONDATA
}

TEST(DecodeMsgInflight, ValidActuatorConfigUnknownKey) {
#define JSONDATA \
    "{\"actuators\":{\"pump\":{\"items\":[{\"id\":9,\"work\":11000,\"unknown_act_key\":\"ignored_val\"," \
    "\"rest\":1100}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 0, 0, 11000, 1100);
}

TEST(DecodeMsgInflight, ComprehensiveConfigWithActuators) {
#define JSONDATA \
    "{\"sensor\":{\"soilmoist\":{\"itvl\":2100,\"qty\":3,\"rsmp\":5,\"outlier\":[35,13],\"mad\":[40,17]}," \
    "\"airtemp\":{\"itvl\":7100,\"qty\":4,\"rsmp\":2,\"outlier\":[35,14],\"mad\":[42,19]},\"light\":{" \
    "\"itvl\":11000,\"qty\":6,\"rsmp\":3,\"outlier\":[36,13],\"mad\":[43,23]}},\"netconn\":{\"itvl\":" \
    "360095},\"daylength\":7200012,\"actuators\":{\"pump\":{\"rm\":[5],\"add\":[17,22],\"items\":[{\"id\":" \
    "9,\"work\":4000,\"rest\":1500,\"srid\":123,\"thre\":1019},{\"id\":22,\"work\":4300,\"rest\":900," \
    "\"srid\":10,\"thre\":1017},{\"id\":17,\"work\":8300,\"rest\":1500,\"srid\":65,\"thre\":1008}]}," \
    "\"fan\":{\"rm\":[12,14],\"add\":[3],\"items\":[{\"id\":7,\"work\":5300,\"rest\":2100,\"srid\":45," \
    "\"thre\":35},{\"id\":3,\"work\":5900,\"rest\":1100,\"srid\":6,\"thre\":35}]},\"bulb\":{\"rm\":[6]," \
    "\"add\":[13],\"items\":[{\"id\":13,\"work\":7100,\"rest\":5700,\"srid\":6,\"thre\":829},{\"id\":10," \
    "\"work\":10500,\"rest\":1300,\"srid\":14,\"thre\":815}]}}}"
    DecodeInflightMsg((const unsigned char *)JSONDATA);
#undef JSONDATA
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
    // Output device configuration
    verifyActuator(&test_gmon.actuator.pump.entries[0], 9, 1019, 123, 4000, 1500);
    verifyActuator(&test_gmon.actuator.pump.entries[1], 17, 1008, 65, 8300, 1500);
    verifyActuator(&test_gmon.actuator.pump.entries[2], 11, 0, 0, 0, 0);
    verifyActuator(&test_gmon.actuator.pump.entries[3], 22, 1017, 10, 4300, 900);
    verifyActuator(&test_gmon.actuator.fan.entries[0], 7, 35, 45, 5300, 2100);
    verifyActuator(&test_gmon.actuator.fan.entries[1], 3, 35, 6, 5900, 1100);
    verifyActuator(&test_gmon.actuator.bulb.entries[0], 13, 829, 6, 7100, 5700);
    verifyActuator(&test_gmon.actuator.bulb.entries[1], 10, 815, 14, 10500, 1300);
    verifyActuator(&test_gmon.actuator.bulb.entries[2], 4, 0, 0, 0, 0);
    // Required daylight length
    TEST_ASSERT_EQUAL(7200012, test_gmon.user_ctrl.required_light_daylength_ticks);
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
