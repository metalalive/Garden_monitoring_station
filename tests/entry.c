#include "unity_fixture.h"

static void RunAllTests(void) {
    RUN_TEST_GROUP(gMonUtility);
    RUN_TEST_GROUP(gMonAppMsg);
    RUN_TEST_GROUP(gMonSensorEvt);
    RUN_TEST_GROUP(gMonSensorSample);
    RUN_TEST_GROUP(gMonActuator);
    RUN_TEST_GROUP(gMonDisplay);
}

int main(int argc, const char *argv[]) { return UnityMain(argc, argv, RunAllTests); }
