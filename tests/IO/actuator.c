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
    gMonStatus status = staSetTrigThresholdPump(&pump.param, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(new_val, pump.param.threshold);
}

TEST(UpdateThresholdPump, ThresholdAtMinBoundary) {
    gMonActuator_t pump = {0};
    unsigned int   new_val = GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP;
    gMonStatus     status = staSetTrigThresholdPump(&pump.param, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(new_val, pump.param.threshold);
}

TEST(UpdateThresholdPump, ThresholdAtMaxBoundary) {
    gMonActuator_t pump = {0};
    unsigned int   new_val = GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP;
    gMonStatus     status = staSetTrigThresholdPump(&pump.param, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(new_val, pump.param.threshold);
}

TEST(UpdateThresholdPump, ThresholdBelowMin) {
    // Set an initial threshold to check it doesn't change
    gMonActuator_t pump = {.param = {.threshold = 50}};
    unsigned int   new_val = GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP - 1;
    gMonStatus     status = staSetTrigThresholdPump(&pump.param, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(50, pump.param.threshold); // Should remain unchanged
}

TEST_GROUP(MeasureWorkingTime);

TEST_SETUP(MeasureWorkingTime) {}

TEST_TEAR_DOWN(MeasureWorkingTime) {}

TEST(MeasureWorkingTime, OffToOn) {
    unsigned int       time_elapsed_ms = 10;
    gMonActuator_t     ator = {.status = GMON_OUT_DEV_STATUS_OFF, .param = {.max_worktime = 100}};
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(10, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, OffStaysOffMaxWorktimeZero) {
    unsigned int       time_elapsed_ms = 10;
    gMonActuator_t     ator = {.status = GMON_OUT_DEV_STATUS_OFF, .param = {.max_worktime = 0}};
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, new_status);
    TEST_ASSERT_EQUAL(0, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, OnStaysOn) {
    unsigned int   time_elapsed_ms = 20;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_ON,
        .curr_worktime = 10,
        .curr_resttime = 0,
        .param = {.max_worktime = 100}
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(30, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, OnToPauseExactTime) {
    unsigned int   time_elapsed_ms = 10;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_ON,
        .curr_worktime = 90,
        .curr_resttime = 0,
        .param = {.max_worktime = 100}
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_PAUSE, new_status);
    TEST_ASSERT_EQUAL(0, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, OnToPauseOverTime) {
    unsigned int   time_elapsed_ms = 20;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_ON,
        .curr_worktime = 90,
        .curr_resttime = 0,
        .param = {.max_worktime = 100}
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_PAUSE, new_status);
    TEST_ASSERT_EQUAL(0, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, PauseStaysPause) {
    unsigned int   time_elapsed_ms = 20;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_PAUSE,
        .param = {.min_resttime = 50},
        .curr_worktime = 0,
        .curr_resttime = 10
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_PAUSE, new_status);
    TEST_ASSERT_EQUAL(0, ator.curr_worktime);
    TEST_ASSERT_EQUAL(30, ator.curr_resttime);
}

TEST(MeasureWorkingTime, PauseToOnExactTime) {
    unsigned int   time_elapsed_ms = 10;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_PAUSE,
        .param = {.min_resttime = 50},
        .curr_worktime = 0,
        .curr_resttime = 40
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(0, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, PauseToOnOverTime) {
    unsigned int   time_elapsed_ms = 20;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_PAUSE,
        .param = {.min_resttime = 50},
        .curr_worktime = 0,
        .curr_resttime = 40,
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_ON, new_status);
    TEST_ASSERT_EQUAL(0, ator.curr_worktime);
    TEST_ASSERT_EQUAL(0, ator.curr_resttime);
}

TEST(MeasureWorkingTime, BrokenToOff) {
    unsigned int   time_elapsed_ms = 10;
    gMonActuator_t ator = {
        .status = GMON_OUT_DEV_STATUS_BROKEN,
        .curr_worktime = 70,
        .curr_resttime = 30,
        .param = {.max_worktime = 100, .min_resttime = 50}
    };
    gMonActuatorStatus new_status = staActuatorMeasureWorkingTime(&ator, time_elapsed_ms);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, new_status);
    TEST_ASSERT_EQUAL(70, ator.curr_worktime);
    TEST_ASSERT_EQUAL(30, ator.curr_resttime);
}

TEST(UpdateThresholdPump, ThresholdAboveMax) {
    gMonActuator_t pump = {.param = {.threshold = 50}};
    unsigned int   new_val = GMON_MAX_ACTUATOR_TRIG_THRESHOLD_PUMP + 1;
    gMonStatus     status = staSetTrigThresholdPump(&pump.param, new_val);
    TEST_ASSERT_EQUAL(GMON_RESP_INVALID_REQ, status);
    TEST_ASSERT_EQUAL(50, pump.param.threshold); // Should remain unchanged
}

TEST_GROUP(AggregateU32);

TEST_SETUP(AggregateU32) {}

TEST_TEAR_DOWN(AggregateU32) {}

TEST(AggregateU32, NullPointerZeroSensor) {
    unsigned int   event_data[] = {10, 20};
    gmonEvent_t    evt = {.num_active_sensors = 0, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {.param = {.sensor_id_mask = 0}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateU32(NULL, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateU32(&evt, NULL, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateU32(&evt, &ator, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    value = 100;
    status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status); // `num_active_sensors` == 0
    TEST_ASSERT_EQUAL(100, value);
    TEST_ASSERT_EQUAL(0, ator.ema.last_aggregated);
    evt.num_active_sensors = 2;
    value = 123;
    status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status); // `sensor_id_mask` == 0x00
    TEST_ASSERT_EQUAL(123, value);
    TEST_ASSERT_EQUAL(0, ator.ema.last_aggregated);
}

TEST(AggregateU32, FirstAggregationAllRelevant) {
    unsigned int   event_data[] = {10, 20, 30, 40, 50};
    gmonEvent_t    evt = {.num_active_sensors = 5, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0
    };
    int        value = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(30, value);
    TEST_ASSERT_EQUAL(30, ator.ema.last_aggregated);
}

TEST(AggregateU32, SubsequentAggregationAllRelevant) {
    unsigned int   event_data[] = {20, 30, 40, 50, 60, 70, 80, 90};
    gmonEvent_t    evt = {.num_active_sensors = 8, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11111111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 30
    };
    int        value = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(42, value);
    TEST_ASSERT_EQUAL(42, ator.ema.last_aggregated);
}

TEST(AggregateU32, SomeIrrelevantSensors) {
#define NUM_EVENT_DATA 8
    unsigned int   event_data[] = {10, 20, 30, 40, 50, 60, 70, 80};
    gmonEvent_t    evt = {.num_active_sensors = NUM_EVENT_DATA, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b10101010}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0
    };
    int        value = 0, idx = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(50, value);
    TEST_ASSERT_EQUAL(50, ator.ema.last_aggregated);
    for (idx = 0; idx < NUM_EVENT_DATA; idx++)
        event_data[idx] += 2;
    ator.param.sensor_id_mask = 0b01010101;
    status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(46, value);
    TEST_ASSERT_EQUAL(46, ator.ema.last_aggregated);
#undef NUM_EVENT_DATA
}

TEST(AggregateU32, AllRelevantSensorsCorrupted) {
    unsigned int   event_data[] = {10, 20, 30};
    gmonEvent_t    evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0b111};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 100
    };
    int        value = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(100, ator.ema.last_aggregated);
}

TEST_GROUP(ShutdownAllActuators);

TEST_SETUP(ShutdownAllActuators) {}

TEST_TEAR_DOWN(ShutdownAllActuators) {}

TEST(ShutdownAllActuators, NullMonitorPointer) {
    gMonStatus status = staEmergencyShutdownAllActuators(NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
}

TEST(ShutdownAllActuators, SuccessAllActuatorsOff) {
    gMonActuator_t ators[3] = {
        {.status = GMON_OUT_DEV_STATUS_ON},
        {.status = GMON_OUT_DEV_STATUS_PAUSE},
        {.status = GMON_OUT_DEV_STATUS_ON},
    };
    // Set initial statuses to ON to verify they change to OFF
    gardenMonitor_t gmon =
        {.actuator = {
             .pump = {.count = 1, .entries = &ators[0]},
             .fan = {.count = 1, .entries = &ators[1]},
             .bulb = {.count = 1, .entries = &ators[2]},
         }};
    // All staTurnOffActuator calls will succeed due to the mock of staPlatformWritePin
    gMonStatus status = staEmergencyShutdownAllActuators(&gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Verify that all actuators' status are set to OFF
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, gmon.actuator.pump.entries[0].status);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, gmon.actuator.fan.entries[0].status);
    TEST_ASSERT_EQUAL(GMON_OUT_DEV_STATUS_OFF, gmon.actuator.bulb.entries[0].status);
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
    gMonActuator_t ator = {.param = {.sensor_id_mask = 0b1}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    gMonStatus     status = staActuatorAggregateAirCond(NULL, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateAirCond(&evt, NULL, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
    status = staActuatorAggregateAirCond(&evt, &ator, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, status);
}

TEST(AggregateAirCond, MalformedData) {
    gmonAirCond_t  event_data[] = {{20.f, 50.f}};
    gmonEvent_t    evt = {.num_active_sensors = 1, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {.param = {.sensor_id_mask = 0b1}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0};
    int            value = 0;
    evt.num_active_sensors = 0;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
    evt.num_active_sensors = 1; // Reset for next assertion
    ator.param.sensor_id_mask = 0;
    status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(AggregateAirCond, SkipNoRelevantSensors) {
    gmonAirCond_t  event_data[] = {{20.f, 50.f}, {22.f, 55.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b00}, .ema.lambda_fixp = 50, .ema.last_aggregated = 12
    };
    int        value = 199; // Check if value is modified
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
    TEST_ASSERT_EQUAL(199, value);
    TEST_ASSERT_EQUAL(12, ator.ema.last_aggregated);
}

TEST(AggregateAirCond, SkipAllRelevantCorrupted) {
    gmonAirCond_t  event_data[] = {{20.f, 50.f}, {22.f, 55.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0b11};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11}, .ema.lambda_fixp = 50, .ema.last_aggregated = 10
    };
    int        value = 199;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(199, value);
    TEST_ASSERT_EQUAL(10, ator.ema.last_aggregated);
}

TEST(AggregateAirCond, SkipAggregatedValuesAreZero) {
    gmonAirCond_t  event_data[] = {{0.f, 0.f}, {0.f, 0.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11}, .ema.lambda_fixp = 50, .ema.last_aggregated = 10
    };
    int        value = 199;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(199, value);
    TEST_ASSERT_EQUAL(10, ator.ema.last_aggregated);
}

TEST(AggregateAirCond, FirstAggregationOk) {
    gmonAirCond_t  event_data[] = {{20.f, 40.f}, {22.f, 42.f}, {24.f, 44.f}};
    gmonEvent_t    evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0b000};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0
    };
    // Calculations (assuming constants):
    // avg.temporature = (20.f + 22.f + 24.f) / 3 = 66.f / 3 = 22.0f
    // avg.humidity = (40.f + 42.f + 44.f) / 3 = 126.f / 3 = 42.0f
    // norm_temp = (22.0f - -5.0f) * 100 / (80 - -5) = 27.0f * 100 / 85 = 31.76470588
    // norm_humid = (42.0f - 20) * 100 / (90 - 20) = 31.428571428
    // new_aircond = 0.55 * 31.76470588 + 0.45 * 31.428571428 = 31.6134453766 -> 31
    // expected_value = 31 (since last_aggregated is 0)
    int        value = 0, expected_value = 31;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, ator.ema.last_aggregated);
}

TEST(AggregateAirCond, SubsequentAggregationOk) {
    gmonAirCond_t  event_data[] = {{26.f, 60.f}, {26.f, 60.f}};
    gmonEvent_t    evt = {.num_active_sensors = 2, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11}, .ema.lambda_fixp = 50, .ema.last_aggregated = 50
    };
    // Calculations (assuming constants):
    // avg.temporature = 26.0f
    // avg.humidity = 60.0f
    // norm_temp = (26.0f - -5.0f) * 100 / (80 - -5) = 31.0f * 100 / 85 = 36.470588
    // norm_humid = (60.0f - 20.0f) * 100 / (90 - 20) = 40.0f * 100 / 70 = 57.142857
    // new_aircond = 0.55 * 36.470588 + 0.45 * 57.142857 = 20.058823 + 25.714285 = 45.773108 -> 45
    // ema: (50 * 45 + (100 - 50) * 50) / 100 = 47
    int        value = 0, expected_value = 47;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, ator.ema.last_aggregated);
}

TEST(AggregateAirCond, SingleRelevantUncorrupted) {
    gmonAirCond_t event_data[] = {{10.f, 40.f}, {15.f, 45.f}, {20.f, 50.f}};
    // Sensors 1 and 2 are corrupted
    gmonEvent_t evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0b0110};
    // All relevant, but 1 and 2 corrupted
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b0111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0
    };
    // Only sensor at idx 0 ({10.f, 40.f}) is relevant and uncorrupted.
    // avg.temporature = 10.0f
    // avg.humidity = 40.0f
    // norm_temp = (10.0f - -5.0f) * 100 / (80 - -5) = 15.0f * 100 / 85 = 17.647058... -> 17
    // norm_humid = (40.0f - 20.0f) * 100 / (90 - 20) = 20.0f * 100 / 70 = 28.571428... -> 28
    // new_aircond = 0.55 * 17.647058 + 0.45 * 28.571428 = 9.7058819 + 12.8571426 = 22.5630245 -> 22
    // expected_value = 22
    int        value = 0, expected_value = 22;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, ator.ema.last_aggregated);
}

