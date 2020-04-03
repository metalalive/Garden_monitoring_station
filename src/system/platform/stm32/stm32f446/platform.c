#include "station_include.h"

#define PLATFORM_ONE_MHZ  1000000

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       alternate;
} hal_pinout_t;

typedef struct {
    SPI_HandleTypeDef  *handler;
    hal_pinout_t  SCK;
    hal_pinout_t  MOSI;
    hal_pinout_t  MISO;
} hal_spi_pinout_t; // TODO I2C pinout structure

static TIM_HandleTypeDef  hal_tim_us; // timer that increment counter every 1 microsecond
static ADC_HandleTypeDef  hadc1; // used as analog input of soil moisture sensor
static SPI_HandleTypeDef  hspi2;
static hal_pinout_t       hal_air_temp_read_pin = {GPIOB, GPIO_PIN_8, 0};
static hal_pinout_t       hal_pump_write_pin = {GPIOC, GPIO_PIN_13, 0};
static hal_pinout_t       hal_fan_write_pin  = {GPIOC, GPIO_PIN_1, 0}; // PC14, PC15 are reserved for RCC LSE clock
static hal_pinout_t       hal_bulb_write_pin = {GPIOC, GPIO_PIN_0, 0};
static hal_pinout_t       hal_display_rst_pin = {GPIOB, GPIO_PIN_14, 0};
static hal_pinout_t       hal_display_dc_pin  = {GPIOB, GPIO_PIN_15, 0};
static hal_spi_pinout_t   hal_display_spi_pins;


#ifndef GMON_CFG_SKIP_PLATFORM_INIT
//
// @brief This function configures the source of the time base.
//        The time source is configured  to have 1ms time base with a dedicated 
//        Tick interrupt priority.
// @note This function is called  automatically at the beginning of program after
//       reset by HAL_Init() or at any time when clock is reconfigured  by HAL_RCC_ClockConfig().
// @note In the default implementation, SysTick timer is the source of time base. 
//       It is used to generate interrupts at regular time intervals. 
//       Care must be taken if HAL_Delay() is called from a peripheral ISR process, 
//       The SysTick interrupt must have higher priority (numerically lower)
//       than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
//       The function is declared as __weak  to be overwritten  in case of other
//       implementation  in user file.
// @param TickPriority Tick interrupt priority.
// @retval HAL status
//
__weak HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  uwTickFreq = HAL_TICK_FREQ_DEFAULT;  // 1KHz
  // Configure the SysTick to have interrupt in 1ms time basis
  if (HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / (1000U / uwTickFreq)) > 0U) {
      return HAL_ERROR;
  }
  // Configure the SysTick IRQ priority
  if (TickPriority < (1UL << __NVIC_PRIO_BITS)) {
      HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
      uwTickPrio = TickPriority;
  } else {
      return HAL_ERROR;
  }
  return HAL_OK; // Return function status
} // end of HAL_InitTick


static void STM32_HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

static HAL_StatusTypeDef STM32_HAL_Init(void)
{
    // Configure Flash prefetch, Instruction cache, Data cache
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    // Set Interrupt Group Priority 
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    // Use systick as time base source and configure 1ms tick (default clock after Reset is HSI) 
    HAL_InitTick(TICK_INT_PRIORITY);
    // Init the low level hardware 
    STM32_HAL_MspInit();
    return HAL_OK;
} // end of STM32_HAL_Init


static HAL_StatusTypeDef SystemClock_Config(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
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
    if (status != HAL_OK) { goto done; }
    // Initializes the CPU, AHB and APB busses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    status =  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
    if (status != HAL_OK) { goto done;  }
    // initialize clocks for RTC
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    status = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
done:
    return status;
} // end of SystemClock_Config
#endif  // end of GMON_CFG_SKIP_PLATFORM_INIT


static HAL_StatusTypeDef  STM32_HAL_timer_us_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    hal_tim_us.Instance = TIM1;
    hal_tim_us.Init.Prescaler = (HAL_RCC_GetHCLKFreq() / (2 * PLATFORM_ONE_MHZ)) - 1; // TODO: figure out how prescaler works in STM32 timer
    hal_tim_us.Init.CounterMode = TIM_COUNTERMODE_UP;
    hal_tim_us.Init.Period = 0xffff - 1;
    hal_tim_us.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    hal_tim_us.Init.RepetitionCounter = 0;
    status = HAL_TIM_Base_Init(&hal_tim_us);
    if(status != HAL_OK) { goto done; }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    status = HAL_TIM_ConfigClockSource(&hal_tim_us, &sClockSourceConfig);
    if(status != HAL_OK) { goto done; }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    status = HAL_TIMEx_MasterConfigSynchronization(&hal_tim_us, &sMasterConfig);
