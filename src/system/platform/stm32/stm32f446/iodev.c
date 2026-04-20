#include "station_include.h"
#include "pin_map.h"

#define PLATFORM_ONE_MHZ      1000000
#define APP_APB2CLK_DIVIDER   RCC_HCLK_DIV1 // PCLK2 freq. == HCLK
#define NUM_AIRTEMP_DATA_PINS 2
#define NUM_LIGHTS_DATA_PINS  3
#define NUM_SOILS_DATA_PINS   4

typedef struct {
    SPI_HandleTypeDef *handler;
    hal_pinout_t       SCK;
    hal_pinout_t       MOSI;
    hal_pinout_t       MISO;
} hal_spi_pinout_t; // TODO I2C pinout structure

typedef struct {
    ADC_HandleTypeDef *reference;
    uint32_t           channel;
    uint8_t            app_sensor_id; // identity in upper application layer
} hal_extend_adc_t;

typedef struct {
    const hal_pinout_t *pwrctrl;
    union {
        const hal_extend_adc_t *adc;
        const hal_pinout_t     *gpio;
    } func;
    uint8_t len;
} hal_extend_t;

// timer that increments counter every 1 microsecond in non-blocking manner,
// all functions in this module accessing this timer are NOT thread-safe, callers
// must handle race condition by themselves.
static TIM_HandleTypeDef hal_tim_us;
static ADC_HandleTypeDef hadc1; // used as analog input of soil moisture sensor
static SPI_HandleTypeDef hspi2;
static DMA_HandleTypeDef hdma_adc1;
// PC14, PC15 are reserved for RCC LSE clock
static const hal_pinout_t hal_air_temp_read_pins[NUM_AIRTEMP_DATA_PINS] = {
    {HW_AIRTEMP_PORT, HW_AIRTEMP1_PIN, 0}, {HW_AIRTEMP_PORT, HW_AIRTEMP2_PIN, 0}
};
static const hal_pinout_t hal_pump_write_pin = {HW_PUMP_PORT, HW_PUMP_PIN, 0};
static const hal_pinout_t hal_fan_write_pin = {HW_FAN_PORT, HW_FAN_PIN, 0};
static const hal_pinout_t hal_bulb_write_pin = {HW_BULB_PORT, HW_BULB_PIN, 0};
static const hal_pinout_t hal_display_rst_pin = {HW_DISPLAY_RST_PORT, HW_DISPLAY_RST_PIN, 0};
static const hal_pinout_t hal_display_dc_pin = {HW_DISPLAY_DC_PORT, HW_DISPLAY_DC_PIN, 0};
static hal_spi_pinout_t   hal_display_spi_pins;
// power gating
static const hal_pinout_t pwrctrl_soilmoist = {HW_PWR_SOIL_MOISTURE_PORT, HW_PWR_SOIL_MOISTURE_PIN, 0};
static const hal_pinout_t pwrctrl_light = {HW_PWR_LIGHT_DETECT_PORT, HW_PWR_LIGHT_DETECT_PIN, 0};
static const hal_pinout_t pwrctrl_airtemp = {HW_PWR_AIRTEMP_DETECT_PORT, HW_PWR_AIRTEMP_DETECT_PIN, 0};

static const hal_extend_adc_t hal_extended_adc_devices[NUM_SOILS_DATA_PINS + NUM_LIGHTS_DATA_PINS] = {
    {.reference = &hadc1, .channel = HW_SOIL_MOISTURE_ADC_CH1, .app_sensor_id = 1},
    {.reference = &hadc1, .channel = HW_SOIL_MOISTURE_ADC_CH2, .app_sensor_id = 2},
    {.reference = &hadc1, .channel = HW_SOIL_MOISTURE_ADC_CH3, .app_sensor_id = 3},
    {.reference = &hadc1, .channel = HW_SOIL_MOISTURE_ADC_CH4, .app_sensor_id = 4},
    {.reference = &hadc1, .channel = HW_LIGHT_SENSOR_ADC_CH1, .app_sensor_id = 1},
    {.reference = &hadc1, .channel = HW_LIGHT_SENSOR_ADC_CH2, .app_sensor_id = 2},
    {.reference = &hadc1, .channel = HW_LIGHT_SENSOR_ADC_CH3, .app_sensor_id = 3},
};