TEST(AggregateAirCond, MixedRelevantCorrupted) {
    gmonAirCond_t event_data[] = {{10.f, 40.f}, {20.f, 50.f}, {30.f, 60.f},  {40.f, 70.f},
                                  {50.f, 80.f}, {60.f, 90.f}, {70.f, 100.f}, {80.f, 110.f}};
    gmonEvent_t   evt = {.num_active_sensors = 8, .data = event_data, .flgs.corruption = 0b01010101};
    // Sensors at idx 0, 2, 4, 6 are corrupted
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11111111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0
    };
    // Only sensors at idx 1 ({10.f, 50.f}), 3 ({40.f, 70.f}), 5 ({60.f, 90.f}),
    // 7 ({80.f, 110.f}) are relevant and uncorrupted.
    // avg.temporature = (20.f + 40.f + 60.f + 80.f) / 4 = 200.0f / 4 = 50.0f
    // avg.humidity = (50.f + 70.f + 90.f + 110.f) / 4 = 320.0f / 4 = 80.0f
    // norm_temp = (50.0f - -5.0f) * 100 / (80 - -5) = 55.0f * 100 / 85 = 64.705882
    // norm_humid = (80.0f - 20.0f) * 100 / (90 - 20) = 60.0f * 100 / 70 = 85.714285
    // new_aircond = 0.55 * 64.705882 + 0.45 * 85.714285 = 74.15966335 -> 74
    int        value = 0, expected_value = 74;
    gMonStatus status = staActuatorAggregateAirCond(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(expected_value, value);
    TEST_ASSERT_EQUAL(expected_value, ator.ema.last_aggregated);
}

