#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

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
    gMonStatus status;
    gMonDisplayBlock_t *dblk;
    uint8_t idx;

    // Expected block types in order of initialization in staDisplayInit
    const gMonBlockType_t expected_btypes[GMON_DISPLAY_NUM_PRINT_STRINGS] = {
        GMON_BLOCK_SENSOR_RECORD,
        GMON_BLOCK_SENSOR_THRESHOLD,
        GMON_BLOCK_ACTUATOR_STATUS,
        GMON_BLOCK_NETCONN_STATUS,
    };
    TEST_ASSERT_EQUAL(11, test_gmon.display.fonts[0].width);
    TEST_ASSERT_EQUAL(18, test_gmon.display.fonts[0].height);
    TEST_ASSERT_NOT_EQUAL(NULL, test_gmon.display.fonts[0].bitmap);

    // Verify scroll speed configuration
    TEST_ASSERT_EQUAL(4, test_gmon.display.config.scroll_speed);

    // Verify each display block
    for (idx = 0; idx < GMON_DISPLAY_NUM_PRINT_STRINGS; idx++) {
        dblk = &test_gmon.display.blocks[idx];
        TEST_ASSERT_EQUAL(expected_btypes[idx], dblk->btype);
        TEST_ASSERT_NOT_NULL(dblk->content.str.data);
        TEST_ASSERT_GREATER_THAN(0, dblk->content.str.len);
        TEST_ASSERT_NOT_NULL(dblk->render);
        TEST_ASSERT_EQUAL(&test_gmon.display.fonts[0], dblk->content.font);
        TEST_ASSERT_EQUAL((test_gmon.display.fonts[0].height + 2) * idx, dblk->content.posy);
    }

    // --- Test rendering for GMON_BLOCK_SENSOR_RECORD ---
    gMonDisplayBlock_t *sensor_record_block = &test_gmon.display.blocks[GMON_BLOCK_SENSOR_RECORD];
    TEST_ASSERT_EQUAL(GMON_BLOCK_SENSOR_RECORD, sensor_record_block->btype);
    TEST_ASSERT_NOT_NULL(sensor_record_block->render);

    gmonEvent_t test_event = {0};

    // Test 1: Update Soil Moisture
    test_event.event_type = GMON_EVENT_SOIL_MOISTURE_UPDATED;
    test_event.data.soil_moist = 789;
    status = sensor_record_block->render(&sensor_record_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Last Record]: Soil moisture: 789 , Air Temp: 134.5'C, Air Humidity: 101, Lightness: 1001.",
        sensor_record_block->content.str.data,
        sensor_record_block->content.str.len
    );

    // Test 2: Update Air Temperature and Humidity
    test_event.event_type = GMON_EVENT_AIR_TEMP_UPDATED;
    test_event.data.air_cond.temporature = 23.5f;
    test_event.data.air_cond.humidity = 90.0f; // Expecting " 90" when formatted as 3-char float with 1 precision
    status = sensor_record_block->render(&sensor_record_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Last Record]: Soil moisture: 789 , Air Temp: 23.5 'C, Air Humidity: 90 , Lightness: 1001.",
        sensor_record_block->content.str.data,
        sensor_record_block->content.str.len
    );

    // Test 3: Update Lightness
    test_event.event_type = GMON_EVENT_LIGHTNESS_UPDATED;
    test_event.data.lightness = 1234;
    status = sensor_record_block->render(&sensor_record_block->content, &test_event);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
    TEST_ASSERT_EQUAL_STRING_LEN(
        "[Last Record]: Soil moisture: 789 , Air Temp: 23.5 'C, Air Humidity: 90 , Lightness: 1234.",
        sensor_record_block->content.str.data,
        sensor_record_block->content.str.len
    );
} // end of TEST(RenderPrintText, InitOk)

TEST_GROUP_RUNNER(gMonDisplay) {
    RUN_TEST_CASE(RenderPrintText, InitOk);
}
