#ifndef STATION_DAYLIGHT_TRACK_H
#define STATION_DAYLIGHT_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus  staDaylightTrackInit(void);

gMonStatus  staDaylightTrackRefreshSensorData(unsigned int *out);

gMonStatus  staSetRequiredDaylenTicks(unsigned int light_length);

unsigned int  staRefreshRequiredLightLength(gMonOutDevStatus bulb_status, unsigned int actual_light_ticks);

unsigned int  staGetTicksSinceLastLightRecording(void);

void staUpdateLightChkInterval(unsigned int netconn_interval, unsigned short num_records_kept);

unsigned int staGetLightChkInterval(void);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_DAYLIGHT_TRACK_H