TEST(AggregateU32, MixedRelevantCorrupted) {
    unsigned int   event_data[] = {10, 20, 30, 40, 50, 60, 70, 80};
    gmonEvent_t    evt = {.num_active_sensors = 8, .data = event_data, .flgs.corruption = 0b10101101};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b11111101}, .ema.lambda_fixp = 50, .ema.last_aggregated = 0
    };
    int        value = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(60, value);
    TEST_ASSERT_EQUAL(60, ator.ema.last_aggregated);
}

TEST(AggregateU32, RelevantUncorruptedAllZero) {
    unsigned int   event_data[] = {0, 0, 0};
    gmonEvent_t    evt = {.num_active_sensors = 3, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 100
    };
    int        value = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(100, ator.ema.last_aggregated);
}

TEST(AggregateU32, RelevantUncorruptedAvgZero) {
    unsigned int   event_data[] = {1, 0, 2, 0, 0, 2, 1};
    gmonEvent_t    evt = {.num_active_sensors = 7, .data = event_data, .flgs.corruption = 0};
    gMonActuator_t ator = {
        .param = {.sensor_id_mask = 0b01111111}, .ema.lambda_fixp = 50, .ema.last_aggregated = 100
    };
    int        value = 0;
    gMonStatus status = staActuatorAggregateU32(&evt, &ator, &value);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(100, ator.ema.last_aggregated);
}

