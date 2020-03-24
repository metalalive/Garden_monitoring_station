#ifndef STATION_AIRCOND_TRACK_H
#define STATION_AIRCOND_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif


gMonStatus  staAirCondTrackInit(void);

gMonStatus  staAirCondTrackRefreshSensorData(float *air_temp, float *air_humid);

void staUpdateAirCondChkInterval(unsigned int netconn_interval, unsigned short num_records_kept);

unsigned int staGetAirCondChkInterval(void);

unsigned int  staGetTicksSinceLastAirCondRecording(void);


#ifdef __cplusplus
}
#endif
#endif // end of STATION_AIRCOND_TRACK_H
