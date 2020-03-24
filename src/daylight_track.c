#include "station_include.h"

static unsigned int  gmon_light_last_recording_tick;
static unsigned int  gmon_light_ticks_since_last_recording;
static unsigned int  gmon_light_requied_daylength_ticks;
static unsigned int  gmon_light_actual_daylength_ticks;
static unsigned int  gmon_light_chk_interval_tick;

static gMonOutDevStatus  gmon_light_prev_bulb_status;


gMonStatus  staDaylightTrackInit(void)
{
    gmon_light_last_recording_tick = 0;
    staSetRequiredDaylenTicks(GMON_CFG_DEFAULT_REQUIRED_LIGHT_LENGTH_TICKS);
    gmon_light_actual_daylength_ticks = 0;
    gmon_light_prev_bulb_status = GMON_OUT_DEV_STATUS_OFF;
    return GMON_RESP_OK;
} // end of staDaylightTrackInit


gMonStatus  staDaylightTrackRefreshSensorData(unsigned int *out)
{
    if(out == NULL) { return GMON_RESP_ERRARGS; }
    unsigned int allowable_diff_ticks = 0;
    unsigned int curr_tick = 0;
    gMonStatus status = GMON_RESP_SKIP;
    
    allowable_diff_ticks  = staGetLightChkInterval();
    allowable_diff_ticks -= (allowable_diff_ticks >> 3);
    curr_tick = stationGetTicksPerDay();
    stationSysEnterCritical();
    gmon_light_ticks_since_last_recording = curr_tick - gmon_light_last_recording_tick;
    if(gmon_light_ticks_since_last_recording > allowable_diff_ticks) {
        status = GMON_SENSOR_READ_FN_LIGHT(out);
        if(status == GMON_RESP_OK) {
            gmon_light_last_recording_tick = curr_tick;
        }
    }
    stationSysExitCritical();
    return status;
} // end of staDaylightTrackRefreshSensorData


gMonStatus  staSetRequiredDaylenTicks(unsigned int light_length)
{
    gMonStatus  status = GMON_RESP_OK;
    if(light_length <= GMON_MAX_REQUIRED_LIGHT_LENGTH_TICKS) {
        stationSysEnterCritical();
        gmon_light_requied_daylength_ticks = light_length;
        stationSysExitCritical();
    } else {
        status = GMON_RESP_INVALID_REQ;
    }
    return status;
} // end of staSetRequiredDaylenTicks

void staUpdateLightChkInterval(unsigned int netconn_interval, unsigned short num_records_kept)
{
    if(num_records_kept == 0) { num_records_kept = 1; }
    stationSysEnterCritical();
    gmon_light_chk_interval_tick = netconn_interval / ((1 + num_records_kept) * GMON_NUM_MILLISECONDS_PER_TICK);
    stationSysExitCritical();
} // end of staUpdateLightChkInterval

unsigned int staGetLightChkInterval(void)
{
    return gmon_light_chk_interval_tick;
} // end of staGetLightChkInterval


unsigned int  staRefreshRequiredLightLength(gMonOutDevStatus bulb_status, unsigned int actual_light_ticks)
{
    unsigned int  out = 0;
    stationSysEnterCritical();
    if(bulb_status == GMON_OUT_DEV_STATUS_OFF) {
        if(gmon_light_prev_bulb_status != GMON_OUT_DEV_STATUS_OFF) {
            gmon_light_actual_daylength_ticks = 0;
        //// } else if() { // if this light tracker recorded sunrise time yesterday
        ////     gmon_light_actual_daylength_ticks = 0;
        }
        gmon_light_actual_daylength_ticks += actual_light_ticks;
    }
    if(gmon_light_requied_daylength_ticks > gmon_light_actual_daylength_ticks) {
        out = gmon_light_requied_daylength_ticks -  gmon_light_actual_daylength_ticks;
    } else {
        out = 0;
    }
    gmon_light_prev_bulb_status = bulb_status;
    stationSysExitCritical();
    return out;
} // end of staRefreshRequiredLightLength


unsigned int  staGetTicksSinceLastLightRecording(void)
{
    return gmon_light_ticks_since_last_recording;
}

