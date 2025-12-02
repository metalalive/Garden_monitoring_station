#include "station_include.h"
#include "pin_map.h"

#define PLATFORM_ONE_MHZ    1000000
#define APP_APB2CLK_DIVIDER RCC_HCLK_DIV1 // PCLK2 freq. == HCLK

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       alternate;
} hal_pinout_t;

typedef struct {
    SPI_HandleTypeDef *handler;
    hal_pinout_t       SCK;
    hal_pinout_t       MOSI;
    hal_pinout_t       MISO;
} hal_spi_pinout_t; // TODO I2C pinout structure

typedef struct {
    ADC_HandleTypeDef *reference;
    uint32_t           channel;
} hal_extend_adc_t;

// timer that increments counter every 1 microsecond in non-blocking manner,
// all functions in this module accessing this timer are NOT thread-safe, callers
// must handle race condition by themselves.
static TIM_HandleTypeDef hal_tim_us;
static ADC_HandleTypeDef hadc1; // used as analog input of soil moisture sensor
static SPI_HandleTypeDef hspi2;
// PC14, PC15 are reserved for RCC LSE clock
static hal_pinout_t     hal_air_temp_read_pin = {HW_AIRTEMP_PORT, HW_AIRTEMP_PIN, 0};
static hal_pinout_t     hal_pump_write_pin = {HW_PUMP_PORT, HW_PUMP_PIN, 0};
static hal_pinout_t     hal_fan_write_pin = {HW_FAN_PORT, HW_FAN_PIN, 0};
static hal_pinout_t     hal_bulb_write_pin = {HW_BULB_PORT, HW_BULB_PIN, 0};
static hal_pinout_t     hal_display_rst_pin = {HW_DISPLAY_RST_PORT, HW_DISPLAY_RST_PIN, 0};
static hal_pinout_t     hal_display_dc_pin = {HW_DISPLAY_DC_PORT, HW_DISPLAY_DC_PIN, 0};
static hal_spi_pinout_t hal_display_spi_pins;

static const hal_extend_adc_t hal_extended_adc_devices[3] = {
    {.reference = &hadc1, .channel = HW_SOIL_MOISTURE_ADC_CH},
    {.reference = &hadc1, .channel = HW_LIGHT_SENSOR_ADC_CH1},
    {.reference = &hadc1, .channel = HW_LIGHT_SENSOR_ADC_CH2},
};

HAL_StatusTypeDef SystemClock_Config(void) {
    HAL_StatusTypeDef        status = HAL_OK;
    RCC_OscInitTypeDef       RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef       RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    // Configure the main internal regulator output voltage
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
    // Initializes the CPU, AHB and APB busses clocks
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 80;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    status = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (status != HAL_OK) {
        goto done;
    }
    // Initializes the CPU, AHB and APB busses clocks
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = APP_APB2CLK_DIVIDER;
    status = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
    if (status != HAL_OK) {
        goto done;
    }
    // initialize clocks for RTC
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    status = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
done:
    return status;
} // end of SystemClock_Config

