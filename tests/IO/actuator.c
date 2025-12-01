#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

TEST_GROUP(UpdateThresholdPump);

TEST_SETUP(UpdateThresholdPump) {}

TEST_TEAR_DOWN(UpdateThresholdPump) {}

TEST(UpdateThresholdPump, NullPointer) {
    gMonStatus status = staSetTrigThresholdPump(NULL, 50);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
}

TEST(UpdateThresholdPump, ValidThresholdWithinRange) {
    gMonActuator_t pump = {0}; // Initialize all members to 0
    // Calculate a mid-range value for testing a valid threshold within bounds
    unsigned int new_val =
        (GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP + GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP) / 2;
    gMonStatus status = staSetTrigThresholdPump(&pump, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(new_val, pump.threshold);
}

TEST(UpdateThresholdPump, ThresholdAtMinBoundary) {
    gMonActuator_t pump = {0};
    unsigned int   new_val = GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP;
    gMonStatus     status = staSetTrigThresholdPump(&pump, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(new_val, pump.threshold);
}

TEST(UpdateThresholdPump, ThresholdAtMaxBoundary) {
    gMonActuator_t pump = {0};
    unsigned int   new_val = GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP;
    gMonStatus     status = staSetTrigThresholdPump(&pump, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(new_val, pump.threshold);
}

TEST(UpdateThresholdPump, ThresholdBelowMin) {
    gMonActuator_t pump = {.threshold = 50}; // Set an initial threshold to check it doesn't change
    unsigned int   new_val = GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP - 1;
    gMonStatus     status = staSetTrigThresholdPump(&pump, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(50, pump.threshold); // Should remain unchanged
}

TEST_GROUP(MeasureWorkingTime);

TEST_SETUP(MeasureWorkingTime) {}

TEST_TEAR_DOWN(MeasureWorkingTime) {}

TEST(MeasureWorkingTime, OffToOn) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_OFF, .max_worktime = 100, .curr_worktime = 0, .curr_resttime = 0
    };
    unsigned int       time_elapsed_ms = 10;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(10, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, OffStaysOffMaxWorktimeZero) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_OFF, .max_worktime = 0, .curr_worktime = 0, .curr_resttime = 0
    };
    unsigned int       time_elapsed_ms = 10;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, new_status);
    TEST_ASSERT_EQUAL(0, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, OnStaysOn) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_ON, .max_worktime = 100, .curr_worktime = 10, .curr_resttime = 0
    };
    unsigned int       time_elapsed_ms = 20;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(30, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, OnToPauseExactTime) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_ON, .max_worktime = 100, .curr_worktime = 90, .curr_resttime = 0
    };
    unsigned int       time_elapsed_ms = 10;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_PAUSE, new_status);
    TEST_ASSERT_EQUAL(0, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, OnToPauseOverTime) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_ON, .max_worktime = 100, .curr_worktime = 90, .curr_resttime = 0
    };
    unsigned int       time_elapsed_ms = 20;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_PAUSE, new_status);
    TEST_ASSERT_EQUAL(0, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, PauseStaysPause) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_PAUSE, .min_resttime = 50, .curr_worktime = 0, .curr_resttime = 10
    };
    unsigned int       time_elapsed_ms = 20;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_PAUSE, new_status);
    TEST_ASSERT_EQUAL(0, dev.curr_worktime);
    TEST_ASSERT_EQUAL(30, dev.curr_resttime);
}

TEST(MeasureWorkingTime, PauseToOnExactTime) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_PAUSE, .min_resttime = 50, .curr_worktime = 0, .curr_resttime = 40
    };
    unsigned int       time_elapsed_ms = 10;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(0, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, PauseToOnOverTime) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_PAUSE, .min_resttime = 50, .curr_worktime = 0, .curr_resttime = 40
    };
    unsigned int       time_elapsed_ms = 20;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(0, dev.curr_worktime);
    TEST_ASSERT_EQUAL(0, dev.curr_resttime);
}

TEST(MeasureWorkingTime, BrokenToOff) {
    gMonActuator_t dev = {
        .status = GMON_OUT_DEV_STATUS_BROKEN,
        .max_worktime = 100,
        .min_resttime = 50,
        .curr_worktime = 70,
        .curr_resttime = 30
    };
    unsigned int       time_elapsed_ms = 10;
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&dev, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, new_status);
    TEST_ASSERT_EQUAL(70, dev.curr_worktime);
    TEST_ASSERT_EQUAL(30, dev.curr_resttime);
}

TEST(UpdateThresholdPump, ThresholdAboveMax) {
    gMonActuator_t pump = {.threshold = 50};
    unsigned int   new_val = GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP + 1;
    gMonStatus     status = staSetTrigThresholdPump(&pump, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(50, pump.threshold); // Should remain unchanged
}

TEST_GROUP_RUNNER(gMonActuator) {
    RUN_TEST_CASE(UpdateThresholdPump, NullPointer);
    RUN_TEST_CASE(UpdateThresholdPump, ValidThresholdWithinRange);
    RUN_TEST_CASE(UpdateThresholdPump, ThresholdAtMinBoundary);
    RUN_TEST_CASE(UpdateThresholdPump, ThresholdAtMaxBoundary);
    RUN_TEST_CASE(UpdateThresholdPump, ThresholdBelowMin);
    RUN_TEST_CASE(UpdateThresholdPump, ThresholdAboveMax);
    RUN_TEST_CASE(MeasureWorkingTime, OffToOn);
    RUN_TEST_CASE(MeasureWorkingTime, OffStaysOffMaxWorktimeZero);
    RUN_TEST_CASE(MeasureWorkingTime, OnStaysOn);
    RUN_TEST_CASE(MeasureWorkingTime, OnToPauseExactTime);
    RUN_TEST_CASE(MeasureWorkingTime, OnToPauseOverTime);
    RUN_TEST_CASE(MeasureWorkingTime, PauseStaysPause);
    RUN_TEST_CASE(MeasureWorkingTime, PauseToOnExactTime);
    RUN_TEST_CASE(MeasureWorkingTime, PauseToOnOverTime);
    RUN_TEST_CASE(MeasureWorkingTime, BrokenToOff);
}
