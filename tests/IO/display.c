#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

#define NUM_UTEST_SENSORS 2

gardenMonitor_t test_gmon = {0}; // Initialize all members to 0

TEST_GROUP(RenderPrintText);

TEST_SETUP(RenderPrintText) {
    gMonStatus status = staDisplayInit(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST_TEAR_DOWN(RenderPrintText) {
    gMonStatus status = staDisplayDeInit(&test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(RenderPrintText, InitOk) {
    gMonDisplayBlock_t *dblk;
    uint8_t             idx;
    // Expected block types in order of initialization in staDisplayInit
    const gMonBlockType_t expected_btypes[GMON_DISPLAY_NUM_PRINT_STRINGS] = {
        GMON_BLOCK_SENSOR_SOIL_RECORD,  // Index 0
        GMON_BLOCK_SENSOR_AIR_RECORD,   // Index 1
        GMON_BLOCK_SENSOR_LIGHT_RECORD, // Index 2
        GMON_BLOCK_ACTUATOR_THRESHOLD,  // Index 3
        GMON_BLOCK_ACTUATOR_STATUS,     // Index 4
        GMON_BLOCK_NETCONN_STATUS,      // Index 5
    };
    TEST_ASSERT_EQUAL(11, test_gmon.display.fonts[0].width);
    TEST_ASSERT_EQUAL(18, test_gmon.display.fonts[0].height);
    TEST_ASSERT_NOT_EQUAL(NULL, test_gmon.display.fonts[0].bitmap);
    // Verify scroll speed configuration
    TEST_ASSERT_EQUAL(4, test_gmon.display.config.scroll_speed);
    TEST_ASSERT_EQUAL(GMON_DISPLAY_NUM_PRINT_STRINGS, test_gmon.display.num_blocks);
    // Verify each display block
    // Sensor blocks (SOIL, AIR, LIGHT) have str.data == NULL and str.len == 0 after init.
    // Other blocks (THRESHOLD, ACTUATOR, NETCONN) have allocated data and non-zero length.
    for (idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        dblk = &test_gmon.display.blocks[idx];
        TEST_ASSERT_EQUAL(expected_btypes[idx], dblk->btype);
        TEST_ASSERT_NOT_NULL(dblk->render);
        TEST_ASSERT_EQUAL(&test_gmon.display.fonts[0], dblk->content.font);
        TEST_ASSERT_EQUAL((test_gmon.display.fonts[0].height + 2) * idx, dblk->content.posy);

        // Specific assertions for str.data and str.len based on block type
        switch (dblk->btype) {
        case GMON_BLOCK_SENSOR_SOIL_RECORD:
        case GMON_BLOCK_SENSOR_AIR_RECORD:
        case GMON_BLOCK_SENSOR_LIGHT_RECORD:
            TEST_ASSERT_NULL(dblk->content.str.data);
            TEST_ASSERT_EQUAL(0, dblk->content.str.len);
            break;
        default: // GMON_BLOCK_ACTUATOR_THRESHOLD, GMON_BLOCK_ACTUATOR_STATUS, GMON_BLOCK_NETCONN_STATUS
            TEST_ASSERT_NOT_NULL(dblk->content.str.data);
            TEST_ASSERT_GREATER_THAN(0, dblk->content.str.len);
            break;
        }
    }
} // end of TEST(RenderPrintText, InitOk)

TEST(RenderPrintText, ActuatorStateOk) {
    gMonDisplayBlock_t *ac_trig_blk = &test_gmon.display.blocks[GMON_BLOCK_ACTUATOR_THRESHOLD];
    // Expected string with padding as staUpdatePrintStrThreshold uses XMEMSET
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Thresold]: Pump: 0   , Fan: 0    , Bulb: 0   .", ac_trig_blk->content.str.data,
        ac_trig_blk->content.str.len
    );
    test_gmon.actuator.pump.threshold = 627;
    test_gmon.actuator.fan.threshold = 57;
    test_gmon.actuator.bulb.threshold = 8;
    gMonStatus status = ac_trig_blk->render(&ac_trig_blk->content, &test_gmon);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Thresold]: Pump: 627 , Fan: 57   , Bulb: 8   .", ac_trig_blk->content.str.data,
        ac_trig_blk->content.str.len
    );
}

TEST(RenderPrintText, SoilSensorLogOk) {
    // --- Variables for sensor data updates ---
    unsigned int test_soil_moist_data[] = {789, 1023, 556};
    gmonEvent_t  test_event = {0};
    // --- Test rendering for GMON_BLOCK_SENSOR_SOIL_RECORD (Index 0) ---
    gMonDisplayBlock_t *soil_block = &test_gmon.display.blocks[GMON_BLOCK_SENSOR_SOIL_RECORD];
    TEST_ASSERT_NOT_NULL(soil_block->render);
    // Update Soil Moisture
    test_event.event_type = GMON_EVENT_SOIL_MOISTURE_UPDATED;
    test_event.data = test_soil_moist_data;
    test_event.num_active_sensors = NUM_UTEST_SENSORS; // Indicate multiple active sensors
    gMonStatus status = soil_block->render(&soil_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Expected string without padding as staUpdatePrintStrSoilSensorData doesn't use XMEMSET for values
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Soil moisture: 789, 1023.", soil_block->content.str.data, soil_block->content.str.len
    );
    test_soil_moist_data[1] = 687;
    status = soil_block->render(&soil_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Soil moisture: 789, 687.", soil_block->content.str.data, soil_block->content.str.len
    );
    // modify number of sensors to trigger memory reallocation,
    test_event.num_active_sensors += 1;
    test_soil_moist_data[1] = 9966;
    status = soil_block->render(&soil_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Soil moisture: 789, 9966, 556.", soil_block->content.str.data,
        soil_block->content.str.len
    );
    test_event.num_active_sensors -= 1;
    test_soil_moist_data[0] = 3309;
    test_soil_moist_data[1] = 515;
    status = soil_block->render(&soil_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Soil moisture: 3309, 515.", soil_block->content.str.data, soil_block->content.str.len
    );
}

