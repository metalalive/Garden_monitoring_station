#ifndef STATION_DAYLIGHT_TRACK_H
#define STATION_DAYLIGHT_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

gMonStatus staDaylightTrackInit(gardenMonitor_t *gmon);

gMonStatus staSetRequiredDaylenTicks(gardenMonitor_t *gmon, unsigned int light_length);

void lightControllerTaskFn(void *params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_DAYLIGHT_TRACK_H
