#include "station_include.h"
#include "mocks.h"

uint32_t UTestSysGetTickCount(void) {
    return g_mock_tick_count;
}

// Global mock variable for system tick count
uint32_t g_mock_tick_count = 0;

void setMockTickCount(uint32_t count) { g_mock_tick_count = count; }

gMonStatus  staSensorInitSoilMoist(void) {
    return GMON_RESP_OK;
}

gMonStatus  staSensorInitLight(void) {
    return GMON_RESP_OK;
}

gMonStatus  staSensorInitAirTemp(void) {
    return GMON_RESP_OK;
}

gMonStatus  staOutdevInitPump(gMonOutDev_t *dev) {
    (void) dev;
    return GMON_RESP_OK;
}

gMonStatus  staOutdevInitFan(gMonOutDev_t *dev) {
    (void) dev;
    return GMON_RESP_OK;
}

gMonStatus  staOutdevInitBulb(gMonOutDev_t *dev) {
    (void) dev;
    return GMON_RESP_OK;
}

gMonStatus staSetNetConnTaskInterval(gardenMonitor_t *gmon, unsigned int interval_ms) {
    gmon->netconn.interval_ms = interval_ms; // Update gmon for verification
    return GMON_RESP_OK;
}

gMonStatus staSetTrigThresholdPump(gMonOutDev_t *pump, unsigned int threshold) {
    pump->threshold = threshold; // Update pump for verification
    return GMON_RESP_OK;
}

gMonStatus staSetTrigThresholdFan(gMonOutDev_t *fan, unsigned int threshold) {
    fan->threshold = threshold; // Update fan for verification
    return GMON_RESP_OK;
}

gMonStatus staSetTrigThresholdBulb(gMonOutDev_t *bulb, unsigned int threshold) {
    bulb->threshold = threshold; // Update bulb for verification
    return GMON_RESP_OK;
}

gMonStatus staSetRequiredDaylenTicks(gardenMonitor_t *gmon, unsigned int light_length) {
    if (light_length <= GMON_MAX_REQUIRED_LIGHT_LENGTH_TICKS) {
        gmon->user_ctrl.required_light_daylength_ticks = light_length;
        return GMON_RESP_OK;
    }
    return GMON_RESP_INVALID_REQ;
}
