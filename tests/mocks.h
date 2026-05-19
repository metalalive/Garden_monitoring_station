#ifndef TEST_GMON_MOCKS_H
#define TEST_GMON_MOCKS_H

#include "station_include.h"

// Define these missing configuration macros with arbitrary values for testing
#define GMON_CFG_NUM_SENSOR_RECORDS_KEEP    5

// Forward declarations for mock functions
gMonStatus staSetNetConnTaskInterval(gMonNet_t *, unsigned int interval_ms);
gMonStatus staSetTrigThresholdPump(gMonActuatorParam_t *, unsigned int threshold);
gMonStatus staSetTrigThresholdFan(gMonActuatorParam_t *, unsigned int threshold);
gMonStatus staSetTrigThresholdBulb(gMonActuatorParam_t *, unsigned int threshold);
gMonStatus staSetRequiredDaylenTicks(gardenMonitor_t *, unsigned int light_length);

#endif // TEST_GMON_MOCKS_H
