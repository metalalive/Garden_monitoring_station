#include "unity_fixture.h"

static void RunAllTests(void) {
    RUN_TEST_GROUP(gMonUtility);
    RUN_TEST_GROUP(gMonAppMsg);
    RUN_TEST_GROUP(gMonSensorEvt);
}

int main(int argc, const char *argv[]) {
    return UnityMain(argc, argv, RunAllTests);
}
