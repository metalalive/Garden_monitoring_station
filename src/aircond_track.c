#include "station_include.h"

static unsigned int  gmon_aircond_last_recording_tick = 0;
static unsigned int  gmon_aircond_ticks_since_last_recording;
static unsigned int  gmon_aircond_chk_interval_tick;

gMonStatus  staAirCondTrackInit(void)
{
    staUpdateAirCondChkInterval((unsigned int)GMON_CFG_NETCONN_START_INTERVAL_MS , (unsigned short)GMON_CFG_NUM_SENSOR_RECORDS_KEEP);
    return GMON_RESP_OK;
} // end of staAirCondTrackInit


gMonStatus  staAirCondTrackRefreshSensorData(float *air_temp, float *air_humid)
{
    if(air_temp == NULL || air_humid == NULL) {
        return GMON_RESP_ERRARGS;
    }
    unsigned int allowable_diff_ticks = 0;
    unsigned int curr_tick = 0;
    gMonStatus status = GMON_RESP_SKIP;
    
    allowable_diff_ticks  = staGetAirCondChkInterval();
    allowable_diff_ticks -= allowable_diff_ticks >> 3;
    curr_tick = stationGetTicksPerDay();
    gmon_aircond_ticks_since_last_recording = curr_tick - gmon_aircond_last_recording_tick;
    if(gmon_aircond_ticks_since_last_recording > allowable_diff_ticks) {
        status = GMON_SENSOR_READ_FN_AIR_TEMP(air_temp, air_humid);
        if(status == GMON_RESP_OK) {
            gmon_aircond_last_recording_tick = curr_tick;
        }
    }
    return status;
} // end of staAirCondTrackRefreshSensorData


void staUpdateAirCondChkInterval(unsigned int netconn_interval, unsigned short num_records_kept)
{
    if(num_records_kept == 0) { num_records_kept = 1; }
    stationSysEnterCritical();
    gmon_aircond_chk_interval_tick = netconn_interval / (2 * num_records_kept * GMON_NUM_MILLISECONDS_PER_TICK);
    stationSysExitCritical();
} // end of staUpdateAirCondChkInterval


unsigned int staGetAirCondChkInterval(void)
{
    return gmon_aircond_chk_interval_tick;
} // end of staGetAirCondChkInterval

