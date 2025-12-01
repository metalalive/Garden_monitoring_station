#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"
#include "mocks.h"

TEST_GROUP(ReverseString);

TEST_SETUP(ReverseString) {
    // No specific setup needed for staReverseString tests
}

TEST_TEAR_DOWN(ReverseString) {
    // No specific tear down needed
}

TEST(ReverseString, NullStringPointer) {
    unsigned char *null_str = NULL;
    // Calling with NULL should not crash due to the initial check in the function
    staReverseString(null_str, 5); // Should do nothing
}

TEST(ReverseString, ZeroSizeString) {
    unsigned char test_str[] = "abc";
    unsigned char expected[] = "abc";
    staReverseString(test_str, 0); // Should do nothing
    TEST_ASSERT_EQUAL_STRING_LEN(expected, test_str, 3);
}

TEST(ReverseString, SingleCharacterString) {
    unsigned char test_str[] = "A";
    unsigned char expected[] = "A";
    staReverseString(test_str, sizeof(test_str) - 1); // -1 for null terminator
    TEST_ASSERT_EQUAL_STRING(expected, test_str);
}

TEST(ReverseString, EvenLengthString) {
    unsigned char test_str[] = "ABCD";
    unsigned char expected[] = "DCBA";
    staReverseString(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, test_str, sizeof(test_str) - 1);
}

