#ifndef STATION_HW_PIN_MAP_H
#define STATION_HW_PIN_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

// ----- pins which cannot be used in STM32F446 board -----
// - PA13 / PA14 are tied with serial-wire debug (SWD) protocol
//   on SL-LINK, both pinouts are manufactued but MUST NOT be used as GPIO pins.
//   since this will activate JTAG pins causing on-chip debug corruption
// - PC14 / PC15 are tied with low-speed external clock module (LSE) and the
//   relevant solder bridges `SB48` / `SB49` are OFF by default . To make it
//   function as GPIO pins, add bridges on `SB48` / `SB49`

// ---- hardware pins mapping to application domain ----
// Sensor Pin Assignments
#define HW_SOIL_MOISTURE_1_PIN   GPIO_PIN_6 // PA6
#define HW_SOIL_MOISTURE_2_PIN   GPIO_PIN_1 // PA1
#define HW_SOIL_MOISTURE_3_PIN   GPIO_PIN_4 // PA4
#define HW_SOIL_MOISTURE_4_PIN   GPIO_PIN_0 // PB0
#define HW_SOIL_MOISTURE_ADC_CH1 ADC_CHANNEL_6
#define HW_SOIL_MOISTURE_ADC_CH2 ADC_CHANNEL_1
#define HW_SOIL_MOISTURE_ADC_CH3 ADC_CHANNEL_4
#define HW_SOIL_MOISTURE_ADC_CH4 ADC_CHANNEL_8

#define HW_PWR_SOIL_MOISTURE_PORT  GPIOA // PA12
#define HW_PWR_SOIL_MOISTURE_PIN   GPIO_PIN_12
#define HW_PWR_LIGHT_DETECT_PORT   GPIOA // PA11
#define HW_PWR_LIGHT_DETECT_PIN    GPIO_PIN_11
#define HW_PWR_AIRTEMP_DETECT_PORT GPIOB // PB2
#define HW_PWR_AIRTEMP_DETECT_PIN  GPIO_PIN_2

#define HW_LIGHT_SENSOR_CH1_PIN GPIO_PIN_7 // PA7
#define HW_LIGHT_SENSOR_CH2_PIN GPIO_PIN_1 // PB1
#define HW_LIGHT_SENSOR_CH3_PIN GPIO_PIN_4 // PC4
#define HW_LIGHT_SENSOR_ADC_CH1 ADC_CHANNEL_7
#define HW_LIGHT_SENSOR_ADC_CH2 ADC_CHANNEL_9
#define HW_LIGHT_SENSOR_ADC_CH3 ADC_CHANNEL_14

#define HW_AIRTEMP_PORT GPIOB
#define HW_AIRTEMP1_PIN GPIO_PIN_8
#define HW_AIRTEMP2_PIN GPIO_PIN_12

// Actuator Pin Assignments
#define HW_PUMP_PORT  GPIOA
#define HW_PUMP_1_PIN GPIO_PIN_8
#define HW_PUMP_2_PIN GPIO_PIN_9
#define HW_PUMP_3_PIN GPIO_PIN_10
#define HW_PUMP_4_PIN GPIO_PIN_15

#define HW_FAN_PORT  GPIOC
#define HW_FAN_1_PIN GPIO_PIN_6

#define HW_BULB_PORT  GPIOC
#define HW_BULB_1_PIN GPIO_PIN_7
#define HW_BULB_2_PIN GPIO_PIN_12
#define HW_BULB_3_PIN GPIO_PIN_13

// Display Pin Assignments
#define HW_DISPLAY_RST_PORT      GPIOB
#define HW_DISPLAY_RST_PIN       GPIO_PIN_14
#define HW_DISPLAY_DC_PORT       GPIOB
#define HW_DISPLAY_DC_PIN        GPIO_PIN_15
#define HW_DISPLAY_SPI_SCK_PORT  GPIOB
#define HW_DISPLAY_SPI_SCK_PIN   GPIO_PIN_13
#define HW_DISPLAY_SPI_SCK_AF    GPIO_AF5_SPI2
#define HW_DISPLAY_SPI_MOSI_PORT GPIOC
#define HW_DISPLAY_SPI_MOSI_PIN  GPIO_PIN_3
#define HW_DISPLAY_SPI_MOSI_AF   GPIO_AF5_SPI2

// Network Device (ESP8266) Pin Assignments
#define HW_ESP8266_UART_RX_PORT GPIOC
#define HW_ESP8266_UART_RX_PIN  GPIO_PIN_5
#define HW_ESP8266_UART_RX_AF   GPIO_AF7_USART3
#define HW_ESP8266_UART_TX_PORT GPIOB
#define HW_ESP8266_UART_TX_PIN  GPIO_PIN_10
#define HW_ESP8266_UART_TX_AF   GPIO_AF7_USART3
#define HW_ESP8266_RST_PORT     ESP8266_RST_PINGRP
#define HW_ESP8266_RST_PIN      ESP8266_RST_PINNUM
// - `PH0` on STM32 board is always pulled HIGH and connected to `CH_PD` and `GPIO0`
//   pins of ESP device.
// - `PH1` on STM32 board is toggled to control remote power switch,
//   (e.g. MOSFET, relay, etc...)
#define HW_ESP8266_CHIPENABLE_PORT GPIOH
#define HW_ESP8266_CHIPENABLE_PIN  GPIO_PIN_0
#define HW_ESP8266_POWERGATE_PORT  GPIOH
#define HW_ESP8266_POWERGATE_PIN   GPIO_PIN_1

// Entropy Source (HCSR04) Pin Assignments
#define HW_ENTROPY_TRIG_PORT ENTROPY_HCSR04_OUT_GRP
#define HW_ENTROPY_TRIG_PIN  ENTROPY_HCSR04_OUT_PINNUM
#define HW_ENTROPY_ECHO_PORT ENTROPY_HCSR04_IN_GRP
#define HW_ENTROPY_ECHO_PIN  ENTROPY_HCSR04_IN_PINNUM

#ifdef __cplusplus
}
#endif
#endif // end of STATION_HW_PIN_MAP_H