static HAL_StatusTypeDef STM32_HAL_timer_us_Init(void) {
    HAL_StatusTypeDef       status = HAL_OK;
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    // Note timer 1 is clocked by PCLK2
    // PCLK2 freq. == HCLK , which means APB2 clock prescaler is zero ,
    // no need to double the timer frequency here
    uint32_t tim1_clk_hz = HAL_RCC_GetPCLK2Freq();
    if (APP_APB2CLK_DIVIDER != RCC_HCLK_DIV1) {
        tim1_clk_hz = tim1_clk_hz << 1;
    }
    hal_tim_us.Instance = TIM1;
    hal_tim_us.Init.Prescaler = (tim1_clk_hz / PLATFORM_ONE_MHZ) - 1;
    hal_tim_us.Init.CounterMode = TIM_COUNTERMODE_UP;
    hal_tim_us.Init.Period = 0xffff - 1;
    hal_tim_us.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    hal_tim_us.Init.RepetitionCounter = 0;
    status = HAL_TIM_Base_Init(&hal_tim_us);
    if (status != HAL_OK) {
        goto done;
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    status = HAL_TIM_ConfigClockSource(&hal_tim_us, &sClockSourceConfig);
    if (status != HAL_OK) {
        goto done;
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    status = HAL_TIMEx_MasterConfigSynchronization(&hal_tim_us, &sMasterConfig);
done:
    return status;
} // end of STM32_HAL_timer_us_Init

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM1) {
        // Peripheral clock enable
        __HAL_RCC_TIM1_CLK_ENABLE();
    }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1) {
        // Peripheral clock enable
        __HAL_RCC_ADC1_CLK_ENABLE();
        // ADC1 GPIO Configuration
        // PA6     ------> ADC1_IN6
        // PA7     ------> ADC1_IN7
        // PB1     ------> ADC1_IN9
        GPIO_InitStruct.Pin = HW_SOIL_MOISTURE_PIN | HW_LIGHT_SENSOR_CH1_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = HW_LIGHT_SENSOR_CH2_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_DISABLE();
        // ADC1 GPIO Configuration
        // PA6     ------> ADC1_IN6
        // PA7     ------> ADC1_IN7
        // PB1     ------> ADC1_IN9
        HAL_GPIO_DeInit(GPIOA, HW_SOIL_MOISTURE_PIN | HW_LIGHT_SENSOR_CH1_PIN);
        HAL_GPIO_DeInit(GPIOB, HW_LIGHT_SENSOR_CH2_PIN);
    }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hspi->Instance == SPI2) {
        __HAL_RCC_SPI2_CLK_ENABLE();
        // SPI2 GPIO Configuration
        // PC3     ------> SPI2_MOSI
        // PB13    ------> SPI2_SCK
        GPIO_InitStruct.Pin = hal_display_spi_pins.MOSI.pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = hal_display_spi_pins.MOSI.alternate;
        HAL_GPIO_Init(hal_display_spi_pins.MOSI.port, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = hal_display_spi_pins.SCK.pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = hal_display_spi_pins.SCK.alternate;
        HAL_GPIO_Init(hal_display_spi_pins.SCK.port, &GPIO_InitStruct);
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        // SPI2 GPIO Configuration
        // PC3     ------> SPI2_MOSI
        // PB13    ------> SPI2_SCK
        HAL_GPIO_DeInit(hal_display_spi_pins.MOSI.port, hal_display_spi_pins.MOSI.pin);
        HAL_GPIO_DeInit(hal_display_spi_pins.SCK.port, hal_display_spi_pins.SCK.pin);
        __HAL_RCC_SPI2_CLK_DISABLE();
    }
}