TEST_GROUP(ActuatorUpdateParam);
TEST_SETUP(ActuatorUpdateParam) {}
TEST_TEAR_DOWN(ActuatorUpdateParam) {}

TEST(ActuatorUpdateParam, UpdateOk) {
    gMonActuatorParam_t old = {
        .threshold = GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP + 1,
        .max_worktime = 100,
        .min_resttime = 50,
        .sensor_id_mask = 0x1
    };
    gMonActuatorParam_t new = {
        .threshold = GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP + 7,
        .max_worktime = 200,
        .min_resttime = 60,
        .sensor_id_mask = 0x3
    };
    gMonStatus status = staActuatorUpdateParam(&old, &new, staSetTrigThresholdPump);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(GMON_MIN_ACTUATOR_TRIG_THRESHOLD_PUMP + 7, old.threshold);
    TEST_ASSERT_EQUAL(200, old.max_worktime);
    TEST_ASSERT_EQUAL(60, old.min_resttime);
    TEST_ASSERT_EQUAL(0x3, old.sensor_id_mask);
}

TEST_GROUP(ActuatorMemoryGrow);
TEST_SETUP(ActuatorMemoryGrow) {}
TEST_TEAR_DOWN(ActuatorMemoryGrow) {}

TEST(ActuatorMemoryGrow, InvalidArgs) {
    gMonActuators_t ator = {0};
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, staActuatorGrowSize(NULL, 5));
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, staActuatorGrowSize(&ator, 0));
}
TEST(ActuatorMemoryGrow, SeveralCallsOk) {
    gMonActuators_t ators = {0};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGrowSize(&ators, 2));
    TEST_ASSERT_EQUAL(2, ators.count);
    TEST_ASSERT_NOT_NULL(ators.entries);
    ators.entries[0].id = 8;
    ators.entries[1].id = 1;
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGrowSize(&ators, 5));
    TEST_ASSERT_EQUAL(5, ators.count);
    ators.entries[2].id = 7;
    ators.entries[3].id = 2;
    ators.entries[4].id = 9;
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGrowSize(&ators, 7));
    TEST_ASSERT_EQUAL(7, ators.count);
    ators.entries[5].id = 6;
    ators.entries[6].id = 3;
    gMonActuatorId_t expected_ids[] = {8, 1, 7, 2, 9, 6, 3};
    for (int i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL_UINT8(expected_ids[i], ators.entries[i].id);
    }
    XMEMFREE(ators.entries);
}