TEST(ReverseString, OddLengthString) {
    unsigned char test_str[] = "ABCDE";
    unsigned char expected[] = "EDCBA";
    staReverseString(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL_STRING(expected, test_str);
}

TEST(ReverseString, StringWithSpaces) {
    unsigned char test_str[] = "hello world";
    unsigned char expected[] = "dlrow olleh";
    staReverseString(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL_STRING(expected, test_str);
}

TEST(ReverseString, StringWithSpecialCharacters) {
    unsigned char test_str[] = "!@#$%";
    unsigned char expected[] = "%$#@!";
    staReverseString(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL_STRING(expected, test_str);
}

TEST_GROUP(CvtFloatToStr);

TEST_SETUP(CvtFloatToStr) {
    // No specific setup needed
}

TEST_TEAR_DOWN(CvtFloatToStr) {
    // No specific tear down needed
}

TEST(CvtFloatToStr, ZeroPrecision) {
    unsigned char outstr[10];
    float         num = 123.456f;
    unsigned int  len = staCvtFloatToStr(outstr, num, 0);
    outstr[len] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("123", outstr);
    TEST_ASSERT_EQUAL(3, len);
}

TEST(CvtFloatToStr, PositiveFloatWithPrecision) {
    unsigned char outstr[10];
    float         num = 123.456f;
    unsigned int  len = staCvtFloatToStr(outstr, num, 2);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("123.45", outstr);
    TEST_ASSERT_EQUAL(6, len);

    len = staCvtFloatToStr(outstr, num, 3);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("123.456", outstr);
    TEST_ASSERT_EQUAL(7, len);
}

TEST(CvtFloatToStr, NegativeFloatWithPrecision) {
    unsigned char outstr[10];
    float         num = -123.456f;
    unsigned int  len = staCvtFloatToStr(outstr, num, 2);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("-123.45", outstr);
    TEST_ASSERT_EQUAL(7, len);

    len = staCvtFloatToStr(outstr, num, 3);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("-123.456", outstr);
    TEST_ASSERT_EQUAL(8, len);
}

TEST(CvtFloatToStr, ZeroValue) {
    unsigned char outstr[10];
    float         num = 0.0f;
    unsigned int  len = staCvtFloatToStr(outstr, num, 2);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("0", outstr);
    TEST_ASSERT_EQUAL(1, len);
}

TEST(CvtFloatToStr, NegativeZeroValue) {
    unsigned char outstr[10];
    float         num = -0.0f; // Note: -0.0f is distinct from 0.0f for floats
    unsigned int  len = staCvtFloatToStr(outstr, num, 2);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("0", outstr);
    TEST_ASSERT_EQUAL(1, len);
}

TEST(CvtFloatToStr, LargeIntegralPart) {
    unsigned char outstr[20];
    float         num = 1234567.89f;
    unsigned int  len = staCvtFloatToStr(outstr, num, 1);
    outstr[len] = '\0';
    TEST_ASSERT_EQUAL_STRING("1234567.8", outstr);
    TEST_ASSERT_EQUAL(9, len);
}

TEST(CvtFloatToStr, RenderPlaceholder) {
    unsigned char outstr[20] = {'H', 'y', 'p', 'e', ' ', 0,   0,   0,   0,   0,
                                0,   0,   0,   ' ', 'D', 'R', 'i', 'V', 'e', 'n'};
    float         num = -123.4567f;
    unsigned int  len = staCvtFloatToStr(&outstr[5], num, 3);
    TEST_ASSERT_EQUAL_STRING_LEN("Hype -123.456 DRiVen", outstr, 20);
    TEST_ASSERT_EQUAL(8, len);
}

TEST_GROUP(ChkIntFromStr);

TEST_SETUP(ChkIntFromStr) {
    // No specific setup needed
}

TEST_TEAR_DOWN(ChkIntFromStr) {
    // No specific tear down needed
}

TEST(ChkIntFromStr, NullStringPointer) {
    unsigned char *null_str = NULL;
    gMonStatus     status = staChkIntFromStr(null_str, 5);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ZeroSizeString) {
    unsigned char test_str[] = "123";
    gMonStatus    status = staChkIntFromStr(test_str, 0);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, EmptyStringPointer) {
    unsigned char *empty_str = (unsigned char *)"";
    gMonStatus     status = staChkIntFromStr(empty_str, 0);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ValidPositiveInteger) {
    unsigned char test_str[] = "12345";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ValidPositiveSingleDigit) {
    unsigned char test_str[] = "7";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ValidPositiveZero) {
    unsigned char test_str[] = "0";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ValidNegativeInteger) {
    unsigned char test_str[] = "-12345";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ValidNegativeSingleDigit) {
    unsigned char test_str[] = "-7";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, ValidNegativeZero) {
    unsigned char test_str[] = "-0";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, StringWithAlphaChar) {
    unsigned char test_str[] = "123A45";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(ChkIntFromStr, StringWithSpecialChar) {
    unsigned char test_str[] = "123.45";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(ChkIntFromStr, StringWithLeadingSpace) {
    unsigned char test_str[] = " 123";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(ChkIntFromStr, StringWithTrailingSpace) {
    unsigned char test_str[] = "123 ";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(ChkIntFromStr, StringWithOnlySignAndNoDigits) {
    unsigned char test_str[] = "-";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_OK, status);
}

TEST(ChkIntFromStr, StringWithOnlySignAndNonDigit) {
    unsigned char test_str[] = "-A";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST(ChkIntFromStr, StringStartingWithInvalidChar) {
    unsigned char test_str[] = "A123";
    gMonStatus    status = staChkIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(GMON_RESP_MALFORMED_DATA, status);
}

TEST_GROUP(CvtIntFromStr);

TEST_SETUP(CvtIntFromStr) {
    // No specific setup needed
}

TEST_TEAR_DOWN(CvtIntFromStr) {
    // No specific tear down needed
}

TEST(CvtIntFromStr, NullStringPointer) {
    unsigned char *null_str = NULL;
    int            result = staCvtIntFromStr(null_str, 5); // Size doesn't matter much for NULL
    TEST_ASSERT_EQUAL(0, result);
}

TEST(CvtIntFromStr, ZeroSizeString) {
    unsigned char test_str[] = "123";
    int           result = staCvtIntFromStr(test_str, 0);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(CvtIntFromStr, EmptyStringPointer) {
    unsigned char *empty_str = (unsigned char *)"";
    int            result = staCvtIntFromStr(empty_str, 0);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(CvtIntFromStr, ValidPositiveInteger) {
    unsigned char test_str[] = "12345";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(12345, result);
}

TEST(CvtIntFromStr, ValidPositiveSingleDigit) {
    unsigned char test_str[] = "7";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(7, result);
}

TEST(CvtIntFromStr, ValidPositiveZero) {
    unsigned char test_str[] = "0";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(CvtIntFromStr, ValidNegativeInteger) {
    unsigned char test_str[] = "-12345";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-12345, result);
}

TEST(CvtIntFromStr, ValidNegativeSingleDigit) {
    unsigned char test_str[] = "-7";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-7, result);
}

TEST(CvtIntFromStr, ValidNegativeZero) {
    unsigned char test_str[] = "-0";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(0, result); // -0 should be 0
}

TEST(CvtIntFromStr, StringWithAlphaChar) {
    unsigned char test_str[] = "123A45";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-1, result); // Should indicate error
}

TEST(CvtIntFromStr, StringWithSpecialChar) {
    unsigned char test_str[] = "123.45";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-1, result); // Should indicate error
}

TEST(CvtIntFromStr, StringWithLeadingSpace) {
    unsigned char test_str[] = " 123";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-1, result); // Space is not a digit
}

TEST(CvtIntFromStr, StringWithTrailingSpace) {
    unsigned char test_str[] = "123 ";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-1, result); // Space is not a digit
}

TEST(CvtIntFromStr, StringWithOnlySignAndNoDigits) {
    unsigned char test_str[] = "-";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(0, result); // Current implementation returns 0 for "-"
}

TEST(CvtIntFromStr, StringWithOnlySignAndNonDigit) {
    unsigned char test_str[] = "-A";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-1, result); // Should indicate error after sign
}

TEST(CvtIntFromStr, StringStartingWithInvalidChar) {
    unsigned char test_str[] = "A123";
    int           result = staCvtIntFromStr(test_str, sizeof(test_str) - 1);
    TEST_ASSERT_EQUAL(-1, result); // 'A' is not a digit
}

TEST_GROUP(TimeTracking);

static gmonTick_t test_gmon_tick;

TEST_SETUP(TimeTracking) {
    // Initialize tick_info for each test
    test_gmon_tick.ticks_per_day = 0;
    test_gmon_tick.days = 0;
    test_gmon_tick.last_read = 0;
    setMockTickCount(0); // Reset mock system tick count
}

TEST_TEAR_DOWN(TimeTracking) {
    // No specific tear down needed
}

TEST(TimeTracking, InitialState) {
    // Initially, everything should be zero
    TEST_ASSERT_EQUAL(0, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, test_gmon_tick.last_read);
}

TEST(TimeTracking, IncrementWithinSameDay) {
    setMockTickCount(100);
    TEST_ASSERT_EQUAL(100, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(100, test_gmon_tick.last_read);

    setMockTickCount(250);
    TEST_ASSERT_EQUAL(250, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(250, test_gmon_tick.last_read);
}

TEST(TimeTracking, CrossSingleDayBoundaryPerfectly) {
    // Simulate going from 0 to GMON_NUM_TICKS_PER_DAY
    setMockTickCount(GMON_NUM_TICKS_PER_DAY);
    TEST_ASSERT_EQUAL(0, stationGetTicksPerDay(&test_gmon_tick)); // Should wrap to 0
    TEST_ASSERT_EQUAL(1, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(GMON_NUM_TICKS_PER_DAY, test_gmon_tick.last_read);
}

TEST(TimeTracking, CrossSingleDayBoundaryWithOffset) {
    // Simulate going from 0 to GMON_NUM_TICKS_PER_DAY + 500 ticks
    setMockTickCount(GMON_NUM_TICKS_PER_DAY + 500);
    TEST_ASSERT_EQUAL(500, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(1, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(GMON_NUM_TICKS_PER_DAY + 500, test_gmon_tick.last_read);
}

TEST(TimeTracking, CrossMultipleDayBoundaries) {
    // Simulate ticks which go across day boundaries
    uint32_t expect_tick = 750;
    setMockTickCount(expect_tick);
    TEST_ASSERT_EQUAL(750, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(expect_tick, test_gmon_tick.last_read);
    expect_tick = 1 * GMON_NUM_TICKS_PER_DAY + 357;
    setMockTickCount(expect_tick);
    TEST_ASSERT_EQUAL(357, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(1, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(expect_tick, test_gmon_tick.last_read);
    expect_tick = 2 * GMON_NUM_TICKS_PER_DAY + 8;
    setMockTickCount(expect_tick);
    TEST_ASSERT_EQUAL(8, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(2, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(expect_tick, test_gmon_tick.last_read);
}

TEST(TimeTracking, MultipleCallsAcrossDays) {
    // First call: increment within day 0
    setMockTickCount(GMON_NUM_TICKS_PER_DAY / 2); // Half a day
    TEST_ASSERT_EQUAL(GMON_NUM_TICKS_PER_DAY / 2, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, stationGetDays(&test_gmon_tick));

    // Second call: cross to day 1
    setMockTickCount(GMON_NUM_TICKS_PER_DAY + 100);
    TEST_ASSERT_EQUAL(100, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(1, stationGetDays(&test_gmon_tick));

    // Third call: cross to day 2
    setMockTickCount(2 * GMON_NUM_TICKS_PER_DAY + 200);
    TEST_ASSERT_EQUAL(200, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(2, stationGetDays(&test_gmon_tick));
}

TEST(TimeTracking, InitialNonZeroStateTicksAndDays) {
    // Setup with some initial state
    test_gmon_tick.ticks_per_day = GMON_NUM_TICKS_PER_DAY / 4; // 1/4 of a day
    test_gmon_tick.days = 5;
    test_gmon_tick.last_read = 1000; // Arbitrary previous tick count
    uint32_t tick_increased = 567;
    // Simulate a small increment within the same day
    setMockTickCount(test_gmon_tick.last_read + tick_increased);
    // The `last_read` member within `test_gmon_tick` is updated by `staRefreshTimeTicks`.
    // We need to store the expected `last_read` before the function call for the assertion.
    uint32_t expect_value = (GMON_NUM_TICKS_PER_DAY / 4) + tick_increased;
    TEST_ASSERT_EQUAL(expect_value, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(5, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(1567, test_gmon_tick.last_read);

    // Simulate crossing a day boundary from this initial state
    // Note: stationGetTicksPerDay will update test_gmon_tick.last_read implicitly
    tick_increased = GMON_NUM_TICKS_PER_DAY + 100;
    setMockTickCount(test_gmon_tick.last_read + tick_increased);
    expect_value = (GMON_NUM_TICKS_PER_DAY / 4) + 100 + 567;
    TEST_ASSERT_EQUAL(expect_value, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(6, stationGetDays(&test_gmon_tick)); // Should increment by 1 day
    TEST_ASSERT_EQUAL(GMON_NUM_TICKS_PER_DAY + 1667, test_gmon_tick.last_read);
}

TEST(TimeTracking, currTickWrapsAroundMaxUint32) {
    // This simulates the system tick counter overflowing and wrapping around.
    test_gmon_tick.last_read = UINT32_MAX - 100; // Near max value
    setMockTickCount(50);                        // Simulate wrap-around (curr_tick < last_read)
    // diff = (UINT32_MAX - (UINT32_MAX - 100)) + 50 + 1 = 100 + 50 + 1 = 151
    TEST_ASSERT_EQUAL(151, stationGetTicksPerDay(&test_gmon_tick));
    TEST_ASSERT_EQUAL(0, stationGetDays(&test_gmon_tick));
    TEST_ASSERT_EQUAL(50, test_gmon_tick.last_read);
}

TEST_GROUP_RUNNER(gMonUtility) {
    RUN_TEST_CASE(ReverseString, NullStringPointer);
    RUN_TEST_CASE(ReverseString, ZeroSizeString);
    RUN_TEST_CASE(ReverseString, SingleCharacterString);
    RUN_TEST_CASE(ReverseString, EvenLengthString);
    RUN_TEST_CASE(ReverseString, OddLengthString);
    RUN_TEST_CASE(ReverseString, StringWithSpaces);
    RUN_TEST_CASE(ReverseString, StringWithSpecialCharacters);
    RUN_TEST_CASE(CvtFloatToStr, ZeroPrecision);
    RUN_TEST_CASE(CvtFloatToStr, PositiveFloatWithPrecision);
    RUN_TEST_CASE(CvtFloatToStr, NegativeFloatWithPrecision);
    RUN_TEST_CASE(CvtFloatToStr, ZeroValue);
    RUN_TEST_CASE(CvtFloatToStr, NegativeZeroValue);
    RUN_TEST_CASE(CvtFloatToStr, LargeIntegralPart);
    RUN_TEST_CASE(CvtFloatToStr, RenderPlaceholder);
    RUN_TEST_CASE(ChkIntFromStr, NullStringPointer);
    RUN_TEST_CASE(ChkIntFromStr, ZeroSizeString);
    RUN_TEST_CASE(ChkIntFromStr, EmptyStringPointer);
    RUN_TEST_CASE(ChkIntFromStr, ValidPositiveInteger);
    RUN_TEST_CASE(ChkIntFromStr, ValidPositiveSingleDigit);
    RUN_TEST_CASE(ChkIntFromStr, ValidPositiveZero);
    RUN_TEST_CASE(ChkIntFromStr, ValidNegativeInteger);
    RUN_TEST_CASE(ChkIntFromStr, ValidNegativeSingleDigit);
    RUN_TEST_CASE(ChkIntFromStr, ValidNegativeZero);
    RUN_TEST_CASE(ChkIntFromStr, StringWithAlphaChar);
    RUN_TEST_CASE(ChkIntFromStr, StringWithSpecialChar);
    RUN_TEST_CASE(ChkIntFromStr, StringWithLeadingSpace);
    RUN_TEST_CASE(ChkIntFromStr, StringWithTrailingSpace);
    RUN_TEST_CASE(ChkIntFromStr, StringWithOnlySignAndNoDigits);
    RUN_TEST_CASE(ChkIntFromStr, StringWithOnlySignAndNonDigit);
    RUN_TEST_CASE(ChkIntFromStr, StringStartingWithInvalidChar);
    RUN_TEST_CASE(CvtIntFromStr, NullStringPointer);
    RUN_TEST_CASE(CvtIntFromStr, ZeroSizeString);
    RUN_TEST_CASE(CvtIntFromStr, EmptyStringPointer);
    RUN_TEST_CASE(CvtIntFromStr, ValidPositiveInteger);
    RUN_TEST_CASE(CvtIntFromStr, ValidPositiveSingleDigit);
    RUN_TEST_CASE(CvtIntFromStr, ValidPositiveZero);
    RUN_TEST_CASE(CvtIntFromStr, ValidNegativeInteger);
    RUN_TEST_CASE(CvtIntFromStr, ValidNegativeSingleDigit);
    RUN_TEST_CASE(CvtIntFromStr, ValidNegativeZero);
    RUN_TEST_CASE(CvtIntFromStr, StringWithAlphaChar);
    RUN_TEST_CASE(CvtIntFromStr, StringWithSpecialChar);
    RUN_TEST_CASE(CvtIntFromStr, StringWithLeadingSpace);
    RUN_TEST_CASE(CvtIntFromStr, StringWithTrailingSpace);
    RUN_TEST_CASE(CvtIntFromStr, StringWithOnlySignAndNoDigits);
    RUN_TEST_CASE(CvtIntFromStr, StringWithOnlySignAndNonDigit);
    RUN_TEST_CASE(CvtIntFromStr, StringStartingWithInvalidChar);

    RUN_TEST_CASE(TimeTracking, InitialState);
    RUN_TEST_CASE(TimeTracking, IncrementWithinSameDay);
    RUN_TEST_CASE(TimeTracking, CrossSingleDayBoundaryPerfectly);
    RUN_TEST_CASE(TimeTracking, CrossSingleDayBoundaryWithOffset);
    RUN_TEST_CASE(TimeTracking, CrossMultipleDayBoundaries);
    RUN_TEST_CASE(TimeTracking, MultipleCallsAcrossDays);
    RUN_TEST_CASE(TimeTracking, InitialNonZeroStateTicksAndDays);
    RUN_TEST_CASE(TimeTracking, currTickWrapsAroundMaxUint32);
}