static const hal_extend_t hal_extended_devices[3] = {
    {.func = {.adc = &hal_extended_adc_devices[0]}, .len = NUM_SOILS_DATA_PINS, .pwrctrl = &pwrctrl_soilmoist
    },
    {.func = {.adc = &hal_extended_adc_devices[4]}, .len = NUM_LIGHTS_DATA_PINS, .pwrctrl = &pwrctrl_light},
    {.func = {.gpio = &hal_air_temp_read_pins[0]}, .len = NUM_AIRTEMP_DATA_PINS, .pwrctrl = &pwrctrl_airtemp},
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

#define GPIOA_ADC1_PINS \
    (HW_SOIL_MOISTURE_1_PIN | HW_SOIL_MOISTURE_2_PIN | HW_SOIL_MOISTURE_3_PIN | HW_LIGHT_SENSOR_CH1_PIN)
#define GPIOB_ADC1_PINS (HW_SOIL_MOISTURE_4_PIN | HW_LIGHT_SENSOR_CH2_PIN)
#define GPIOC_ADC1_PINS (HW_LIGHT_SENSOR_CH3_PIN)
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // for power control pins
    GPIO_InitStruct.Pin = HW_PWR_SOIL_MOISTURE_PIN | HW_PWR_LIGHT_DETECT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(HW_PWR_SOIL_MOISTURE_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = HW_PWR_AIRTEMP_DETECT_PIN;
    HAL_GPIO_Init(HW_PWR_AIRTEMP_DETECT_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if (hadc->Instance == ADC1) {
        // Peripheral clock enable
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();
        // ADC1 GPIO Configuration
        // PA1 --> ADC1_IN1, PA4 --> ADC1_IN4, PA6 --> ADC1_IN6,
        // PA7 --> ADC1_IN7, PB0 --> ADC1_IN8, PB1 --> ADC1_IN9
        // PC4 --> ADC1_IN14
        GPIO_InitStruct.Pin = GPIOA_ADC1_PINS;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIOB_ADC1_PINS;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIOC_ADC1_PINS;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        HAL_GPIO_DeInit(GPIOA, GPIOA_ADC1_PINS);
        HAL_GPIO_DeInit(GPIOB, GPIOB_ADC1_PINS);
        HAL_GPIO_DeInit(GPIOC, GPIOC_ADC1_PINS);
        __HAL_RCC_ADC1_CLK_DISABLE();
        __HAL_RCC_DMA2_CLK_DISABLE();
    }
    HAL_GPIO_DeInit(HW_PWR_SOIL_MOISTURE_PORT, HW_PWR_SOIL_MOISTURE_PIN | HW_PWR_LIGHT_DETECT_PIN);
    HAL_GPIO_DeInit(HW_PWR_AIRTEMP_DETECT_PORT, HW_PWR_AIRTEMP_DETECT_PIN);
}
#undef GPIOA_ADC_PINS
#undef GPIOB_ADC_PINS
#undef GPIOC_ADC_PINS

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

static gMonStatus STM32_HAL_ADC1_Init(void) {
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    // in this proejct, multi-channel ADC will be configured, one of the channels is used for analog signal
    // of soil moisture sensor, 2 of the channels are used for analog signals of light-dependent resistors
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_10B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_StatusTypeDef status = HAL_ADC_Init(&hadc1);
    if (status != HAL_OK)
        return GMON_RESP_ERR;
    hdma_adc1.Instance = DMA2_Stream4;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_adc1.Init.MemDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_adc1.Init.Mode = DMA_NORMAL;
    hdma_adc1.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    status = HAL_DMA_Init(&hdma_adc1);
    if (status == HAL_OK) {
        __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
    }
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
    HAL_StatusTypeDef status = STM32_HAL_ADC1_Init();
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

gMonStatus staSensorPlatformInitSoilMoist(gMonSensorMeta_t *s) {
    if (s->num_items > hal_extended_devices[0].len)
        return GMON_RESP_ERRARGS;
    s->lowlvl = (void *)&hal_extended_devices[0];
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformDeInitSoilMoist(gMonSensorMeta_t *s) {
    s->lowlvl = NULL;
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformInitLight(gMonSensorMeta_t *s) {
    if (s->num_items > hal_extended_devices[1].len)
        return GMON_RESP_ERRARGS;
    s->lowlvl = (void *)&hal_extended_devices[1];
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformDeInitLight(gMonSensorMeta_t *s) {
    s->lowlvl = NULL;
    return GMON_RESP_OK;
}

static gMonStatus PowerCompGeneric(gMonSensorMeta_t *meta, uint8_t enable) {
    const hal_extend_t *ll_devs = (const hal_extend_t *)meta->lowlvl;
    return staPlatformWritePin((void *)ll_devs->pwrctrl, enable);
}
gMonStatus staSensorPlatformPowerUp(gMonSensorMeta_t *meta) { return PowerCompGeneric(meta, GPIO_PIN_SET); }
gMonStatus staSensorPlatformPowerDown(gMonSensorMeta_t *meta) {
    return PowerCompGeneric(meta, GPIO_PIN_RESET);
}

static void staResetADCregMap(ADC_TypeDef *regmap) {
    regmap->SQR1 &= ~(ADC_SQR1_L);
    regmap->CR1 &= ~(ADC_CR1_SCAN);
    regmap->CR2 &= ~(ADC_CR2_EOCS);
}

gMonStatus staPlatformReadSoilMoistSensor(gMonSensorMeta_t *sensor, gmonSensorSample_t *out) {
    if (sensor == NULL || out == NULL || sensor->num_items == 0)
        return GMON_RESP_ERRARGS;
    if (out->dtype != GMON_SENSOR_DATA_TYPE_U32)
        return GMON_RESP_ERRARGS;
    HAL_StatusTypeDef       hal_status = HAL_OK;
    const hal_extend_t     *ll_devs = (const hal_extend_t *)sensor->lowlvl;
    const hal_extend_adc_t *adc_devs = ll_devs->func.adc;

    unsigned short k = 0, max_oversample_len = 0;
    if (adc_devs == NULL)
        return GMON_RESP_ERRARGS;
    // the ADC registers below should be only modified once, otherwise ADC might
    // improperly sample input voltage state resulting in unstable read value.
    // Currently only ADC1 is applied so all references point to a single ADC1
    // instance.
    ADC_TypeDef *regmap = adc_devs[0].reference->Instance;
    staResetADCregMap(regmap);
    regmap->SQR1 |= ADC_SQR1(sensor->num_items);
    regmap->CR1 |= ADC_CR1_SCANCONV(ENABLE);
    regmap->CR2 |= ADC_CR2_EOCSelection(ADC_EOC_SEQ_CONV);

    for (k = 0; k < sensor->num_items; k++) {
        if (out[k].data == NULL || out[k].len == 0 || out[k].id != adc_devs[k].app_sensor_id)
            return GMON_RESP_ERRMEM;

        char enabled = staSensorPollEnabled((gMonSoilSensorMeta_t *)sensor, k);
        if (!enabled)
            continue;

        ADC_HandleTypeDef *adc_ref = adc_devs[k].reference;
        if (out[k].len > max_oversample_len)
            max_oversample_len = out[k].len;

        ADC_ChannelConfTypeDef sConfig = {
            .Channel = adc_devs[k].channel,
            .Rank = k + 1,
            .SamplingTime = ADC_SAMPLETIME_15CYCLES,
        };
        hal_status = HAL_ADC_ConfigChannel(adc_ref, &sConfig);
        if (hal_status != HAL_OK)
            return GMON_RESP_ERR;
    }
    // Perform resampling rounds with start/stop per round
    for (unsigned short m_round = 0; m_round < max_oversample_len; m_round++) {
        ADC_HandleTypeDef *adc_ref = adc_devs[k].reference;
        // Start ADC for this resampling round,
        // note in current implementation, all channels comes from the same
        // ADC1 components, simply start ADC1 at here.
        uint32_t dma_buffer[sensor->num_items];
        hal_status = HAL_ADC_Start_DMA(adc_ref, dma_buffer, sensor->num_items);
        if (hal_status != HAL_OK)
            break;
        // currently DMA interrupt is masked / suppressed  by upper application layer
        // caller, the workaround here is to polling EN bit in SxCR register to see
        // whether it is cleared by hardware (implicitly means end of transfer)
        while (HAL_IS_BIT_SET(adc_ref->DMA_Handle->Instance->CR, DMA_SxCR_EN))
            ;
        // TODO , error processing
        HAL_ADC_Stop_DMA(adc_ref);
        for (k = 0; k < sensor->num_items; k++) {
            if (m_round < out[k].len) {
                ((unsigned int *)out[k].data)[m_round] = dma_buffer[k];
            }
        }
    }
    return (hal_status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
} // end of staPlatformReadSoilMoistSensor

gMonStatus staPlatformReadLightSensor(gMonSensorMeta_t *sensor, gmonSensorSample_t *out) {
    if (sensor == NULL || out == NULL || sensor->num_items == 0)
        return GMON_RESP_ERRARGS;
    if (out->dtype != GMON_SENSOR_DATA_TYPE_U32)
        return GMON_RESP_ERRARGS;
    HAL_StatusTypeDef   hal_status = HAL_OK;
    const hal_extend_t *ll_devs = (const hal_extend_t *)sensor->lowlvl;
    if (ll_devs == NULL)
        return GMON_RESP_ERRARGS;

    const hal_extend_adc_t *adc_devs = ll_devs->func.adc;

    if (adc_devs == NULL)
        return GMON_RESP_ERRARGS;

    ADC_TypeDef *regmap = adc_devs[0].reference->Instance;
    staResetADCregMap(regmap);
    regmap->SQR1 |= ADC_SQR1(sensor->num_items);
    regmap->CR1 |= ADC_CR1_SCANCONV(DISABLE);
    regmap->CR2 |= ADC_CR2_EOCSelection(ADC_EOC_SINGLE_CONV);

    for (unsigned short k = 0; k < sensor->num_items; k++) {
        ADC_HandleTypeDef     *adc_ref = adc_devs[k].reference;
        ADC_ChannelConfTypeDef sConfig = {.Rank = 1, .SamplingTime = ADC_SAMPLETIME_144CYCLES};
        if (out[k].data == NULL || out[k].len == 0 || out[k].id != adc_devs[k].app_sensor_id)
            return GMON_RESP_ERRMEM;

        sConfig.Channel = adc_devs[k].channel;
        hal_status = HAL_ADC_ConfigChannel(adc_ref, &sConfig);
        if (hal_status != HAL_OK)
            goto done;
        for (unsigned short m_round = 0; m_round < out[k].len; m_round++) {
            hal_status = HAL_ADC_Start(adc_ref);
            if (hal_status != HAL_OK)
                break;
            hal_status = HAL_ADC_PollForConversion(adc_ref, 65);
            if (hal_status == HAL_OK) {
                uint32_t adc_value = HAL_ADC_GetValue(adc_ref);
                ((unsigned int *)out[k].data)[m_round] = adc_value;
            }
            HAL_ADC_Stop(adc_ref);
        }
    }
done:
    return (hal_status == HAL_OK ? GMON_RESP_OK : GMON_RESP_ERR);
} // end of staPlatformReadLightSensor

gMonStatus staSensorPlatformInitAirTemp(gMonSensorMeta_t *s) {
    if (s == NULL || s->num_items > hal_extended_devices[2].len)
        return GMON_RESP_ERRARGS;
    s->lowlvl = (void *)&hal_extended_devices[2];
    return GMON_RESP_OK;
}

gMonStatus staSensorPlatformDeInitAirTemp(gMonSensorMeta_t *s) {
    s->lowlvl = NULL;
    return GMON_RESP_OK;
}

gMonStatus staActuatorPlatformInitPump(void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    *(const hal_pinout_t **)pinstruct = &hal_pump_write_pin;
    return GMON_RESP_OK;
}

gMonStatus staActuatorPlatformInitFan(void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    *(const hal_pinout_t **)pinstruct = &hal_fan_write_pin;
    return GMON_RESP_OK;
}

gMonStatus staActuatorPlatformInitBulb(void **pinstruct) {
    if (pinstruct == NULL) {
        return GMON_RESP_ERRARGS;
    }
    *(const hal_pinout_t **)pinstruct = &hal_bulb_write_pin;
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
    const hal_pinout_t *hal_pinstruct = (const hal_pinout_t *)pinstruct;
    uint32_t            current_counter;
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

void *staPlatformFindIOpin(void *lowlvl, uint8_t idx) {
    void *out = NULL;
    if (lowlvl == NULL)
        return out;
    const hal_extend_t *ll_devs = (const hal_extend_t *)lowlvl;
    if (idx < ll_devs->len)
        out = (void *)&ll_devs->func.gpio[idx];
    return out;
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
    const hal_pinout_t *hal_pinstruct = (const hal_pinout_t *)pinstruct;
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