TEST_GROUP(ActuatorMemoryShrink);
TEST_SETUP(ActuatorMemoryShrink) {}
TEST_TEAR_DOWN(ActuatorMemoryShrink) {}
TEST(ActuatorMemoryShrink, InvalidArgs) {
    gMonActuators_t  ators = {0};
    gMonActuator_t   ator = {0};
    gMonActuatorId_t ids = {0};
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, staActuatorShrinkSize(NULL, 0, NULL));
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, staActuatorShrinkSize(&ators, 0, NULL));
    ators.count = 2;
    ators.entries = &ator;
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorShrinkSize(&ators, 1, &ids));
    ator.id = 6;
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorShrinkSize(&ators, 1, &ids));
}

TEST(ActuatorMemoryShrink, ShrinkPartialSuccess) {
    gMonActuators_t ators = {0};
    staActuatorGrowSize(&ators, 3);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[0], 4, 58));
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[1], 9, 23));
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[2], 7, 49));
    gMonActuatorId_t ids2rm_1[] = {9};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorShrinkSize(&ators, 2, ids2rm_1));
    TEST_ASSERT_EQUAL(2, ators.count);
    TEST_ASSERT_EQUAL(4, ators.entries[0].id);
    TEST_ASSERT_EQUAL(7, ators.entries[1].id);
    TEST_ASSERT_EQUAL(58, ators.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(49, ators.entries[1].ema.lambda_fixp);
    staActuatorGrowSize(&ators, 5);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[2], 10, 25));
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[3], 1, 106));
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[4], 2, 95));
    gMonActuatorId_t ids2rm_2[] = {10, 4};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorShrinkSize(&ators, 3, ids2rm_2));
    TEST_ASSERT_EQUAL(3, ators.count);
    TEST_ASSERT_EQUAL(7, ators.entries[0].id);
    TEST_ASSERT_EQUAL(1, ators.entries[1].id);
    TEST_ASSERT_EQUAL(2, ators.entries[2].id);
    TEST_ASSERT_EQUAL(49, ators.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(106, ators.entries[1].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(95, ators.entries[2].ema.lambda_fixp);
    staActuatorGrowSize(&ators, 5);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[3], 6, 40));
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorGenericInit(&ators.entries[4], 9, 35));
    gMonActuatorId_t ids2rm_3[] = {7, 2};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorShrinkSize(&ators, 3, ids2rm_3));
    TEST_ASSERT_EQUAL(3, ators.count);
    TEST_ASSERT_EQUAL(1, ators.entries[0].id);
    TEST_ASSERT_EQUAL(6, ators.entries[1].id);
    TEST_ASSERT_EQUAL(9, ators.entries[2].id);
    TEST_ASSERT_EQUAL(106, ators.entries[0].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(40, ators.entries[1].ema.lambda_fixp);
    TEST_ASSERT_EQUAL(35, ators.entries[2].ema.lambda_fixp);
    XMEMFREE(ators.entries);
}

