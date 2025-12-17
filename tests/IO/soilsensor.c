#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

TEST_GROUP(SensorFastPoll);

// Helper variables for testing
static gMonSoilSensorMeta_t test_soil_sensor_meta;
static gMonActuator_t       test_pump_actuator;

TEST_SETUP(SensorFastPoll) {
    // Reset test sensor metadata
    XMEMSET(&test_soil_sensor_meta, 0, sizeof(gMonSoilSensorMeta_t));
    test_soil_sensor_meta.super.read_interval_ms = 10000; // 10 seconds
    test_soil_sensor_meta.super.num_items = 4;            // 4 sensors
    test_soil_sensor_meta.fast_poll.divisor = 5;
    test_soil_sensor_meta.fast_poll._div_cnt = 0; // Default to full poll cycle

    // Reset test pump actuator
    XMEMSET(&test_pump_actuator, 0, sizeof(gMonActuator_t));
    test_pump_actuator.sensor_id_mask = (1 << 0) | (1 << 2); // Mask for sensor 0 and 2
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_OFF;
}

TEST_TEAR_DOWN(SensorFastPoll) {}

static void ut_verify_poll_enabled(gMonSoilSensorMeta_t *s, unsigned short len, const char *expect_vals) {
    for (unsigned short idx = 0; idx < len; idx++) {
        char actual_val = staSensorPollEnabled(s, idx);
        TEST_ASSERT_EQUAL_INT8(expect_vals[idx], actual_val);
    }
}

// Test cases for staSensorFastPollToggle
TEST(SensorFastPoll, ToggleActuatorOnOff) {
    gMonStatus status;
    // Initially no fast poll enabled
    TEST_ASSERT_EQUAL(0x0, test_soil_sensor_meta.fast_poll.enabled[0]);
    // all sensors should be polling at default rate
    TEST_ASSERT_EQUAL(10000, staSensorReadInterval(&test_soil_sensor_meta));
    const char enabled_bits_allone[] = {1, 1, 1, 1};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_allone);

    // ---- sub-case 1 , Set actuator to ON status ----
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_ON;
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // sensors 0 and 2 are polling at faster rate
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    const char enabled_bits_02on[] = {1, 0, 1, 0};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);

    // ---- sub-case 2 , Test with another actuator mask, should OR ----
    gMonActuator_t another_actuator = {0};
    another_actuator.sensor_id_mask = (1 << 1); // Mask for sensor 1
    another_actuator.status = GMON_OUT_DEV_STATUS_ON;
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &another_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // sensors 0, 1, and 2 are polling at faster rate
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    const char enabled_bits_012on[] = {1, 1, 1, 0};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_012on);
    TEST_ASSERT_EQUAL(5, test_soil_sensor_meta.fast_poll._div_cnt);

    // ---- sub-case 3 , Set actuator to OFF status ----
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_OFF;
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    const char enabled_bits_1on[] = {0, 1, 0, 0};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_1on);

    // ---- sub-case 4 , Test turning off a mask that wasn't enabled ----
    another_actuator.sensor_id_mask = (1 << 5); // Mask for sensor 6 (unknown)
    another_actuator.status = GMON_OUT_DEV_STATUS_OFF;
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &another_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // only sensors 1 is polling at faster rate
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_1on);
    TEST_ASSERT_EQUAL(5, test_soil_sensor_meta.fast_poll._div_cnt);

    // ---- sub-case 5, turn off with another actuator mask ----
    another_actuator.sensor_id_mask = (1 << 1); // Mask for sensor 1
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &another_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // all sensors go back to default polling rate
    TEST_ASSERT_EQUAL(10000, staSensorReadInterval(&test_soil_sensor_meta));
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_allone);
}

TEST(SensorFastPoll, ToggleActuatorNoChange) {
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_ON;
    gMonStatus status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // sensors 0 and 2 are polling at faster rate
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    const char enabled_bits_02on[] = {1, 0, 1, 0};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);

    test_pump_actuator.status = GMON_OUT_DEV_STATUS_PAUSE;
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);

    test_pump_actuator.status = GMON_OUT_DEV_STATUS_BROKEN;
    status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);
}