TEST(RenderPrintText, AirSensorLogOk) {
    gmonAirCond_t test_aircond_data[] = {
        {.temporature = 23.5f, .humidity = 90.5f},
        {.temporature = 25.0f, .humidity = 85.5f},
        {.temporature = 93.5f, .humidity = 89.f},
    };
    gmonEvent_t test_event = {0};
    // --- Test rendering for GMON_BLOCK_SENSOR_AIR_RECORD (Index 1) ---
    gMonDisplayBlock_t *air_block = &test_gmon.display.blocks[GMON_BLOCK_SENSOR_AIR_RECORD];
    TEST_ASSERT_NOT_NULL(air_block->render);
    // Update Air Temperature and Humidity
    test_event.event_type = GMON_EVENT_AIR_TEMP_UPDATED;
    test_event.data = test_aircond_data;
    test_event.num_active_sensors = NUM_UTEST_SENSORS;
    gMonStatus status = air_block->render(&air_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Expected string without padding
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Air Temp: 23.5'C, Air Humidity: 90.5, Air Temp: 25'C, Air Humidity: 85.5.",
        air_block->content.str.data, air_block->content.str.len
    );
    test_aircond_data[1].humidity = -999.5; // event data reach the limit
    status = air_block->render(&air_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Air Temp: 23.5'C, Air Humidity: 90.5, Air Temp: 25'C, Air Humidity: -999.5.",
        air_block->content.str.data, air_block->content.str.len
    );
    test_aircond_data[1].humidity = -1000.5; // event data exceeds the limit
    status = air_block->render(&air_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    test_aircond_data[0].humidity = 9959.5;
    test_aircond_data[1].humidity = 78.0;
    status = air_block->render(&air_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Air Temp: 23.5'C, Air Humidity: 9959.5, Air Temp: 25'C, Air Humidity: 78.",
        air_block->content.str.data, air_block->content.str.len
    );
    // modify number of sensors to trigger memory reallocation,
    test_event.num_active_sensors += 1;
    test_aircond_data[0].humidity = 88.5;
    test_aircond_data[1].temporature = 19.0;
    status = air_block->render(&air_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Air Temp: 23.5'C, Air Humidity: 88.5, Air Temp: 19'C, Air Humidity: 78,"
        " Air Temp: 93.5'C, Air Humidity: 89.",
        air_block->content.str.data, air_block->content.str.len
    );
    test_event.num_active_sensors -= 1;
    test_aircond_data[0].temporature = 21.5;
    test_aircond_data[1].temporature = 16.5;
    status = air_block->render(&air_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Air Temp: 21.5'C, Air Humidity: 88.5, Air Temp: 16.5'C, Air Humidity: 78.",
        air_block->content.str.data, air_block->content.str.len
    );
}

TEST(RenderPrintText, LightSensorLogOk) {
    unsigned int test_lightness_data[] = {1234, 567, 8901};
    gmonEvent_t  test_event = {0};
    // --- Test rendering for GMON_BLOCK_SENSOR_LIGHT_RECORD (Index 2) ---
    gMonDisplayBlock_t *light_block = &test_gmon.display.blocks[GMON_BLOCK_SENSOR_LIGHT_RECORD];
    TEST_ASSERT_NOT_NULL(light_block->render);
    // Update Lightness
    test_event.event_type = GMON_EVENT_LIGHTNESS_UPDATED;
    test_event.data = test_lightness_data;
    test_event.num_active_sensors = NUM_UTEST_SENSORS; // Indicate multiple active sensors
    gMonStatus status = light_block->render(&light_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    // Expected string without padding
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Lightness: 1234, 567.", light_block->content.str.data, light_block->content.str.len
    );
    test_lightness_data[0] = 9999; // event data reach the limit
    status = light_block->render(&light_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Lightness: 9999, 567.", light_block->content.str.data, light_block->content.str.len
    );
    test_lightness_data[1] = 10000; // event data exceeds the limit
    status = light_block->render(&light_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_ERRMEM, status);
    test_lightness_data[1] = 9998;
    status = light_block->render(&light_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Lightness: 9999, 9998.", light_block->content.str.data, light_block->content.str.len
    );
    // modify number of sensors to trigger memory reallocation,
    test_event.num_active_sensors += 1;
    test_lightness_data[0] = 4321;
    status = light_block->render(&light_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Sensor Log]: Lightness: 4321, 9998, 8901.", light_block->content.str.data,
        light_block->content.str.len
    );
}

TEST_GROUP_RUNNER(gMonDisplay) {
    RUN_TEST_CASE(RenderPrintText, InitOk);
    RUN_TEST_CASE(RenderPrintText, SoilSensorLogOk);
    RUN_TEST_CASE(RenderPrintText, AirSensorLogOk);
    RUN_TEST_CASE(RenderPrintText, LightSensorLogOk);
    RUN_TEST_CASE(RenderPrintText, ActuatorStateOk);
}