done:
    return status;
} // end of STM32_HAL_timer_us_Init


void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance==TIM1) {
        // Peripheral clock enable
        __HAL_RCC_TIM1_CLK_ENABLE();
    }
} // end of HAL_TIM_Base_MspInit


void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hadc->Instance == ADC1) {
        // Peripheral clock enable
        __HAL_RCC_ADC1_CLK_ENABLE();
        // ADC1 GPIO Configuration    
        // PA6     ------> ADC1_IN6 
        // PA7     ------> ADC1_IN7
        // PB1     ------> ADC1_IN9 
        GPIO_InitStruct.Pin  = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin  = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
} // end of HAL_ADC_MspInit


void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1) {
        // Peripheral clock disable
        __HAL_RCC_ADC1_CLK_DISABLE();
        // ADC1 GPIO Configuration    
        // PA6     ------> ADC1_IN6 
        // PA7     ------> ADC1_IN7
        // PB1     ------> ADC1_IN9 
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6 | GPIO_PIN_7);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);
    }
} // end of HAL_ADC_MspDeInit


void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hspi->Instance == SPI2) {
        // Peripheral clock enable
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
} // end of HAL_SPI_MspInit


void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    if(hspi->Instance == SPI2) {
        // SPI2 GPIO Configuration    
        // PC3     ------> SPI2_MOSI
        // PB13    ------> SPI2_SCK 
        HAL_GPIO_DeInit(hal_display_spi_pins.MOSI.port, hal_display_spi_pins.MOSI.pin);
        HAL_GPIO_DeInit(hal_display_spi_pins.SCK.port,  hal_display_spi_pins.SCK.pin);
        // Peripheral clock disable
        __HAL_RCC_SPI2_CLK_DISABLE();
    }
} // end of HAL_SPI_MspDeInit


