#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "pin_map.h"

extern void STM32_generic_USART_IRQHandler(UART_HandleTypeDef *);
extern void STM32_generic_DMAstream_IRQHandler(DMA_HandleTypeDef *);

// STM32F446 board doesn't have network hardware module, in order to run this MQTT implementation,
// it's essential to connect this board to external network device (e.g. ESP8266 wifi module)
static UART_HandleTypeDef haluart3;
// the DMA module used with UART3 (STM32F4xx board)
static DMA_HandleTypeDef haldma_usart3_rx;
// timer for estimating entropy in cryptography algorithm in the TLS library
static TIM_HandleTypeDef htim2;

HAL_StatusTypeDef STM32_HAL_GeneralTimer_Init(uint32_t TickPriority) {
    RCC_ClkInitTypeDef clkconfig = {0};

    uint32_t uwTimclock = 0, uwPrescalerValue = 0, pFLatency = 0;

    __HAL_RCC_TIM2_CLK_ENABLE();
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    uwTimclock = HAL_RCC_GetPCLK1Freq();
    /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000) - 1);
    htim2.Instance = TIM2;
    /* Initialize TIMx peripheral as follow:
    + Period = [(TIM2CLK/1000) - 1]. to have a (1/1000) s time base.
    + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
    + ClockDivision = 0
    + Counter direction = Up
    */
    htim2.Init.Period = (1000000 / 1000) - 1;
    htim2.Init.Prescaler = uwPrescalerValue;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_Base_Init(&htim2) == HAL_OK) {
        HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority, 0);
        /* Start the TIM time Base generation in interrupt mode */
        return HAL_TIM_Base_Start_IT(&htim2);
    }
    return HAL_ERROR;
} // end of STM32_HAL_GeneralTimer_Init

HAL_StatusTypeDef STM32_HAL_UART_Init(void) {
    haluart3.Instance = USART3;
    haluart3.Init.BaudRate = 115200;
    haluart3.Init.WordLength = UART_WORDLENGTH_8B;
    haluart3.Init.StopBits = UART_STOPBITS_1;
    haluart3.Init.Parity = UART_PARITY_NONE;
    haluart3.Init.Mode = UART_MODE_TX_RX;
    haluart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    haluart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_StatusTypeDef result = HAL_UART_Init(&haluart3);
    if (result == HAL_OK) {
        // manually enable IDLE line detection interrupt
        __HAL_UART_ENABLE_IT(&haluart3, UART_IT_IDLE);
        HAL_NVIC_SetPriority(USART3_IRQn, (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
    return result;
}

HAL_StatusTypeDef STM32_HAL_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();
    //  ---------- initialize DMA for Rx of ESP device. ----------
    haldma_usart3_rx.Instance = DMA1_Stream1;
    haldma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
    haldma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    haldma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    haldma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
    haldma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    haldma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    haldma_usart3_rx.Init.Mode = DMA_CIRCULAR;
    haldma_usart3_rx.Init.Priority = DMA_PRIORITY_LOW;
    haldma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_StatusTypeDef result = HAL_DMA_Init(&haldma_usart3_rx);
    if (result == HAL_OK) {
        HAL_NVIC_SetPriority(
            DMA1_Stream1_IRQn, (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0
        );
        HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
        __HAL_LINKDMA(&haluart3, hdmarx, haldma_usart3_rx);
    }
    return result;
}

HAL_StatusTypeDef STM32_HAL_UART_DeInit(void) {
    haluart3.Instance = USART3;
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    __HAL_UART_DISABLE_IT(&haluart3, UART_IT_IDLE);
    HAL_UART_DeInit(&haluart3);
    return HAL_OK;
}

UART_HandleTypeDef *STM32_config_UART(void) { return &haluart3; }
DMA_HandleTypeDef  *STM32_config_DMA4UART(void) { return &haldma_usart3_rx; }
TIM_HandleTypeDef  *STM32_config_GeneralTimer(void) { return &htim2; }

// brief This function handles Non maskable interrupt.
void NMI_Handler(void) {}

// brief This function handles Debug monitor.
void DebugMon_Handler(void) {}

// brief This function handles Pre-fetch fault, memory access fault.
void BusFault_Handler(void) { configASSERT(0); }

// brief This function handles Undefined instruction or illegal state.
void UsageFault_Handler(void) { configASSERT(0); }

void vTestAppHardFaultHandler(UBaseType_t *sp) {
    BaseType_t done = vPortTryRecoverHardFault(sp);
    configASSERT(done);
}

void SysTick_Handler(void) { vPortSysTickHandler(); }

void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }

void USART3_IRQHandler(void) { STM32_generic_USART_IRQHandler(&haluart3); }

void DMA1_Stream1_IRQHandler(void) { STM32_generic_DMAstream_IRQHandler(&haldma_usart3_rx); }

// will be called by HAL_UART_Init()
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        // USART3 GPIO Configuration
        // PC5     ------> USART3_RX
        // PB10    ------> USART3_TX
        GPIO_InitStruct.Pin = HW_ESP8266_UART_RX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = HW_ESP8266_UART_RX_AF;
        HAL_GPIO_Init(HW_ESP8266_UART_RX_PORT, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = HW_ESP8266_UART_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = HW_ESP8266_UART_TX_AF;
        HAL_GPIO_Init(HW_ESP8266_UART_TX_PORT, &GPIO_InitStruct);
    }
} // end of HAL_UART_MspInit

// will be called by HAL_UART_DeInit()
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_DISABLE();
        // USART3 GPIO Configuration
        // PC5     ------> USART3_RX
        // PB10    ------> USART3_TX
        HAL_GPIO_DeInit(HW_ESP8266_UART_TX_PORT, HW_ESP8266_UART_TX_PIN);
        HAL_GPIO_DeInit(HW_ESP8266_UART_RX_PORT, HW_ESP8266_UART_RX_PIN);
    }
} // end of HAL_UART_MspDeInit

HAL_StatusTypeDef STM32_HAL_GPIO_Init(void) {
    //  ---------- initialize GPIO pins  for ESP device ----------
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // `PH0` on STM32 board is always pulled HIGH and connected to `CH_PD` and `GPIO0`
    // pin of ESP device, for now they are hardcoded here they don't need to be
    // shared in core library implementation
    __HAL_RCC_GPIOH_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    // configure network device RST pin
    GPIO_InitStruct.Pin = HW_ESP8266_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HW_ESP8266_RST_PORT, &GPIO_InitStruct);
    // Configure GPIO pin for sonar sensor
    GPIO_InitStruct.Pin = HW_ENTROPY_TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HW_ENTROPY_TRIG_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = HW_ENTROPY_ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(HW_ENTROPY_ECHO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HW_ESP8266_RST_PORT, HW_ESP8266_RST_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(HW_ENTROPY_TRIG_PORT, HW_ENTROPY_TRIG_PIN, GPIO_PIN_RESET);
    return HAL_OK;
}
