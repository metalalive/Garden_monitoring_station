#ifndef STATION_AIRCOND_TRACK_H
#define STATION_AIRCOND_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus staAirCondTrackInit(void);

void airQualityMonitorTaskFn(void *params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_AIRCOND_TRACK_H
