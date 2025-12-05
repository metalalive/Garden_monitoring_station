#include "unity.h"
#include "unity_fixture.h"
#include "station_include.h"

TEST_GROUP(AbsInt);

TEST_SETUP(AbsInt) {}

TEST_TEAR_DOWN(AbsInt) {}

TEST(AbsInt, Int32Ok) {
    unsigned int result = staAbsInt(42);
    TEST_ASSERT_EQUAL(42, result);
    result = staAbsInt(-42);
    TEST_ASSERT_EQUAL(42, result);
    result = staAbsInt(0);
    TEST_ASSERT_EQUAL(0, result);
    result = staAbsInt(1);
    TEST_ASSERT_EQUAL(1, result);
    result = staAbsInt(-1);
    TEST_ASSERT_EQUAL(1, result);
    result = staAbsInt(2147483647);
    TEST_ASSERT_EQUAL(2147483647, result);
    result = staAbsInt(-2147483648);
    TEST_ASSERT_EQUAL(2147483648U, result);
    result = staAbsInt(1000000);
    TEST_ASSERT_EQUAL(1000000, result);
    result = staAbsInt(-1000000);
    TEST_ASSERT_EQUAL(1000000, result);
}

TEST_GROUP(PartitionIntArray);

TEST_SETUP(PartitionIntArray) {}

TEST_TEAR_DOWN(PartitionIntArray) {}

static void verify_partition(unsigned int *list, unsigned short len, unsigned short partition_index) {
    if (len < 2) {
        // For empty or single-element arrays, no partitioning occurs, so the property
        // of elements left being <= elements right doesn't apply across a meaningful boundary.
        return;
    }
    unsigned short i = 0, j = 0;
    // Hoare partition property: all elements left of (or at) partition_index
    // are less than or equal to all elements right of it.
    for (i = 0; i <= partition_index; ++i) {
        for (j = partition_index + 1; j < len; ++j) {
            TEST_ASSERT_LESS_OR_EQUAL(list[j], list[i]);
        }
    }
}

// Function to sort an array (for verifying content integrity)
static int compare_unsigned_ints(const void *a, const void *b) {
    unsigned int val_a = *(const unsigned int *)a;
    unsigned int val_b = *(const unsigned int *)b;
    if (val_a < val_b)
        return -1;
    if (val_a > val_b)
        return 1;
    return 0;
}

// Helper to check if original elements are preserved (elements are just reordered)
static void verify_content_integrity(unsigned int *original, unsigned int *partitioned, unsigned short len) {
    if (len == 0)
        return;
    unsigned int *original_sorted = (unsigned int *)malloc(len * sizeof(unsigned int));
    unsigned int *partitioned_sorted = (unsigned int *)malloc(len * sizeof(unsigned int));

    TEST_ASSERT_NOT_NULL(original_sorted);
    TEST_ASSERT_NOT_NULL(partitioned_sorted);

    memcpy(original_sorted, original, len * sizeof(unsigned int));
    memcpy(partitioned_sorted, partitioned, len * sizeof(unsigned int));

    qsort(original_sorted, len, sizeof(unsigned int), compare_unsigned_ints);
    qsort(partitioned_sorted, len, sizeof(unsigned int), compare_unsigned_ints);

    for (unsigned short i = 0; i < len; ++i) {
        TEST_ASSERT_EQUAL(original_sorted[i], partitioned_sorted[i]);
    }

    free(original_sorted);
    free(partitioned_sorted);
}

TEST_GROUP(QuickSelect);

TEST_SETUP(QuickSelect) {
    // No specific setup needed
}

TEST_TEAR_DOWN(QuickSelect) {
    // No specific tear down needed
}

// Helper to sort an array for comparison, reused from PartitionIntArray tests
static int compare_unsigned_ints_asc(const void *a, const void *b) {
    unsigned int val_a = *(const unsigned int *)a;
    unsigned int val_b = *(const unsigned int *)b;
    if (val_a < val_b)
        return -1;
    if (val_a > val_b)
        return 1;
    return 0;
}