TEST(ActuatorMemoryShrink, ShrinkFullSuccess) {
    gMonActuators_t ators = {0};
    staActuatorGrowSize(&ators, 3);
    ators.entries[0].id = 14;
    ators.entries[1].id = 19;
    ators.entries[2].id = 17;
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorShrinkSize(&ators, 0, NULL));
    TEST_ASSERT_EQUAL(0, ators.count);
    TEST_ASSERT_NULL(ators.entries);
}

TEST(ActuatorMemoryShrink, SkipIfSizeNotSmaller) {
    gMonActuators_t  ators = {0};
    gMonActuatorId_t ids = {0};
    staActuatorGrowSize(&ators, 2);
    TEST_ASSERT_EQUAL(2, ators.count);
    TEST_ASSERT_NOT_NULL(ators.entries);
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, staActuatorShrinkSize(&ators, 3, &ids));
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, staActuatorShrinkSize(&ators, 2, &ids));
    XMEMFREE(ators.entries);
}

TEST_GROUP(ActuatorAdjustSize);
TEST_SETUP(ActuatorAdjustSize) {}
TEST_TEAR_DOWN(ActuatorAdjustSize) {}

TEST(ActuatorAdjustSize, InvalidAndEdgeCases) {
    gMonActuator_t  entries[5] = {{.id = 1}, {.id = 2}, {.id = 3}, {.id = 4}, {.id = 5}};
    gMonActuators_t ators = {.entries = entries, .count = 5};
    // NULL pointers
    TEST_ASSERT_EQUAL(GMON_RESP_ERRARGS, staActuatorAdjustSize(NULL, NULL, NULL));
    // Both NULL ids -> Skip
    TEST_ASSERT_EQUAL(GMON_RESP_SKIP, staActuatorAdjustSize(&ators, NULL, NULL));
    // Zero ID in ids2add
    gMonActuatorId_t  add_ids[] = {0}, rm_ids[] = {0};
    gMonActuatorIds_t ids2add = {.id = add_ids, .count = 1}, ids2rm = {.id = rm_ids, .count = 1};
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorAdjustSize(&ators, &ids2add, NULL));
    // Zero ID in ids2rm
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorAdjustSize(&ators, NULL, &ids2rm));
    // Zero ID in ators
    add_ids[0] = 12;
    ators.entries[0].id = 0;
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorAdjustSize(&ators, &ids2add, NULL));
    ators.entries[0].id = 1; // Restore
    // ID in ids2rm not exist in ators
    rm_ids[0] = 199;
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorAdjustSize(&ators, NULL, &ids2rm));

    // Duplicate in ators
    ators.entries[0].id = 2; // Duplicate of entries[1].id
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorAdjustSize(&ators, &ids2add, NULL));
    ators.entries[0].id = 1; // Restore
    // Duplicate in ids2add
    gMonActuatorId_t  add_ids_dup[] = {6, 6};
    gMonActuatorIds_t ids2add_dup = {.id = add_ids_dup, .count = 2};
    TEST_ASSERT_EQUAL(GMON_RESP_ERR_NOT_SUPPORT, staActuatorAdjustSize(&ators, &ids2add_dup, NULL));
    // total actuators exceeding hard limit
    gMonActuatorId_t  add_ids_exceed[] = {20, 21, 22, 23};
    gMonActuatorIds_t ids2add_exceed = {.id = add_ids_exceed, .count = 4};
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, staActuatorAdjustSize(&ators, &ids2add_exceed, NULL));
}

