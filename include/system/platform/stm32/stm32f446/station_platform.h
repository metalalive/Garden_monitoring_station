#ifndef STATION_PLATFORM_H
#define STATION_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

// these come from third-party STM32CubeMX library
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#define GMON_PLATFORM_PIN_DIRECTION_OUT  1 // from host microcontroller to wired sensor
#define GMON_PLATFORM_PIN_DIRECTION_IN   2 // from wired sensor to host microcontroller

#define GMON_PLATFORM_DISPLAY_SPI  1
#define GMON_PLATFORM_DISPLAY_I2C  2

#define GMON_PLATFORM_PIN_RESET  GPIO_PIN_RESET
#define GMON_PLATFORM_PIN_SET    GPIO_PIN_SET

gMonStatus  stationPlatformInit(void);
gMonStatus  stationPlatformDeinit(void);

gMonStatus  staSensorPlatformInitSoilMoist(void);
gMonStatus  staSensorPlatformDeInitSoilMoist(void);
gMonStatus  staPlatformReadSoilMoistSensor(unsigned int *out);

gMonStatus  staSensorPlatformInitAirTemp(void **pinstruct);
gMonStatus  staSensorPlatformDeInitAirTemp(void);

gMonStatus  staSensorPlatformInitLight(void);
gMonStatus  staSensorPlatformDeInitLight(void);
gMonStatus  staPlatformReadLightSensor(unsigned int *out);

gMonStatus  staOutDevPlatformInitPump(void **pinstruct);
gMonStatus  staOutDevPlatformInitFan(void **pinstruct);
gMonStatus  staOutDevPlatformInitBulb(void **pinstruct);
gMonStatus  staOutDevPlatformInitDisplay(uint8_t  comm_protocal_id, void **pinstruct);
gMonStatus  staOutDevPlatformDeinitDisplay(void *pinstruct);

gMonStatus  staPlatformPinSetDirection(void *pinstruct, uint8_t direction);

gMonStatus  staPlatformWritePin(void *pinstruct, uint8_t new_state);
uint8_t     staPlatformReadPin(void *pinstruct);

gMonStatus  staPlatformDelayUs(uint16_t us);

void*  staPlatformiGetDisplayRstPin(void);
void*  staPlatformiGetDisplayDataCmdPin(void);


#ifdef __cplusplus
}
#endif
#endif // end of STATION_PLATFORM_H
