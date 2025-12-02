#ifndef STATION_PLATFORM_H
#define STATION_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

// these come from third-party STM32CubeMX library
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#define GMON_PLATFORM_PIN_DIRECTION_OUT 1 // from host microcontroller to wired sensor
#define GMON_PLATFORM_PIN_DIRECTION_IN  2 // from wired sensor to host microcontroller

#define GMON_PLATFORM_DISPLAY_SPI 1
#define GMON_PLATFORM_DISPLAY_I2C 2

#define GMON_PLATFORM_PIN_RESET GPIO_PIN_RESET
#define GMON_PLATFORM_PIN_SET   GPIO_PIN_SET

// ---- functions that interface hardware implementation from application domain ----

gMonStatus stationPlatformInit(void);
gMonStatus stationPlatformDeinit(void);

gMonStatus staSensorPlatformInitSoilMoist(gMonSensor_t *);
gMonStatus staSensorPlatformDeInitSoilMoist(gMonSensor_t *);
gMonStatus staPlatformReadSoilMoistSensor(unsigned int *out);

gMonStatus staSensorPlatformInitAirTemp(gMonSensor_t *);
gMonStatus staSensorPlatformDeInitAirTemp(gMonSensor_t *);

gMonStatus staSensorPlatformInitLight(gMonSensor_t *);
gMonStatus staSensorPlatformDeInitLight(gMonSensor_t *);
gMonStatus staPlatformReadLightSensor(unsigned int *out);

gMonStatus staActuatorPlatformInitPump(void **pinstruct);
gMonStatus staActuatorPlatformInitFan(void **pinstruct);
gMonStatus staActuatorPlatformInitBulb(void **pinstruct);

gMonStatus staDisplayPlatformInit(uint8_t comm_protocal_id, void **pinstruct);
gMonStatus staDisplayPlatformDeinit(void *pinstruct);

gMonStatus staPlatformPinSetDirection(void *pinstruct, uint8_t direction);
gMonStatus staPlatformSPItransmit(void *pinstruct, unsigned char *pData, unsigned short sz);

gMonStatus staPlatformWritePin(void *pinstruct, uint8_t new_state);
uint8_t    staPlatformReadPin(void *pinstruct);

gMonStatus staPlatformDelayUs(uint16_t us);

// direction :
// - 1: transition from LOW to HIGH
// - 0: transition from HIGH to LOW
// us : pulse length in microseconds
gMonStatus staPlatformMeasurePulse(void *pinstruct, uint8_t *direction, uint16_t *us);

void *staPlatformiGetDisplayRstPin(void);
void *staPlatformiGetDisplayDataCmdPin(void);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_PLATFORM_H
