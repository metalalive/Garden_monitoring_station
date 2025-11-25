#include "unity_fixture.h"

static void RunAllTests(void) {
    RUN_TEST_GROUP(gMonUtility);
    RUN_TEST_GROUP(gMonAppMsg);
}

int main(int argc, const char *argv[]) {
    return UnityMain(argc, argv, RunAllTests);
}
