#ifndef STATION_SOILCOND_TRACK_H
#define STATION_SOILCOND_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

gmonSensorSample_t *staAllocSensorSampleBuffer(gMonSensorMeta_t *, gmonSensorDataType_t);

void pumpControllerTaskFn(void *params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_SOILCOND_TRACK_H
