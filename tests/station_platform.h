#ifndef STATION_PLATFORM_H
#define STATION_PLATFORM_H
#ifdef __cplusplus
extern "C" {
#endif

gMonStatus staSensorPlatformInitSoilMoist(gMonSensorMeta_t *);
gMonStatus staSensorPlatformDeInitSoilMoist(gMonSensorMeta_t *);
gMonStatus staPlatformReadSoilMoistSensor(gMonSensorMeta_t *, gmonSensorSample_t *) ;

#ifdef __cplusplus
}
#endif
#endif // end of STATION_PLATFORM_H
