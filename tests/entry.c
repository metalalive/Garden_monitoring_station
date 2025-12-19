#include "unity_fixture.h"

static void RunAllTests(void) {
    RUN_TEST_GROUP(gMonUtilityStrProcess);
    RUN_TEST_GROUP(gMonUtilityStatistical);
    RUN_TEST_GROUP(gMonAppMsg);
    RUN_TEST_GROUP(gMonSensorEvt);
    RUN_TEST_GROUP(gMonSensorSample);
    RUN_TEST_GROUP(gMonActuator);
    RUN_TEST_GROUP(gMonDisplay);
    RUN_TEST_GROUP(gMonSoilSensor);
}

int main(int argc, const char *argv[]) { return UnityMain(argc, argv, RunAllTests); }
