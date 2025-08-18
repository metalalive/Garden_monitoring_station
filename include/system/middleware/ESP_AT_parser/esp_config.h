#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Add necessary includes for memory management functions
#include <stddef.h> // For size_t
#include <string.h> // For memset
#include <limits.h> // For SIZE_MAX (used in overflow check for calloc)

// Forward declarations for FreeRTOS memory functions, assuming they are defined elsewhere
// (e.g., in FreeRTOS.h or a specific port's header file)
extern void *pvPortMalloc(size_t xWantedSize);
extern void  vPortFree(void *pv);
void        *pvPortCalloc(size_t nmemb, size_t size);
void        *pvPortRealloc(void *old, size_t size);

// in this project we apply ESP-12, FreeRTOS port for STM32 Cortex-M4, turn off AT echo function
#define ESP_CFG_DEV_ESP12 1
#define ESP_CFG_SYS_PORT  ESP_SYS_PORT_FREERTOS
#define ESP_CFG_PING      1

// specify hardware reset pin instead of running AT+RST command
#define ESP_CFG_RST_PIN

// max bytes per packet size for this MQTT implementation
#define ESP_CFG_MAX_BYTES_PER_CIPSEND 1440

// DO NOT automatically reset / restore ESP device in eESPinit()
// we will do it manually separately
#define ESP_CFG_RESTORE_ON_INIT 0
#define ESP_CFG_RST_ON_INIT     0

// used FreeRTOS heap memory functions here because it seems more stable than
// the memory functions provided by cross-compile toolchain.
#define ESP_MALLOC(sizebytes) pvPortMalloc((size_t)(sizebytes))
#define ESP_MEMFREE(mptr)     vPortFree((void *)(mptr))

// implemented at here using FreeRTOS heap functions
#define ESP_CALLOC(nmemb, size)      pvPortCalloc(nmemb, (size_t)(size))
#define ESP_REALLOC(memptr, newsize) pvPortRealloc(memptr, (size_t)(newsize))

#ifdef __cplusplus
}
#endif
#endif // end of  __ESP_CONFIG_H
