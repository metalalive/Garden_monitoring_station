#ifndef STATION_PLATFORM_H
#define STATION_PLATFORM_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define GMON_PLATFORM_PIN_DIRECTION_OUT 1 // from host microcontroller to wired sensor
#define GMON_PLATFORM_PIN_DIRECTION_IN  2 // from wired sensor to host microcontroller

#define GMON_PLATFORM_PIN_RESET 0
#define GMON_PLATFORM_PIN_SET   1

gMonStatus staSensorPlatformInitSoilMoist(gMonSensorMeta_t *);
gMonStatus staSensorPlatformDeInitSoilMoist(gMonSensorMeta_t *);
gMonStatus staPlatformReadSoilMoistSensor(gMonSensorMeta_t *, gmonSensorSample_t *) ;

gMonStatus staSensorPlatformInitLight(gMonSensorMeta_t *);
gMonStatus staSensorPlatformDeInitLight(gMonSensorMeta_t *);
gMonStatus staPlatformReadLightSensor(gMonSensorMeta_t *, gmonSensorSample_t *out);

gMonStatus staSensorPlatformInitAirTemp(gMonSensorMeta_t *);
gMonStatus staSensorPlatformDeInitAirTemp(gMonSensorMeta_t *);
gMonStatus staPlatformMeasurePulse(void *pinstruct, uint8_t *direction, uint16_t *us);
gMonStatus staPlatformPinSetDirection(void *pinstruct, uint8_t direction);
gMonStatus staPlatformWritePin(void *pinstruct, uint8_t new_state);

gMonStatus stationSysDelayUs(unsigned short time_us);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_PLATFORM_H