TEST(ActuatorAdjustSize, SuccessfulSizeIncreaseNothingRemoved) {
    gMonActuatorId_t ids[5] = {3, 7, 8, 13, 16};
    gMonActuator_t  *entries = XMALLOC(5 * sizeof(gMonActuator_t));
    for (int i = 0; i < 5; i++)
        entries[i].id = ids[i];
    gMonActuators_t   ators = {.entries = entries, .count = 5};
    gMonActuatorId_t  add_ids[] = {12, 4};
    gMonActuatorIds_t ids2add = {.id = add_ids, .count = 2};
    gMonStatus        status = staActuatorAdjustSize(&ators, &ids2add, NULL);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL(7, ators.count);
    gMonActuatorId_t expected[] = {3, 7, 8, 13, 16, 12, 4};
    for (int i = 0; i < 7; i++)
        TEST_ASSERT_EQUAL(expected[i], ators.entries[i].id);
    XMEMFREE(ators.entries);
}

TEST(ActuatorAdjustSize, SuccessfulSizeIncreaseSomeRemovedSomeAdded) {
    gMonActuatorId_t ids[5] = {3, 7, 8, 13, 16};
    gMonActuator_t  *entries = XMALLOC(5 * sizeof(gMonActuator_t));
    for (int i = 0; i < 5; i++)
        entries[i].id = ids[i];
    gMonActuators_t   ators = {.entries = entries, .count = 5};
    gMonActuatorId_t  add_ids[] = {12, 9, 4}, rm_ids[] = {13, 7};
    gMonActuatorIds_t ids2add = {.id = add_ids, .count = 3}, ids2rm = {.id = rm_ids, .count = 2};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorAdjustSize(&ators, &ids2add, &ids2rm));
    TEST_ASSERT_EQUAL(6, ators.count);
    gMonActuatorId_t expected[] = {3, 9, 8, 12, 16, 4};
    for (int i = 0; i < 6; i++)
        TEST_ASSERT_EQUAL(expected[i], ators.entries[i].id);
    XMEMFREE(ators.entries);
}

TEST(ActuatorAdjustSize, SuccessfulSizeDecreaseNothingAdded) {
    gMonActuatorId_t ids[5] = {3, 7, 8, 13, 16};
    gMonActuator_t  *entries = XMALLOC(5 * sizeof(gMonActuator_t));
    for (int i = 0; i < 5; i++)
        entries[i].id = ids[i];
    gMonActuators_t   ators = {.entries = entries, .count = 5};
    gMonActuatorId_t  rm_ids[] = {13, 7};
    gMonActuatorIds_t ids2rm = {.id = rm_ids, .count = 2};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorAdjustSize(&ators, NULL, &ids2rm));
    TEST_ASSERT_EQUAL(3, ators.count);
    gMonActuatorId_t expected[] = {3, 8, 16};
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_EQUAL(expected[i], ators.entries[i].id);
    XMEMFREE(ators.entries);
}