static gMonStatus STM32_HAL_ADC1_Init(void)
{ // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  // in this proejct, multi-channel ADC will be configured, one of the channels is used for analog signal
  // of soil moisture sensor, 2 of the channels are used for analog signals of light-dependent resistors
    HAL_StatusTypeDef  status = HAL_OK;
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_10B;
    hadc1.Init.ScanConvMode = ENABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 3;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    status = HAL_ADC_Init(&hadc1);
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of STM32_HAL_ADC1_Init


static gMonStatus STM32_HAL_SPI2_Init(void)
{ // SPI2 parameter configuration
    HAL_StatusTypeDef  status = HAL_OK;
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
    status = HAL_SPI_Init(&hspi2);
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of STM32_HAL_SPI2_Init



gMonStatus  stationPlatformInit(void)
{
    HAL_StatusTypeDef  status = HAL_OK;
#ifndef GMON_CFG_SKIP_PLATFORM_INIT
    // Reset of all peripherals, Initializes the Flash interface and the Systick. 
    STM32_HAL_Init();
    // Configure the system clock 
    status = SystemClock_Config();
#endif  // end of GMON_CFG_SKIP_PLATFORM_INIT
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    status = STM32_HAL_ADC1_Init();
    if(status != HAL_OK) { goto done; }
    status = STM32_HAL_timer_us_Init(); // initialize 1 us timer
    if(status != HAL_OK) { goto done; }
    status = HAL_TIM_Base_Start(&hal_tim_us);
done:
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of stationPlatformInit


gMonStatus  stationPlatformDeinit(void)
{
    HAL_StatusTypeDef  status = HAL_OK;
    status = HAL_TIM_Base_Stop(&hal_tim_us);
    if(status != HAL_OK) { goto done; }
    status = HAL_ADC_DeInit(&hadc1);
done:
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of stationPlatformDeinit


gMonStatus  staSensorPlatformInitSoilMoist(void)
{
    HAL_StatusTypeDef  status = HAL_OK;
    ADC_ChannelConfTypeDef sConfig = {0};
    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    sConfig.Channel = ADC_CHANNEL_6;
    sConfig.Rank = 1; // ADC_RANK_NONE
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staSensorPlatformInitSoilMoist


gMonStatus  staSensorPlatformDeInitSoilMoist(void)
{
    return  GMON_RESP_OK;
} // end of staSensorPlatformDeInitSoilMoist


gMonStatus  staSensorPlatformInitLight(void)
{
    HAL_StatusTypeDef  status = HAL_OK;
    ADC_ChannelConfTypeDef sConfig = {0};
    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    sConfig.Channel = ADC_CHANNEL_7;
    sConfig.Rank = 2;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    if(status != HAL_OK) { goto done; }
    sConfig.Channel = ADC_CHANNEL_9;
    sConfig.Rank = 3;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    status = HAL_ADC_ConfigChannel(&hadc1, &sConfig);
done:
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staSensorPlatformInitLight


gMonStatus  staSensorPlatformDeInitLight(void)
{
    return  GMON_RESP_OK;
} // end of staSensorPlatformDeInitLight



gMonStatus  staPlatformReadSoilMoistSensor(unsigned int *out)
{ // get status of analog pin by polling the sensor
    if(out == NULL) { return GMON_RESP_ERRARGS; }
    unsigned int timeout = 100; // time ticks
    HAL_StatusTypeDef  status = HAL_OK;
    status = HAL_ADC_Start(&hadc1);
    if(status != HAL_OK) { goto done; }
    status = HAL_ADC_PollForConversion(&hadc1, timeout);
    if(status != HAL_OK) { goto done; }
    *out   = HAL_ADC_GetValue(&hadc1);
done:
    HAL_ADC_Stop(&hadc1);
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staPlatformReadSoilMoistSensor


gMonStatus  staPlatformReadLightSensor(unsigned int *out)
{
    if(out == NULL) { return GMON_RESP_ERRARGS; }
    volatile uint32_t  light_rd_data[2] = {0, 0};
    unsigned int timeout = 100; // time ticks
    HAL_StatusTypeDef  status = HAL_OK;
    status = HAL_ADC_Start(&hadc1);
    if(status != HAL_OK) { goto done; }
    HAL_ADC_PollForConversion(&hadc1, timeout); // workaround: skip first one : analog signal of soil moisture sensor
    status = HAL_ADC_PollForConversion(&hadc1, timeout);
    if(status != HAL_OK) { goto done; }
    light_rd_data[0] = HAL_ADC_GetValue(&hadc1);
    status = HAL_ADC_PollForConversion(&hadc1, timeout);
    if(status != HAL_OK) { goto done; }
    light_rd_data[1] = HAL_ADC_GetValue(&hadc1);
done:
    HAL_ADC_Stop(&hadc1);
    if(status == HAL_OK) {
        *out = (light_rd_data[0] + light_rd_data[1]) / 2;
    }
    return (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staPlatformReadLightSensor


gMonStatus  staSensorPlatformInitAirTemp(void **pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    *(hal_pinout_t **)pinstruct = &hal_air_temp_read_pin;
    return  GMON_RESP_OK;
} // end of staSensorPlatformInitAirTemp

gMonStatus  staSensorPlatformDeInitAirTemp(void)
{
    return  GMON_RESP_OK;
} // end of staSensorPlatformDeInitAirTemp


gMonStatus  staOutDevPlatformInitPump(void **pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    *(hal_pinout_t **)pinstruct = &hal_pump_write_pin;
    return  GMON_RESP_OK;
} // end of staOutDevPlatformInitPump

gMonStatus  staOutDevPlatformInitFan(void **pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    *(hal_pinout_t **)pinstruct = &hal_fan_write_pin;
    return  GMON_RESP_OK;
} // end of staOutDevPlatformInitFan

gMonStatus  staOutDevPlatformInitBulb(void **pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    *(hal_pinout_t **)pinstruct = &hal_bulb_write_pin;
    return  GMON_RESP_OK;
} // end of staOutDevPlatformInitBulb


gMonStatus  staOutDevPlatformInitDisplay(uint8_t  comm_protocal_id, void **pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    gMonStatus  status = GMON_RESP_OK;

    switch(comm_protocal_id) {
        case GMON_PLATFORM_DISPLAY_SPI:
            hal_display_spi_pins.handler = &hspi2;
            hal_display_spi_pins.SCK.port  = GPIOB;
            hal_display_spi_pins.SCK.pin   = GPIO_PIN_13;
            hal_display_spi_pins.SCK.alternate = GPIO_AF5_SPI2;
            hal_display_spi_pins.MOSI.port = GPIOC;
            hal_display_spi_pins.MOSI.pin  = GPIO_PIN_3;
            hal_display_spi_pins.MOSI.alternate = GPIO_AF5_SPI2;
            hal_display_spi_pins.MISO.port = NULL;
            hal_display_spi_pins.MISO.pin  = 0;
            hal_display_spi_pins.MISO.alternate  = 0;
            status = STM32_HAL_SPI2_Init();
            if(status != GMON_RESP_OK) { break; }
            *(hal_spi_pinout_t **)pinstruct = &hal_display_spi_pins;
            break;
        case GMON_PLATFORM_DISPLAY_I2C: // TODO
        default:
            status = GMON_RESP_ERR_NOT_SUPPORT;
            break;
    } // end of switch case
    return  status;
} // end of staOutDevPlatformInitDisplay


void*  staPlatformiGetDisplayRstPin(void)
{
    return (void *)&hal_display_rst_pin;
} // end of staPlatformiGetDisplayRstPin


void*  staPlatformiGetDisplayDataCmdPin(void)
{
    return (void *)&hal_display_dc_pin;
} // end of staPlatformiGetDisplayDataCmdPin


gMonStatus  staOutDevPlatformDeinitDisplay(void *pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    HAL_StatusTypeDef  status = HAL_OK;
    if(pinstruct == &hal_display_spi_pins) {
        status = HAL_SPI_DeInit(hal_display_spi_pins.handler);
    }
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staOutDevPlatformDeinitDisplay


gMonStatus  staPlatformSPItransmit(void *pinstruct, unsigned char *pData, unsigned short sz)
{
    if(pinstruct == NULL || pData == NULL || sz == 0) {
        return GMON_RESP_ERRARGS;
    }
    hal_spi_pinout_t  *spi = NULL;
    HAL_StatusTypeDef  status = HAL_OK;

    spi = (hal_spi_pinout_t*) pinstruct;
    status = HAL_SPI_Transmit(spi->handler, pData, sz, HAL_MAX_DELAY);
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staPlatformSPItransmit


gMonStatus  staPlatformDelayUs(uint16_t us)
{
    HAL_StatusTypeDef  status = HAL_OK;
    __HAL_TIM_SET_COUNTER(&hal_tim_us, 0);
    while(__HAL_TIM_GET_COUNTER(&hal_tim_us) < us);
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staPlatformDelayUs


gMonStatus  staPlatformPinSetDirection(void *pinstruct, uint8_t direction)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    hal_pinout_t      *hal_pinstruct = NULL;
    GPIO_InitTypeDef   GPIO_InitStruct = {0};
    HAL_StatusTypeDef  status  = HAL_OK;

    hal_pinstruct = (hal_pinout_t *)pinstruct;
    HAL_GPIO_DeInit(hal_pinstruct->port, hal_pinstruct->pin);
    switch(direction) {
        case GMON_PLATFORM_PIN_DIRECTION_OUT:
            // Configure GPIO pin Output Level
            HAL_GPIO_WritePin(hal_pinstruct->port, hal_pinstruct->pin, GPIO_PIN_RESET);
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            break;
        case GMON_PLATFORM_PIN_DIRECTION_IN :
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLDOWN; //GPIO_NOPULL;
            break;
        default:
            status = HAL_ERROR;
            break;
    } // end of switch case
    if(status == HAL_OK) {
        GPIO_InitStruct.Pin = hal_pinstruct->pin;
        HAL_GPIO_Init(hal_pinstruct->port, &GPIO_InitStruct);
    }
    return  (status == HAL_OK ? GMON_RESP_OK: GMON_RESP_ERR);
} // end of staPlatformPinSetDirection


gMonStatus  staPlatformWritePin(void *pinstruct, uint8_t new_state)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    hal_pinout_t  *hal_pinstruct = NULL;
    hal_pinstruct = (hal_pinout_t *)pinstruct;
    HAL_GPIO_WritePin(hal_pinstruct->port, hal_pinstruct->pin, new_state);
    return  GMON_RESP_OK;
} // end of staPlatformWritePin


uint8_t     staPlatformReadPin(void *pinstruct)
{
    if(pinstruct == NULL) { return GMON_RESP_ERRARGS; }
    hal_pinout_t *hal_pinstruct = NULL;
    hal_pinstruct = (hal_pinout_t *)pinstruct;
    return  (uint8_t) HAL_GPIO_ReadPin(hal_pinstruct->port, hal_pinstruct->pin);
} // end of staPlatformReadPin