TEST(QuickSelect, NullList) {
    unsigned int  *list = NULL;
    unsigned short len = 5;
    unsigned short k = 2;
    unsigned int   result = staQuickSelect(list, len, k);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(QuickSelect, ZeroLengthList) {
    unsigned int   list[] = {1, 2, 3}; // Placeholder, actual length is 0
    unsigned short len = 0;
    unsigned short k = 0;
    unsigned int   result = staQuickSelect(list, len, k);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(QuickSelect, KOutOfBounds_GreaterThanOrEqualToLength) {
    unsigned int   list[] = {10, 20, 30};
    unsigned short len = 3;
    unsigned short k_equal_len = 3;
    unsigned short k_greater_than_len = 5;

    TEST_ASSERT_EQUAL(0, staQuickSelect(list, len, k_equal_len));
    TEST_ASSERT_EQUAL(0, staQuickSelect(list, len, k_greater_than_len));
}

TEST(QuickSelect, SingleElementList_K0) {
    unsigned int   list[] = {42};
    unsigned short len = 1;
    unsigned short k = 0;
    unsigned int   result = staQuickSelect(list, len, k);
    TEST_ASSERT_EQUAL(42, result);
}

TEST(QuickSelect, SmallestElement_K0_Unsorted) {
    unsigned int baselist[] = {5, 2, 8, 1, 9, 3};
    // the same values read several times
    unsigned int   expect_num[] = {1, 5, 9, 5, 1, 5, 9, 5, 1};
    unsigned int   test_index[] = {0, 3, 5, 3, 0, 3, 5, 3, 0};
    unsigned short len = 6, num_patterns = 9, idx = 0, k = 0; // 0-indexed smallest element
    for (idx = 0; idx < num_patterns; idx++) {
        k = test_index[idx];
        unsigned short actual_num = staQuickSelect(baselist, len, k);
        TEST_ASSERT_EQUAL(expect_num[idx], actual_num);
    }
}

TEST(QuickSelect, SortedAscending) {
    unsigned int   baselist[] = {1, 2, 3, 4, 5, 6};
    unsigned int  *expect_num = baselist;
    unsigned short len = 6, k = 0;
    for (k = 0; k < len; k++) {
        unsigned int currlist[6] = {0};
        memcpy(currlist, baselist, sizeof(unsigned int) * len);
        TEST_ASSERT_EQUAL(expect_num[k], staQuickSelect(currlist, len, k));
    }
}

TEST(QuickSelect, SortedDescending) {
    unsigned int   baselist[] = {19, 18, 17, 16, 15, 14, 13};
    unsigned int   expect_num[] = {13, 14, 15, 16, 17, 18, 19};
    unsigned short len = 7, k = 0;
    for (k = 0; k < len; k++) {
        unsigned int currlist[7] = {0};
        memcpy(currlist, baselist, sizeof(unsigned int) * len);
        TEST_ASSERT_EQUAL(expect_num[k], staQuickSelect(currlist, len, k));
        TEST_ASSERT_EQUAL(expect_num[k], staQuickSelect(currlist, len, k));
    }
}

TEST(QuickSelect, WithDuplicates) {
    // Original baselist: {3, 1, 4, 1, 5, 9, 2, 6, 5, 3}
    // Appended numbers (keeping existing): {7, 7, 0, 0, 8, 8, 2}
    unsigned int baselist[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 7, 7, 0, 0, 8, 8, 2};
    // Sorted version of baselist: {0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 8, 9}

    // Keeping existing access patterns and adding more, with values adjusted for the new baselist.
    // Note: The order of k values can be jumping around as per request.
    unsigned int test_index[] = {0, 1, 4, 9, 2, 5, 8, 10, 13, 16}; // k-th element to find (0-indexed)
    unsigned int expected_list[] = {0, 0, 2, 5, 1, 2, 4, 5, 7, 9};

    unsigned short len = sizeof(baselist) / sizeof(baselist[0]);
    unsigned short num_patterns = sizeof(test_index) / sizeof(test_index[0]);
    unsigned short idx = 0, k = 0;

    for (idx = 0; idx < num_patterns; idx++) {
        k = test_index[idx];
        unsigned int currlist[len];
        memcpy(currlist, baselist, sizeof(unsigned int) * len);
        unsigned int actual_num = staQuickSelect(currlist, len, k);
        TEST_ASSERT_EQUAL(expected_list[idx], actual_num);
    }
}

TEST(QuickSelect, AllElementsIdentical) {
    unsigned int   list[] = {7, 7, 7, 7, 7};
    unsigned short len = 5;

    TEST_ASSERT_EQUAL(7, staQuickSelect(list, len, 0));
    TEST_ASSERT_EQUAL(7, staQuickSelect(list, len, 2));
    TEST_ASSERT_EQUAL(7, staQuickSelect(list, len, 4));
}

TEST(QuickSelect, MiddleElementSmallUnsorted) {
    // Find the 2nd smallest element (k=1) in a small unsorted array
    unsigned int   baselist[] = {30, 10, 20};
    unsigned short len = sizeof(baselist) / sizeof(baselist[0]);
    unsigned short k = 1; // 2nd smallest (0-indexed)
    unsigned int   currlist[len];
    memcpy(currlist, baselist, sizeof(unsigned int) * len);
    TEST_ASSERT_EQUAL(20, staQuickSelect(currlist, len, k));
}

TEST(QuickSelect, MiddleElementReverseSorted) {
    // Find the 5th smallest element (k=4) in a reverse sorted array
    unsigned int   baselist[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    unsigned short len = sizeof(baselist) / sizeof(baselist[0]);
    unsigned short k = 4; // 5th smallest (0-indexed)
    unsigned int   currlist[len];
    memcpy(currlist, baselist, sizeof(unsigned int) * len);
    TEST_ASSERT_EQUAL(5, staQuickSelect(currlist, len, k));
}

TEST(QuickSelect, ElementWithScatteredDuplicates) {
    // Find the 4th smallest element (k=3) where the value has duplicates scattered
    unsigned int   baselist[] = {1, 5, 2, 5, 3, 5, 4, 5};
    unsigned short len = sizeof(baselist) / sizeof(baselist[0]);
    unsigned short k = 3; // 4th smallest (0-indexed)
    unsigned int   currlist[len];
    memcpy(currlist, baselist, sizeof(unsigned int) * len);
    TEST_ASSERT_EQUAL(4, staQuickSelect(currlist, len, k));
}

TEST(QuickSelect, TwoElements_K0) {
    unsigned int   list[] = {20, 10};
    unsigned short len = 2;
    TEST_ASSERT_EQUAL(10, staQuickSelect(list, len, 0));
}

TEST(QuickSelect, TwoElements_K1) {
    unsigned int   list[] = {20, 10};
    unsigned short len = 2;
    TEST_ASSERT_EQUAL(20, staQuickSelect(list, len, 1));
}

TEST(QuickSelect, LargeArrayMidK) {
    unsigned int   list[] = {99, 12, 54, 87, 3, 65, 21, 78, 43, 8, 91, 33, 70, 1, 26, 60, 15, 82, 49, 38};
    unsigned short len = 20;
    unsigned short k = 9; // 10th smallest (0-indexed)

    unsigned int expected_list[] = {99, 12, 54, 87, 3,  65, 21, 78, 43, 8,
                                    91, 33, 70, 1,  26, 60, 15, 82, 49, 38};
    qsort(expected_list, len, sizeof(unsigned int), compare_unsigned_ints_asc);

    TEST_ASSERT_EQUAL(expected_list[k], staQuickSelect(list, len, k));
}

TEST(QuickSelect, LargeArraySmallK) {
    unsigned int   list[] = {99, 12, 54, 87, 3, 65, 21, 78, 43, 8, 91, 33, 70, 1, 26, 60, 15, 82, 49, 38};
    unsigned short len = 20;
    unsigned short k = 2; // 3rd smallest (0-indexed)

    unsigned int expected_list[] = {99, 12, 54, 87, 3,  65, 21, 78, 43, 8,
                                    91, 33, 70, 1,  26, 60, 15, 82, 49, 38};
    qsort(expected_list, len, sizeof(unsigned int), compare_unsigned_ints_asc);

    TEST_ASSERT_EQUAL(expected_list[k], staQuickSelect(list, len, k));
}

TEST(QuickSelect, LargeArrayLargeK) {
    unsigned int   list[] = {99, 12, 54, 87, 3, 65, 21, 78, 43, 8, 91, 33, 70, 1, 26, 60, 15, 82, 49, 38};
    unsigned short len = 20;
    unsigned short k = 17; // 18th smallest (0-indexed)

    unsigned int expected_list[] = {99, 12, 54, 87, 3,  65, 21, 78, 43, 8,
                                    91, 33, 70, 1,  26, 60, 15, 82, 49, 38};
    qsort(expected_list, len, sizeof(unsigned int), compare_unsigned_ints_asc);

    TEST_ASSERT_EQUAL(expected_list[k], staQuickSelect(list, len, k));
}

TEST(PartitionIntArray, EmptyArray) {
    unsigned int   list[] = {};
    unsigned short len = 0;
    unsigned int   original_list[] = {};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, SingleElementArray) {
    unsigned int   list[] = {10};
    unsigned short len = 1;
    unsigned int   original_list[] = {10};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    TEST_ASSERT_EQUAL(10, list[0]); // Ensure element is untouched
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, TwoElementsAscending) {
    unsigned int   list[] = {1, 2};
    unsigned short len = 2;
    unsigned int   original_list[] = {1, 2};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, TwoElementsDescending) {
    unsigned int   list[] = {2, 1};
    unsigned short len = 2;
    unsigned int   original_list[] = {2, 1};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, TwoElementsEqual) {
    unsigned int   list[] = {5, 5};
    unsigned short len = 2;
    unsigned int   original_list[] = {5, 5};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, AllElementsIdentical) {
    unsigned int   list[] = {7, 7, 7, 7, 7};
    unsigned short len = 5;
    unsigned int   original_list[] = {7, 7, 7, 7, 7};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(2, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, SortedAscending) {
    unsigned int   list[] = {1, 2, 3, 4, 5};
    unsigned short len = 5;
    unsigned int   original_list[] = {1, 2, 3, 4, 5};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, SortedDescending) {
    unsigned int   list[] = {5, 4, 3, 2, 1};
    unsigned short len = 5;
    unsigned int   original_list[] = {5, 4, 3, 2, 1};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(3, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, MixedPivotSmallest) {
    unsigned int   list[] = {1, 5, 2, 8, 3};
    unsigned short len = 5;
    unsigned int   original_list[] = {1, 5, 2, 8, 3};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, MixedPivotLargest) {
    unsigned int   list[] = {8, 1, 5, 2, 3};
    unsigned short len = 5;
    unsigned int   original_list[] = {8, 1, 5, 2, 3};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(3, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, MixedPivotMiddle) {
    unsigned int   list[] = {5, 2, 8, 1, 9, 3};
    unsigned short len = 6;
    unsigned int   original_list[] = {5, 2, 8, 1, 9, 3};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(2, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, MixedPivotMiddle2) {
#define NUM_INIT_ITEMS  5
#define NUM_TOTAL_ITEMS 10
    unsigned int   currlist[NUM_TOTAL_ITEMS] = {0};
    unsigned int   original_list[NUM_TOTAL_ITEMS] = {15, 12, 18, 11, 19, 5, 23, 8, 25, 4};
    unsigned short expect_partition_idx[6] = {1, 2, 2, 3, 3, 4};
    for (unsigned short len = NUM_INIT_ITEMS; len <= NUM_TOTAL_ITEMS; len++) {
        memcpy(currlist, original_list, sizeof(unsigned int) * len);
        unsigned short partition_index = staPartitionIntArray(currlist, len);
        TEST_ASSERT_EQUAL(expect_partition_idx[len - NUM_INIT_ITEMS], partition_index);
        verify_partition(currlist, len, partition_index);
        verify_content_integrity(original_list, currlist, len);
    }
#undef NUM_INIT_ITEMS
#undef NUM_TOTAL_ITEMS
}

TEST(PartitionIntArray, MixedPivotMiddle3) {
    unsigned int   list[] = {6, 9, 5};
    unsigned short len = 3;
    unsigned int   original_list[] = {6, 9, 5};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, DuplicatesPivot) {
    unsigned int   list[] = {3, 5, 1, 5, 2, 4};
    unsigned short len = 6;
    unsigned int   original_list[] = {3, 5, 1, 5, 2, 4};
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(1, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, DuplicatesOfPivotScattered) {
    // Pivot is 5. Other 5s are scattered.
    unsigned int   list[] = {5, 1, 8, 5, 2, 9, 5, 3};
    unsigned short len = sizeof(list) / sizeof(list[0]);
    unsigned int   original_list[len];
    memcpy(original_list, list, len * sizeof(unsigned int));

    unsigned short partition_index = staPartitionIntArray(list, len);
    // Expected trace for {5, 1, 8, 5, 2, 9, 5, 3} with pivot 5:
    // 1. Initial: lo=0 (val 5), hi=7 (val 3). Both conditions fail. lo=0, hi=7.
    //    Swap(list[0], list[7]) -> {3, 1, 8, 5, 2, 9, 5, 5}. lo=1, hi=6.
    // 2. lo=1 (val 1, 5>1 T) -> lo=2. lo=2 (val 8, 5>8 F). lo is 2.
    //    hi=6 (val 5, 5<5 F). hi is 6.
    //    Swap(list[2], list[6]) -> {3, 1, 5, 5, 2, 9, 8, 5}. lo=3, hi=5.
    // 3. lo=3 (val 5, 5>5 F). lo is 3.
    //    hi=5 (val 9, 5<9 T) -> hi=4. hi=4 (val 2, 5<2 F). hi is 4.
    //    Swap(list[3], list[4]) -> {3, 1, 5, 2, 5, 9, 8, 5}. lo=4, hi=3.
    // 4. lo=4, hi=3. lo >= hi. Break. Return hi = 3.
    TEST_ASSERT_EQUAL(3, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, PivotIsMaxWithDuplicates) {
    // Pivot is 9 (max value), with another 9 at the end.
    unsigned int   list[] = {9, 1, 8, 2, 7, 3, 9};
    unsigned short len = sizeof(list) / sizeof(list[0]);
    unsigned int   original_list[len];
    memcpy(original_list, list, len * sizeof(unsigned int));

    unsigned short partition_index = staPartitionIntArray(list, len);
    // Expected trace for {9, 1, 8, 2, 7, 3, 9} with pivot 9:
    // 1. Initial: lo=0 (val 9), hi=6 (val 9). Both conditions fail. lo=0, hi=6.
    //    Swap(list[0], list[6]) -> {9, 1, 8, 2, 7, 3, 9}. lo=1, hi=5. (Array content visually unchanged)
    // 2. lo=1 (val 1, 9>1 T) -> lo=2. lo=2 (val 8, 9>8 T) -> lo=3. lo=3 (val 2, 9>2 T) -> lo=4.
    //    lo=4 (val 7, 9>7 T) -> lo=5. lo=5 (val 3, 9>3 T) -> lo=6. lo=6 (val 9, 9>9 F). lo is 6.
    //    hi=5 (val 3, 9<3 F). hi is 5.
    // 3. lo=6, hi=5. lo >= hi. Break. Return hi = 5.
    TEST_ASSERT_EQUAL(5, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, PivotIsMinWithDuplicates) {
    unsigned int   list[] = {1, 5, 2, 8, 3, 7, 1};
    unsigned short len = sizeof(list) / sizeof(list[0]);
    unsigned int   original_list[len];
    memcpy(original_list, list, len * sizeof(unsigned int));

    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(0, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST(PartitionIntArray, ManyDuplicates) {
    unsigned int   list[] = {3, 1, 3, 2, 3, 1, 3, 2, 0, 3, 1, 2, 0, 3, 1, 2};
    unsigned short len = sizeof(list) / sizeof(list[0]);
    unsigned int   original_list[len];
    memcpy(original_list, list, len * sizeof(unsigned int));
    unsigned short partition_index = staPartitionIntArray(list, len);
    TEST_ASSERT_EQUAL(10, partition_index);
    verify_partition(list, len, partition_index);
    verify_content_integrity(original_list, list, len);
}

TEST_GROUP(FindMedian);

TEST_SETUP(FindMedian) {}

TEST_TEAR_DOWN(FindMedian) {}

TEST(FindMedian, NullListZeroLength) {
    unsigned int   list[] = {1, 2, 3}; // Placeholder, actual length is 0
    unsigned short len = 3;
    unsigned int   result = staFindMedian(NULL, len);
    TEST_ASSERT_EQUAL(0, result);
    result = staFindMedian(list, 0);
    TEST_ASSERT_EQUAL(0, result);
}

TEST(FindMedian, SingleElementList) {
    unsigned int   list[] = {42};
    unsigned short len = 1;
    unsigned int   result = staFindMedian(list, len);
    TEST_ASSERT_EQUAL(42, result);
}

TEST(FindMedian, OddLength) {
    unsigned int baselist[] = {11, 12, 13, 14, 15};
    TEST_ASSERT_EQUAL(13, staFindMedian(baselist, 5));
    unsigned int baselist2[] = {25, 24, 23, 22, 21};
    TEST_ASSERT_EQUAL(23, staFindMedian(baselist2, 5));
    unsigned int baselist3[] = {5, 2, 8, 11, 19, 4, 10};
    TEST_ASSERT_EQUAL(8, staFindMedian(baselist3, 7));
}

TEST(FindMedian, EvenLength) {
    unsigned int baselist[] = {31, 32, 33, 34, 35, 36};
    TEST_ASSERT_EQUAL(33, staFindMedian(baselist, 6));
    unsigned int baselist2[] = {86, 85, 84, 83, 82, 81};
    TEST_ASSERT_EQUAL(83, staFindMedian(baselist2, 6));
    unsigned int baselist3[] = {25, 22, 28, 21, 29, 23};
    TEST_ASSERT_EQUAL(24, staFindMedian(baselist3, 6));
}

TEST(FindMedian, WithDuplicatesOddLength) {
    unsigned int baselist[] = {1, 3, 2, 3, 2, 3, 4};
    // Sorted: {1, 2, 2, 3, 3, 3, 4}. Median: 3
    unsigned short len = 7;
    TEST_ASSERT_EQUAL(3, staFindMedian(baselist, len));
}

TEST(FindMedian, WithDuplicatesEvenLength) {
    unsigned int baselist[] = {1, 3, 2, 2, 3, 4};
    // Sorted: {1, 2, 2, 3, 3, 4}. Medians: 2, 3. Average: (2+3)/2 = 2 (integer division).
    unsigned short len = 6;
    TEST_ASSERT_EQUAL(2, staFindMedian(baselist, len));
}

TEST(FindMedian, AllElementsIdentical) {
    unsigned int baselist[] = {7, 7, 7, 7, 7};
    TEST_ASSERT_EQUAL(7, staFindMedian(baselist, 5));
    TEST_ASSERT_EQUAL(7, staFindMedian(baselist, 4));
}

TEST(FindMedian, LargeArrayOddLength) {
    unsigned int   baselist[] = {99, 12, 54, 87, 3,  65, 21, 78, 43, 8,  91,
                                 33, 70, 1,  26, 60, 15, 82, 49, 38, 100}; // 21 elements
    unsigned short len = 21;
    unsigned int   currlist[len];
    memcpy(currlist, baselist, sizeof(unsigned int) * len);
    // Sorted list: {1, 3, 8, 12, 15, 21, 26, 33, 38, 43, 49, 54, 60, 65, 70, 78, 82, 87, 91, 99, 100}
    // Middle element (index 10) is 49.
    TEST_ASSERT_EQUAL(49, staFindMedian(currlist, len));
}

TEST(FindMedian, LargeArrayEvenLength) {
    unsigned int   baselist[] = {99, 12, 54, 87, 3,  65, 21, 78, 43, 8,
                                 91, 33, 70, 1,  26, 60, 15, 82, 49, 38}; // 20 elements
    unsigned short len = 20;
    unsigned int   currlist[len];
    memcpy(currlist, baselist, sizeof(unsigned int) * len);
    // Sorted list: {1, 3, 8, 12, 15, 21, 26, 33, 38, 43, 49, 54, 60, 65, 70, 78, 82, 87, 91, 99}
    // Middle elements (indices 9 and 10) are 43 and 49. Average: (43+49)/2 = 92/2 = 46.
    TEST_ASSERT_EQUAL(46, staFindMedian(currlist, len));
}

TEST_GROUP(MedianAbsDeviation);

TEST_SETUP(MedianAbsDeviation) {}

TEST_TEAR_DOWN(MedianAbsDeviation) {}

TEST(MedianAbsDeviation, InputZeroNull) {
    unsigned int   median = 5;
    unsigned int   list[] = {1, 2, 3}; // Actual list content is irrelevant for len=0
    unsigned short len = 5;
    TEST_ASSERT_EQUAL(0, staMedianAbsDeviation(median, NULL, len));
    TEST_ASSERT_EQUAL(0, staMedianAbsDeviation(median, list, 0));
    TEST_ASSERT_EQUAL(0, staMedianAbsDeviation(0, list, len));
}

TEST(MedianAbsDeviation, OddLengthList) {
    unsigned int   list[] = {1, 2, 3, 4, 5};
    unsigned int   median = 3; // Median of list itself
    unsigned short len = 5;
    // Deviations: { |1-3|, |2-3|, |3-3|, |4-3|, |5-3| } = {2, 1, 0, 1, 2}
    // Sorted deviations: {0, 1, 1, 2, 2}
    // Median of deviations: 1
    TEST_ASSERT_EQUAL(1, staMedianAbsDeviation(median, list, len));
}

TEST(MedianAbsDeviation, EvenLengthList) {
    unsigned int   list[] = {1, 2, 3, 4, 5, 6};
    unsigned int   median = 3; // (3+4)/2 = 3 (integer division)
    unsigned short len = sizeof(list) / sizeof(list[0]);
    // Deviations: { |1-3|, |2-3|, |3-3|, |4-3|, |5-3|, |6-3| } = {2, 1, 0, 1, 2, 3}
    // Sorted deviations: {0, 1, 1, 2, 2, 3}
    // staFindMedian for even length will find median of 2 and 1 (values at indices 2 and 3 after sorting)
    // (2+1)/2 = 1 (integer division)
    TEST_ASSERT_EQUAL(1, staMedianAbsDeviation(median, list, len));
}

TEST(MedianAbsDeviation, ListWithDuplicates) {
    unsigned int   list[] = {10, 20, 10, 30, 20};
    unsigned int   median = 20;
    unsigned short len = sizeof(list) / sizeof(list[0]);
    // Deviations: { |10-20|, |20-20|, |10-20|, |30-20|, |20-20| } = {10, 0, 10, 10, 0}
    // Sorted deviations: {0, 0, 10, 10, 10}
    // Median of deviations: 10
    TEST_ASSERT_EQUAL(10, staMedianAbsDeviation(median, list, len));
}

TEST(MedianAbsDeviation, AllElementsIdentical) {
    unsigned int   list[] = {5, 5, 5, 5, 5};
    unsigned int   median = 5;
    unsigned short len = sizeof(list) / sizeof(list[0]);
    // Deviations: { |5-5|, |5-5|, |5-5|, |5-5|, |5-5| } = {0, 0, 0, 0, 0}
    // Sorted deviations: {0, 0, 0, 0, 0}
    // Median of deviations: 0
    TEST_ASSERT_EQUAL(0, staMedianAbsDeviation(median, list, len));
}

TEST(MedianAbsDeviation, MixedValuesAndNegativeDiffs) {
    unsigned int   list[] = {10, 1, 5, 12, 8};
    unsigned int   median = 8;
    unsigned short len = sizeof(list) / sizeof(list[0]);
    // Deviations: { |10-8|, |1-8|, |5-8|, |12-8|, |8-8| } = {2, 7, 3, 4, 0}
    // Sorted deviations: {0, 2, 3, 4, 7}
    // Median of deviations: 3
    TEST_ASSERT_EQUAL(3, staMedianAbsDeviation(median, list, len));
}

TEST(MedianAbsDeviation, ComplexScenario) {
    unsigned int list[] = {4, 100, 2, 101, 3, 102};
    // Using a hypothetical median not derived from the list, as the function takes it as input.
    unsigned int   median = 50;
    unsigned short len = sizeof(list) / sizeof(list[0]);
    // Deviations: { |1-50|, |2-50|, |3-50|, |100-50|, |101-50|, |102-50| }
    //           = { 49, 48, 47, 50, 51, 52 }
    // Sorted deviations: {47, 48, 49, 50, 51, 52}
    // staFindMedian for even length will find median of 50 and 49 (values at indices 2 and 3 after sorting)
    // (50+49)/2 = 49 (integer division)
    TEST_ASSERT_EQUAL(49, staMedianAbsDeviation(median, list, len));
}

TEST_GROUP_RUNNER(gMonUtilityStatistical) {
    RUN_TEST_CASE(AbsInt, Int32Ok);

    RUN_TEST_CASE(PartitionIntArray, EmptyArray);
    RUN_TEST_CASE(PartitionIntArray, SingleElementArray);
    RUN_TEST_CASE(PartitionIntArray, TwoElementsAscending);
    RUN_TEST_CASE(PartitionIntArray, TwoElementsDescending);
    RUN_TEST_CASE(PartitionIntArray, TwoElementsEqual);
    RUN_TEST_CASE(PartitionIntArray, AllElementsIdentical);
    RUN_TEST_CASE(PartitionIntArray, SortedAscending);
    RUN_TEST_CASE(PartitionIntArray, SortedDescending);
    RUN_TEST_CASE(PartitionIntArray, MixedPivotSmallest);
    RUN_TEST_CASE(PartitionIntArray, MixedPivotLargest);
    RUN_TEST_CASE(PartitionIntArray, MixedPivotMiddle);
    RUN_TEST_CASE(PartitionIntArray, MixedPivotMiddle2);
    RUN_TEST_CASE(PartitionIntArray, MixedPivotMiddle3);
    RUN_TEST_CASE(PartitionIntArray, DuplicatesOfPivotScattered);
    RUN_TEST_CASE(PartitionIntArray, PivotIsMaxWithDuplicates);
    RUN_TEST_CASE(PartitionIntArray, PivotIsMinWithDuplicates);
    RUN_TEST_CASE(PartitionIntArray, DuplicatesPivot);
    RUN_TEST_CASE(PartitionIntArray, ManyDuplicates);

    RUN_TEST_CASE(QuickSelect, NullList);
    RUN_TEST_CASE(QuickSelect, ZeroLengthList);
    RUN_TEST_CASE(QuickSelect, KOutOfBounds_GreaterThanOrEqualToLength);
    RUN_TEST_CASE(QuickSelect, SingleElementList_K0);
    RUN_TEST_CASE(QuickSelect, SmallestElement_K0_Unsorted);
    RUN_TEST_CASE(QuickSelect, SortedAscending);
    RUN_TEST_CASE(QuickSelect, SortedDescending);
    RUN_TEST_CASE(QuickSelect, WithDuplicates);
    RUN_TEST_CASE(QuickSelect, AllElementsIdentical);
    RUN_TEST_CASE(QuickSelect, TwoElements_K0);
    RUN_TEST_CASE(QuickSelect, TwoElements_K1);
    RUN_TEST_CASE(QuickSelect, MiddleElementSmallUnsorted);
    RUN_TEST_CASE(QuickSelect, MiddleElementReverseSorted);
    RUN_TEST_CASE(QuickSelect, ElementWithScatteredDuplicates);
    RUN_TEST_CASE(QuickSelect, LargeArrayMidK);
    RUN_TEST_CASE(QuickSelect, LargeArraySmallK);
    RUN_TEST_CASE(QuickSelect, LargeArrayLargeK);

    RUN_TEST_CASE(FindMedian, NullListZeroLength);
    RUN_TEST_CASE(FindMedian, SingleElementList);
    RUN_TEST_CASE(FindMedian, OddLength);
    RUN_TEST_CASE(FindMedian, EvenLength);
    RUN_TEST_CASE(FindMedian, WithDuplicatesOddLength);
    RUN_TEST_CASE(FindMedian, WithDuplicatesEvenLength);
    RUN_TEST_CASE(FindMedian, AllElementsIdentical);
    RUN_TEST_CASE(FindMedian, LargeArrayOddLength);
    RUN_TEST_CASE(FindMedian, LargeArrayEvenLength);

    RUN_TEST_CASE(MedianAbsDeviation, InputZeroNull);
    RUN_TEST_CASE(MedianAbsDeviation, OddLengthList);
    RUN_TEST_CASE(MedianAbsDeviation, EvenLengthList);
    RUN_TEST_CASE(MedianAbsDeviation, ListWithDuplicates);
    RUN_TEST_CASE(MedianAbsDeviation, AllElementsIdentical);
    RUN_TEST_CASE(MedianAbsDeviation, MixedValuesAndNegativeDiffs);
    RUN_TEST_CASE(MedianAbsDeviation, ComplexScenario);
}