static gMonStatus STM32_HAL_ADC1_Init(unsigned short num_devices) {
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    // in this proejct, multi-channel ADC will be configured, one of the channels is used for analog signal
    // of soil moisture sensor, 2 of the channels are used for analog signals of light-dependent resistors
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_10B;
    hadc1.Init.ScanConvMode = ENABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = num_devices;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_StatusTypeDef status = HAL_ADC_Init(&hadc1);
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

static gMonStatus STM32_HAL_SPI2_Init(void) {
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_1LINE;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    HAL_StatusTypeDef status = HAL_SPI_Init(&hspi2);
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

// in this application , `HAL_Init` and `SystemClock_Config` are executed during
// network initialization through `mqttClientInit` .
gMonStatus stationPlatformInit(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    unsigned short    num_analog_pins = sizeof(hal_extended_adc_devices);
    HAL_StatusTypeDef status = STM32_HAL_ADC1_Init(num_analog_pins);
    if (status != HAL_OK) {
        goto done;
    }
    status = STM32_HAL_timer_us_Init(); // initialize 1 us timer
    if (status != HAL_OK) {
        goto done;
    }
    status = HAL_TIM_Base_Start(&hal_tim_us);
done:
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

gMonStatus stationPlatformDeinit(void) {
    HAL_StatusTypeDef status = HAL_TIM_Base_Stop(&hal_tim_us);
    if (status != HAL_OK) {
        goto done;
    }
    status = HAL_ADC_DeInit(&hadc1);
done:
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

gMonStatus staSensorPlatformInitSoilMoist(gMonSensor_t *s) {
    s->lowlvl = (void *)&hal_extended_adc_devices[0];
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformDeInitSoilMoist(gMonSensor_t *s) {
    s->lowlvl = NULL;
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformInitLight(gMonSensor_t *s) {
    s->lowlvl = (void *)&hal_extended_adc_devices[1];
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformDeInitLight(gMonSensor_t *s) {
    s->lowlvl = NULL;
    return GMON_RESP_OK;
}

gMonStatus staPlatformReadSoilMoistSensor(unsigned int *out) {
    // get status of analog pin by polling the sensor
    if (out == NULL)
        return GMON_RESP_ERRARGS;
    HAL_StatusTypeDef      status = HAL_OK;
    ADC_ChannelConfTypeDef sConfig = {0};
    // Configure for the selected ADC regular channel its corresponding rank
    // in the sequencer and its sample time.
    sConfig.Channel = HW_SOIL_MOISTURE_ADC_CH;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    if (status != HAL_OK)
        goto done;
    unsigned int timeout = 100; // time ticks
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK)
        goto done;
    status = HAL_ADC_PollForConversion(&hadc1, timeout);
    if (status != HAL_OK)
        goto done;
    *out = HAL_ADC_GetValue(&hadc1);
done:
    HAL_ADC_Stop(&hadc1);
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
} // end of staPlatformReadSoilMoistSensor

gMonStatus staPlatformReadLightSensor(unsigned int *out) {
    volatile uint32_t light_rd_data[2] = {0, 0};
    unsigned int      timeout = 100; // time ticks
    HAL_StatusTypeDef status = HAL_OK;
    if (out == NULL)
        return GMON_RESP_ERRARGS;
    ADC_ChannelConfTypeDef sConfig = {0};
    // --------------
    // Configure for the selected ADC regular channel its corresponding rank
    // in the sequencer and its sample time.
    sConfig.Channel = HW_LIGHT_SENSOR_ADC_CH1;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    if (status != HAL_OK)
        goto done;
    sConfig.Channel = HW_LIGHT_SENSOR_ADC_CH2;
    sConfig.Rank = 2;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    if (status != HAL_OK)
        goto done;
    // --------------
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK)
        goto done;
    // workaround: skip first one : analog signal of soil moisture sensor
    // HAL_ADC_PollForConversion(&hadc1, timeout);
    status = HAL_ADC_PollForConversion(&hadc1, timeout);
    if (status != HAL_OK)
        goto done;
    light_rd_data[0] = HAL_ADC_GetValue(&hadc1);
    status = HAL_ADC_PollForConversion(&hadc1, timeout);
    if (status != HAL_OK)
        goto done;
    light_rd_data[1] = HAL_ADC_GetValue(&hadc1);
done:
    HAL_ADC_Stop(&hadc1);
    if (status == HAL_OK) {
        *out = (light_rd_data[0] + light_rd_data[1]) / 2;
    }
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

gMonStatus staSensorPlatformInitAirTemp(gMonSensor_t *s) {
    if (s == NULL)
        return GMON_RESP_ERRARGS;
    void **pinstruct = &s->lowlvl;
    *(hal_pinout_t **)pinstruct = &hal_air_temp_read_pin;
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformDeInitAirTemp(gMonSensor_t *s) {
    s->lowlvl = NULL;
    return GMON_RESP_OK;
}

gMonStatus staActuatorPlatformInitPump(void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    *(hal_pinout_t **)pinstruct = &hal_pump_write_pin;
    return GMON_RESP_OK;
}

gMonStatus staActuatorPlatformInitFan(void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    *(hal_pinout_t **)pinstruct = &hal_fan_write_pin;
    return GMON_RESP_OK;
}

gMonStatus staActuatorPlatformInitBulb(void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    *(hal_pinout_t **)pinstruct = &hal_bulb_write_pin;
    return GMON_RESP_OK;
}

gMonStatus staDisplayPlatformInit(uint8_t comm_protocal_id, void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    gMonStatus status = GMON_RESP_OK;
    switch (comm_protocal_id) {
    case GMON_PLATFORM_DISPLAY_SPI:
        hal_display_spi_pins.handler = &hspi2;
        hal_display_spi_pins.SCK.port = HW_DISPLAY_SPI_SCK_PORT;
        hal_display_spi_pins.SCK.pin = HW_DISPLAY_SPI_SCK_PIN;
        hal_display_spi_pins.SCK.alternate = HW_DISPLAY_SPI_SCK_AF;
        hal_display_spi_pins.MOSI.port = HW_DISPLAY_SPI_MOSI_PORT;
        hal_display_spi_pins.MOSI.pin = HW_DISPLAY_SPI_MOSI_PIN;
        hal_display_spi_pins.MOSI.alternate = HW_DISPLAY_SPI_MOSI_AF;
        hal_display_spi_pins.MISO.port = NULL;
        hal_display_spi_pins.MISO.pin = 0;
        hal_display_spi_pins.MISO.alternate = 0;
        status = STM32_HAL_SPI2_Init();
        if (status != GMON_RESP_OK) {
            break;
        }
        *(hal_spi_pinout_t **)pinstruct = &hal_display_spi_pins;
        break;
    case GMON_PLATFORM_DISPLAY_I2C: // TODO
    default:
        status = GMON_RESP_ERR_NOT_SUPPORT;
        break;
    } // end of switch case
    return status;
} // end of staDisplayPlatformInit

void *staPlatformiGetDisplayRstPin(void) { return (void *)&hal_display_rst_pin; }

void *staPlatformiGetDisplayDataCmdPin(void) { return (void *)&hal_display_dc_pin; }

gMonStatus staDisplayPlatformDeinit(void *pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    HAL_StatusTypeDef status = HAL_OK;
    if (pinstruct == &hal_display_spi_pins) {
        status = HAL_SPI_DeInit(hal_display_spi_pins.handler);
    }
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

gMonStatus staPlatformSPItransmit(void *pinstruct, unsigned char *pData, unsigned short sz) {
    if (pinstruct == NULL || pData == NULL || sz == 0) {
        return GMON_RESP_ERRARGS;
    }
    hal_spi_pinout_t *spi = (hal_spi_pinout_t *)pinstruct;
    HAL_StatusTypeDef status = HAL_SPI_Transmit(spi->handler, pData, sz, HAL_MAX_DELAY);
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
}

gMonStatus staPlatformDelayUs(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&hal_tim_us, 0);
    while (__HAL_TIM_GET_COUNTER(&hal_tim_us) < us)
        ;
    return GMON_RESP_OK;
}

gMonStatus staPlatformMeasurePulse(void *pinstruct, uint8_t *direction, uint16_t *us) {
    if (pinstruct == NULL || direction == NULL || us == NULL) {
        return GMON_RESP_ERRARGS;
    }
    hal_pinout_t *hal_pinstruct = (hal_pinout_t *)pinstruct;
    uint32_t      current_counter;
    // reset timer to zero
    __HAL_TIM_SET_COUNTER(&hal_tim_us, 0);
    // read initial state of the given GPIO pin pinstruct, record it to a local variable, say s0
    GPIO_PinState s0 = HAL_GPIO_ReadPin(hal_pinstruct->port, hal_pinstruct->pin);
    // read the same GPIO pin in a loop until the state is changed.
    do {
        current_counter = __HAL_TIM_GET_COUNTER(&hal_tim_us);
        // if the timer value is about to exceed its max representable value, return appropriate error
        if (current_counter >= 0xFF80) { // max limited period is 0xFF80. Check before it wraps.
            return GMON_RESP_TIMEOUT;    // Represents a "read sensor timeout"
        }
    } while (HAL_GPIO_ReadPin(hal_pinstruct->port, hal_pinstruct->pin) == s0);
    // retrieve timer value , write it to given argument us
    *us = (uint16_t)current_counter;
    *direction = (s0 == GPIO_PIN_RESET) ? 1 : 0;
    return GMON_RESP_OK;
}

gMonStatus staPlatformPinSetDirection(void *pinstruct, uint8_t direction) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    hal_pinout_t     *hal_pinstruct = NULL;
    GPIO_InitTypeDef  GPIO_InitStruct = {0};
    HAL_StatusTypeDef status = HAL_OK;

    hal_pinstruct = (hal_pinout_t *)pinstruct;
    HAL_GPIO_DeInit(hal_pinstruct->port, hal_pinstruct->pin);
    switch (direction) {
    case GMON_PLATFORM_PIN_DIRECTION_OUT:
        // Configure GPIO pin Output Level
        HAL_GPIO_WritePin(hal_pinstruct->port, hal_pinstruct->pin, GPIO_PIN_RESET);
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        break;
    case GMON_PLATFORM_PIN_DIRECTION_IN:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL; // GPIO_PULLDOWN;
        break;
    default:
        status = HAL_ERROR;
        break;
    } // end of switch case
    if (status == HAL_OK) {
        GPIO_InitStruct.Pin = hal_pinstruct->pin;
        HAL_GPIO_Init(hal_pinstruct->port, &GPIO_InitStruct);
    }
    return (status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
} // end of staPlatformPinSetDirection

gMonStatus staPlatformWritePin(void *pinstruct, uint8_t new_state) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    hal_pinout_t *hal_pinstruct = NULL;
    hal_pinstruct = (hal_pinout_t *)pinstruct;
    HAL_GPIO_WritePin(hal_pinstruct->port, hal_pinstruct->pin, new_state);
    return GMON_RESP_OK;
}

uint8_t staPlatformReadPin(void *pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    hal_pinout_t *hal_pinstruct = NULL;
    hal_pinstruct = (hal_pinout_t *)pinstruct;
    return (uint8_t)HAL_GPIO_ReadPin(hal_pinstruct->port, hal_pinstruct->pin);
}
