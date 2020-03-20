#ifndef STATION_AIRCOND_TRACK_H
#define STATION_AIRCOND_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

#define  GMON_AIRCOND_RECORDING_INTERVAL_TICKS   (GMON_CFG_NETCONN_START_INTERVAL_MS / ((2 * GMON_CFG_NUM_SENSOR_RECORDS_KEEP) * GMON_NUM_MILLISECONDS_PER_TICK))

gMonStatus  staAirCondTrackInit(void);

gMonStatus  staAirCondTrackRefreshSensorData(float *air_temp, float *air_humid);

void staUpdateAirCondChkInterval(unsigned int netconn_interval, unsigned short num_records_kept);

unsigned int staGetAirCondChkInterval(void);


#ifdef __cplusplus
}
#endif
#endif // end of STATION_AIRCOND_TRACK_H