TEST(SensorFastPoll, ToggleInvalidArgs) {
    gMonStatus status;
    // Null s_meta
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_ON;
    status = staSensorFastPollToggle(NULL, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    // Null actuator
    status = staSensorFastPollToggle(&test_soil_sensor_meta, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
}

TEST(SensorFastPoll, RefreshFastPollNoActive) {
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_OFF;
    gMonStatus status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

    status = staSensorRefreshFastPollRatio(&test_soil_sensor_meta);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    // all sensors still polling at default rate
    TEST_ASSERT_EQUAL(10000, staSensorReadInterval(&test_soil_sensor_meta));
    const char enabled_bits_allone[] = {1, 1, 1, 1};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_allone);
}

TEST(SensorFastPoll, RefreshFastPollCountdown) {
    const char enabled_bits_02on[] = {1, 0, 1, 0};
    const char enabled_bits_allone[] = {1, 1, 1, 1};
    test_pump_actuator.status = GMON_OUT_DEV_STATUS_ON;
    gMonStatus status = staSensorFastPollToggle(&test_soil_sensor_meta, &test_pump_actuator);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);

    // reset internal counter to divisor, and then decrement until 1
    for (char idx = 0; idx < 4; idx++) {
        status = staSensorRefreshFastPollRatio(&test_soil_sensor_meta);
        TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
        // internal counter goes like 4, 3, 2, 1
        TEST_ASSERT_EQUAL(10000 / 5, staSensorReadInterval(&test_soil_sensor_meta));
        TEST_ASSERT_EQUAL(4 - idx, test_soil_sensor_meta.fast_poll._div_cnt);
        ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);
    }
    // Countdown to 0
    status = staSensorRefreshFastPollRatio(&test_soil_sensor_meta);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_allone);

    // After reaching 0, next call should reset to divisor and decrement
    status = staSensorRefreshFastPollRatio(&test_soil_sensor_meta);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(4, test_soil_sensor_meta.fast_poll._div_cnt);
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);
}

// Test cases for staSensorPollEnabled
TEST(SensorFastPoll, PollEnabledCycle) {
    test_soil_sensor_meta.fast_poll.enabled[0] = (1 << 0); // Sensor 0 enabled, sensor 1 not
    test_soil_sensor_meta.fast_poll._div_cnt = 0;          // Full poll cycle (divisor reached 0)
    // All sensors should be enabled
    const char enabled_bits_allone[] = {1, 1, 1, 1};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_allone);

    test_soil_sensor_meta.fast_poll.enabled[0] = (1 << 0) | (1 << 3); // Sensor 0 and 3 enabled
    test_soil_sensor_meta.fast_poll._div_cnt = 1;                     // Not a full poll cycle
    // Only sensors specified by mask should be enabled
    const char enabled_bits_02on[] = {1, 0, 0, 1};
    ut_verify_poll_enabled(&test_soil_sensor_meta, 4, enabled_bits_02on);
}

TEST(SensorFastPoll, PollEnabledInvalidArgs) {
    // Null s_meta
    TEST_ASSERT_EQUAL(0, staSensorPollEnabled(NULL, 0));
    // Invalid index (idx >= num_items)
    test_soil_sensor_meta.super.num_items = 2; // Only 0 and 1 are valid
    test_soil_sensor_meta.fast_poll.enabled[0] = (1 << 0);
    test_soil_sensor_meta.fast_poll._div_cnt = 3;
    TEST_ASSERT_EQUAL(0, staSensorPollEnabled(&test_soil_sensor_meta, 2));
}

TEST_GROUP_RUNNER(gMonSoilSensor) {
    RUN_TEST_CASE(SensorFastPoll, ToggleActuatorOnOff);
    RUN_TEST_CASE(SensorFastPoll, ToggleActuatorNoChange);
    RUN_TEST_CASE(SensorFastPoll, ToggleInvalidArgs);
    RUN_TEST_CASE(SensorFastPoll, RefreshFastPollNoActive);
    RUN_TEST_CASE(SensorFastPoll, RefreshFastPollCountdown);
    RUN_TEST_CASE(SensorFastPoll, PollEnabledCycle);
    RUN_TEST_CASE(SensorFastPoll, PollEnabledInvalidArgs);
}
