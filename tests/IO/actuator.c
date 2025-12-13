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

TEST_GROUP(AggregateU32);

TEST_SETUP(AggregateU32) {}

TEST_TEAR_DOWN(AggregateU32) {}

TEST(AggregateU32, NullPointerZeroSensor) {
    unsigned int   event_data[] = {10, 20};
    gmonEvent_t    evt = {.num_active_sensors = 0, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(NULL, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateU32(&evt, NULL, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateU32(&evt, &dev, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    value = 100;
    status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status); // `num_active_sensors` == 0
    TEST_ASSERT_EQUAL(100, value);
    TEST_ASSERT_EQUAL(0, dev.ema.last_aggregated);
    evt.num_active_sensors = 2;
    value = 123;
    status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status); // `sensor_id_mask` == 0x00
    TEST_ASSERT_EQUAL(123, value);
    TEST_ASSERT_EQUAL(0, dev.ema.last_aggregated);
}

TEST(AggregateU32, FirstAggregationAllRelevant) {
    unsigned int   event_data[] = {10, 20, 30, 40, 50};
    gmonEvent_t    evt = {.num_active_sensors = 5, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b11111, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(30, value);
    TEST_ASSERT_EQUAL(30, dev.ema.last_aggregated);
}

TEST(AggregateU32, SubsequentAggregationAllRelevant) {
    unsigned int   event_data[] = {20, 30, 40, 50, 60, 70, 80, 90};
    gmonEvent_t    evt = {.num_active_sensors = 8, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b11111111, .ema.lambda_fixp = 50, .ema.last_aggregated = 30};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(42, value);
    TEST_ASSERT_EQUAL(42, dev.ema.last_aggregated);
}

TEST(AggregateU32, SomeIrrelevantSensors) {
#define NUM_EVENT_DATA 8
    unsigned int   event_data[] = {10, 20, 30, 40, 50, 60, 70, 80};
    gmonEvent_t    evt = {.num_active_sensors = NUM_EVENT_DATA, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b10101010, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0, idx = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(50, value);
    TEST_ASSERT_EQUAL(50, dev.ema.last_aggregated);
    for (idx = 0; idx < NUM_EVENT_DATA; idx++)
        event_data[idx] += 2;
    dev.sensor_id_mask = 0b01010101;
    status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(46, value);
    TEST_ASSERT_EQUAL(46, dev.ema.last_aggregated);
#undef NUM_EVENT_DATA
}

TEST(AggregateU32, AllRelevantSensorsCorrupted) {
    unsigned int   event_data[] = {10, 20, 30};
    gmonEvent_t    evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0b111};
    gMonActuator_t dev = {.sensor_id_mask = 0b111, .ema.lambda_fixp = 50, .ema.last_aggregated = 100};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(100, dev.ema.last_aggregated);
}

TEST_GROUP(AggregateAirCond);

TEST_SETUP(AggregateAirCond) {}

TEST_TEAR_DOWN(AggregateAirCond) {}

// Assuming the following constants are defined globally or via station_include.h for calculation:
// - GMON_MIN_AIR_TEMPERATURE
// - GMON_MAX_AIR_TEMPERATURE
// - GMON_MIN_AIR_HUMIDITY_SUPPORTED
// - GMON_MAX_AIR_HUMIDITY_SUPPORTED

TEST(AggregateAirCond, NullPointer) {
    gmonAirCond_t  dummy_event_data[] = {{0.f, 0.f}};
    gmonEvent_t    evt = {.num_active_sensors = 1, .data = dummy_event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b1, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateAirCond(NULL, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateAirCond(&evt, NULL, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateAirCond(&evt, &dev, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
}

TEST(AggregateAirCond, MalformedData) {
    gmonAirCond_t  event_data[] = {{20.f, 50.f}};
    gmonEvent_t    evt = {.num_active_sensors = 1, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b1, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    evt.num_active_sensors = 0;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
    evt.num_active_sensors = 1; // Reset for next assertion
    dev.sensor_id_mask = 0;
    status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(AggregateAirCond, SkipNoRelevantSensors) {
    gmonAirCond_t  event_data[] = {{20.f, 50.f}, {22.f, 55.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b00, .ema.lambda_fixp = 50, .ema.last_aggregated = 12};
    int            value = 999; // Check if value is modified
    gMonStatus     status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
    TEST_ASSERT_EQUAL(999, value);
    TEST_ASSERT_EQUAL(12, dev.ema.last_aggregated);
}

TEST(AggregateAirCond, SkipAllRelevantCorrupted) {
    gmonAirCond_t  event_data[] = {{20.f, 50.f}, {22.f, 55.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0b11};
    gMonActuator_t dev = {.sensor_id_mask = 0b11, .ema.lambda_fixp = 50, .ema.last_aggregated = 10};
    int            value = 999;
    gMonStatus     status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(999, value);
    TEST_ASSERT_EQUAL(10, dev.ema.last_aggregated);
}

TEST(AggregateAirCond, SkipAggregatedValuesAreZero) {
    gmonAirCond_t  event_data[] = {{0.f, 0.f}, {0.f, 0.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b11, .ema.lambda_fixp = 50, .ema.last_aggregated = 10};
    int            value = 999;
    gMonStatus     status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(999, value);
    TEST_ASSERT_EQUAL(10, dev.ema.last_aggregated);
}

TEST(AggregateAirCond, FirstAggregationOk) {
    gmonAirCond_t  event_data[] = {{20.f, 40.f}, {22.f, 42.f}, {24.f, 44.f}};
    gmonEvent_t    evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0b000};
    gMonActuator_t dev = {.sensor_id_mask = 0b111, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    // Calculations (assuming constants):
    // avg.temporature = (20.f + 22.f + 24.f) / 3 = 66.f / 3 = 22.0f
    // avg.humidity = (40.f + 42.f + 44.f) / 3 = 126.f / 3 = 42.0f
    // norm_temp = (22.0f - -5.0f) * 100 / (80 - -5) = 27.0f * 100 / 85 = 31.76470588
    // norm_humid = (42.0f - 20) * 100 / (90 - 20) = 31.428571428
    // new_aircond = 0.55 * 31.76470588 + 0.45 * 31.428571428 = 31.6134453766 -> 31
    // expected_value = 31 (since last_aggregated is 0)
    int        value = 0, expected_value = 31;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, dev.ema.last_aggregated);
}

TEST(AggregateAirCond, SubsequentAggregationOk) {
    gmonAirCond_t  event_data[] = {{26.f, 60.f}, {26.f, 60.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b11, .ema.lambda_fixp = 50, .ema.last_aggregated = 50};
    // Calculations (assuming constants):
    // avg.temporature = 26.0f
    // avg.humidity = 60.0f
    // norm_temp = (26.0f - -5.0f) * 100 / (80 - -5) = 31.0f * 100 / 85 = 36.470588
    // norm_humid = (60.0f - 20.0f) * 100 / (90 - 20) = 40.0f * 100 / 70 = 57.142857
    // new_aircond = 0.55 * 36.470588 + 0.45 * 57.142857 = 20.058823 + 25.714285 = 45.773108 -> 45
    // ema: (50 * 45 + (100 - 50) * 50) / 100 = 47
    int        value = 0, expected_value = 47;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, dev.ema.last_aggregated);
}

TEST(AggregateAirCond, SingleRelevantUncorrupted) {
    gmonAirCond_t event_data[] = {{10.f, 40.f}, {15.f, 45.f}, {20.f, 50.f}};
    // Sensors 1 and 2 are corrupted
    gmonEvent_t evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0b0110};
    // All relevant, but 1 and 2 corrupted
    gMonActuator_t dev = {.sensor_id_mask = 0b0111, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    // Only sensor at idx 0 ({10.f, 40.f}) is relevant and uncorrupted.
    // avg.temporature = 10.0f
    // avg.humidity = 40.0f
    // norm_temp = (10.0f - -5.0f) * 100 / (80 - -5) = 15.0f * 100 / 85 = 17.647058... -> 17
    // norm_humid = (40.0f - 20.0f) * 100 / (90 - 20) = 20.0f * 100 / 70 = 28.571428... -> 28
    // new_aircond = 0.55 * 17.647058 + 0.45 * 28.571428 = 9.7058819 + 12.8571426 = 22.5630245 -> 22
    // expected_value = 22
    int        value = 0, expected_value = 22;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, dev.ema.last_aggregated);
}

TEST(AggregateAirCond, MixedRelevantCorrupted) {
    gmonAirCond_t event_data[] = {{10.f, 40.f}, {20.f, 50.f}, {30.f, 60.f},  {40.f, 70.f},
                                  {50.f, 80.f}, {60.f, 90.f}, {70.f, 100.f}, {80.f, 110.f}};
    gmonEvent_t   evt = {.num_active_sensors = 8, .data = event_data, .flgs.corruption = 0b01010101};
    // Sensors at idx 0, 2, 4, 6 are corrupted
    gMonActuator_t dev = {.sensor_id_mask = 0b11111111, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    // Only sensors at idx 1 ({10.f, 50.f}), 3 ({40.f, 70.f}), 5 ({60.f, 90.f}),
    // 7 ({80.f, 110.f}) are relevant and uncorrupted.
    // avg.temporature = (20.f + 40.f + 60.f + 80.f) / 4 = 200.0f / 4 = 50.0f
    // avg.humidity = (50.f + 70.f + 90.f + 110.f) / 4 = 320.0f / 4 = 80.0f
    // norm_temp = (50.0f - -5.0f) * 100 / (80 - -5) = 55.0f * 100 / 85 = 64.705882
    // norm_humid = (80.0f - 20.0f) * 100 / (90 - 20) = 60.0f * 100 / 70 = 85.714285
    // new_aircond = 0.55 * 64.705882 + 0.45 * 85.714285 = 74.15966335 -> 74
    int        value = 0, expected_value = 74;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, dev.ema.last_aggregated);
}

TEST(AggregateU32, MixedRelevantCorrupted) {
    unsigned int   event_data[] = {10, 20, 30, 40, 50, 60, 70, 80};
    gmonEvent_t    evt = {.num_active_sensors = 8, .data = event_data, .flgs.corruption = 0b10101101};
    gMonActuator_t dev = {.sensor_id_mask = 0b11111101, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(60, value);
    TEST_ASSERT_EQUAL(60, dev.ema.last_aggregated);
}

TEST(AggregateU32, RelevantUncorruptedAllZero) {
    unsigned int   event_data[] = {0, 0, 0};
    gmonEvent_t    evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b111, .ema.lambda_fixp = 50, .ema.last_aggregated = 100};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(100, dev.ema.last_aggregated);
}

TEST(AggregateU32, RelevantUncorruptedAvgZero) {
    unsigned int   event_data[] = {1, 0, 2, 0, 0, 2, 1};
    gmonEvent_t    evt = {.num_active_sensors = 7, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t dev = {.sensor_id_mask = 0b01111111, .ema.lambda_fixp = 50, .ema.last_aggregated = 100};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(&evt, &dev, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(100, dev.ema.last_aggregated);
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
    RUN_TEST_CASE(MeasureWorkingTime, PauseToOnOverTime);
    RUN_TEST_CASE(MeasureWorkingTime, BrokenToOff);
    RUN_TEST_CASE(AggregateU32, NullPointerZeroSensor);
    RUN_TEST_CASE(AggregateU32, FirstAggregationAllRelevant);
    RUN_TEST_CASE(AggregateU32, SubsequentAggregationAllRelevant);
    RUN_TEST_CASE(AggregateU32, SomeIrrelevantSensors);
    RUN_TEST_CASE(AggregateU32, AllRelevantSensorsCorrupted);
    RUN_TEST_CASE(AggregateU32, MixedRelevantCorrupted);
    RUN_TEST_CASE(AggregateU32, RelevantUncorruptedAllZero);
    RUN_TEST_CASE(AggregateU32, RelevantUncorruptedAvgZero);
    RUN_TEST_CASE(AggregateAirCond, NullPointer);
    RUN_TEST_CASE(AggregateAirCond, MalformedData);
    RUN_TEST_CASE(AggregateAirCond, SkipNoRelevantSensors);
    RUN_TEST_CASE(AggregateAirCond, SkipAllRelevantCorrupted);
    RUN_TEST_CASE(AggregateAirCond, SkipAggregatedValuesAreZero);
    RUN_TEST_CASE(AggregateAirCond, FirstAggregationOk);
    RUN_TEST_CASE(AggregateAirCond, SubsequentAggregationOk);
    RUN_TEST_CASE(AggregateAirCond, SingleRelevantUncorrupted);
    RUN_TEST_CASE(AggregateAirCond, MixedRelevantCorrupted);
}