TEST(ActuatorAdjustSize, SuccessfulSizeDecreaseSomeRemovedSomeAdded) {
    gMonActuatorId_t ids[5] = {3, 7, 8, 13, 16};
    gMonActuator_t  *entries = XMALLOC(5 * sizeof(gMonActuator_t));
    for (int i = 0; i < 5; i++)
        entries[i].id = ids[i];
    gMonActuators_t   ators = {.entries = entries, .count = 5};
    gMonActuatorId_t  add_ids[] = {11}, rm_ids[] = {13, 7};
    gMonActuatorIds_t ids2add = {.id = add_ids, .count = 1}, ids2rm = {.id = rm_ids, .count = 2};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorAdjustSize(&ators, &ids2add, &ids2rm));
    TEST_ASSERT_EQUAL(4, ators.count);
    gMonActuatorId_t expected[] = {3, 8, 11, 16};
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_EQUAL(expected[i], ators.entries[i].id);
    XMEMFREE(ators.entries);
}

TEST(ActuatorAdjustSize, SuccessWithSizeUnchangedSomeRemovedSomeAdded) {
    gMonActuatorId_t ids[5] = {3, 7, 8, 13, 16};
    gMonActuator_t  *entries = XMALLOC(5 * sizeof(gMonActuator_t));
    for (int i = 0; i < 5; i++)
        entries[i].id = ids[i];
    gMonActuators_t   ators = {.entries = entries, .count = 5};
    gMonActuatorId_t  add_ids[] = {11, 5}, rm_ids[] = {13, 7};
    gMonActuatorIds_t ids2add = {.id = add_ids, .count = 2}, ids2rm = {.id = rm_ids, .count = 2};
    TEST_ASSERT_EQUAL(GMON_RESP_OK, staActuatorAdjustSize(&ators, &ids2add, &ids2rm));
    TEST_ASSERT_EQUAL(5, ators.count);
    gMonActuatorId_t expected[] = {3, 5, 8, 11, 16};
    for (int i = 0; i < 5; i++)
        TEST_ASSERT_EQUAL(expected[i], ators.entries[i].id);
    XMEMFREE(ators.entries);
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
    RUN_TEST_CASE(ShutdownAllActuators, NullMonitorPointer);
    RUN_TEST_CASE(ShutdownAllActuators, SuccessAllActuatorsOff);
    RUN_TEST_CASE(ActuatorUpdateParam, UpdateOk);
    RUN_TEST_CASE(ActuatorMemoryGrow, InvalidArgs);
    RUN_TEST_CASE(ActuatorMemoryGrow, SeveralCallsOk);
    RUN_TEST_CASE(ActuatorMemoryShrink, InvalidArgs);
    RUN_TEST_CASE(ActuatorMemoryShrink, ShrinkPartialSuccess);
    RUN_TEST_CASE(ActuatorMemoryShrink, ShrinkFullSuccess);
    RUN_TEST_CASE(ActuatorMemoryShrink, SkipIfSizeNotSmaller);
    RUN_TEST_CASE(ActuatorAdjustSize, InvalidAndEdgeCases);
    RUN_TEST_CASE(ActuatorAdjustSize, SuccessfulSizeIncreaseNothingRemoved);
    RUN_TEST_CASE(ActuatorAdjustSize, SuccessfulSizeIncreaseSomeRemovedSomeAdded);
    RUN_TEST_CASE(ActuatorAdjustSize, SuccessfulSizeDecreaseNothingAdded);
    RUN_TEST_CASE(ActuatorAdjustSize, SuccessfulSizeDecreaseSomeRemovedSomeAdded);
    RUN_TEST_CASE(ActuatorAdjustSize, SuccessWithSizeUnchangedSomeRemovedSomeAdded);
}
