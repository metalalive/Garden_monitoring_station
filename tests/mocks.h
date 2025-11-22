#ifndef TEST_GMON_MOCKS_H
#define TEST_GMON_MOCKS_H

#include "station_include.h"

// Define these missing configuration macros with arbitrary values for testing
#define GMON_CFG_NUM_SENSOR_RECORDS_KEEP    5

// Forward declarations for mock functions
gMonStatus staSetNetConnTaskInterval(gardenMonitor_t *gmon, unsigned int interval_ms);
gMonStatus staSetTrigThresholdPump(gMonOutDev_t *pump, unsigned int threshold);
gMonStatus staSetTrigThresholdFan(gMonOutDev_t *fan, unsigned int threshold);
gMonStatus staSetTrigThresholdBulb(gMonOutDev_t *bulb, unsigned int threshold);
gMonStatus staSetRequiredDaylenTicks(gardenMonitor_t *gmon, unsigned int light_length);

#endif // TEST_GMON_MOCKS_H
