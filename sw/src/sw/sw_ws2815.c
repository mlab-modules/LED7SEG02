/**
 * @file sw_ws2815.c
 * @brief WS2815 addressable LED driver implementation for STM32G030K8T6
 * @author Richard Roztocil
 *
 * Target MCU: STM32G030K8T6
 * - Cortex-M0+ @ 64MHz max
 * - Uses TIM + DMA for precise WS2815 timing
 * - Recommended: TIM1/TIM3 with DMA for GPIO bit-banging or PWM
 */

#include "sw_ws2815.h"

#include "stm32g0xx_hal.h"

/* Configuration */
#define WS2815_LED_COUNT    30  // Adjust based on 7-segment display layout
#define WS2815_BUFFER_SIZE  (WS2815_LED_COUNT * 3)

/* Hardware Configuration */
#define WS2815_TIM          TIM3
#define WS2815_TIM_CHANNEL  TIM_CHANNEL_2
#define WS2815_GPIO_PORT    GPIOA
#define WS2815_GPIO_PIN     GPIO_PIN_7  // TIM3_CH2
#define WS2815_DMA_CHANNEL  DMA1_Channel2  // Changed from Channel1 (used by SPI) to Channel2

/* Timing Configuration (assumes 64MHz system clock)
 * WS2815 bit period: ~1.25us (800kHz)
 * Timer period: 80 cycles @ 64MHz = 1.25us
 *
 * Datasheet WS2815 timing specifications:
 * T0H: 220-380ns  -> 24 cycles = 375ns
 * T0L: 580-1600ns -> 56 cycles = 875ns
 * T1H: 580-1600ns -> 52 cycles = 812.5ns
 * T1L: 220-420ns  -> 28 cycles = 437.5ns (slightly over, but works)
 */
#define WS2815_TIM_PERIOD   80
#define WS2815_BIT_0_DUTY   24
#define WS2815_BIT_1_DUTY   52

/* Private variables */
static WS2815_Color_t led_buffer[WS2815_LED_COUNT];
static uint8_t brightness = 255;

/* Public variables (needed by IRQ handlers) */
TIM_HandleTypeDef htim_ws2815;
DMA_HandleTypeDef hdma_ws2815;

// DMA buffer for PWM values (24 bits per LED + reset period)
// Reset needs >280us LOW = ~224 slots at 800kHz (1.25us per slot)
// Using 250 slots = 312.5us for safety margin
#define WS2815_RESET_SLOTS  250
static uint16_t ws2815_dma_buffer[WS2815_LED_COUNT * 24 + WS2815_RESET_SLOTS];
static volatile uint8_t ws2815_busy = 0;

/* Private function prototypes */
static void WS2815_GPIO_Init(void);
static void WS2815_TIM_Init(void);
static void WS2815_DMA_Init(void);
static void WS2815_EncodeLEDData(void);

/**
 * @brief Initialize GPIO for WS2815 data line
 */
static void WS2815_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clock
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin as TIM3_CH2 alternate function
    GPIO_InitStruct.Pin = WS2815_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
    HAL_GPIO_Init(WS2815_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief Initialize Timer for WS2815 PWM generation
 */
static void WS2815_TIM_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    // Enable TIM3 clock
    __HAL_RCC_TIM3_CLK_ENABLE();

    // Configure timer base
    htim_ws2815.Instance = WS2815_TIM;
    htim_ws2815.Init.Prescaler = 0;  // No prescaler, run at full speed (64MHz)
    htim_ws2815.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_ws2815.Init.Period = WS2815_TIM_PERIOD - 1;
    htim_ws2815.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_ws2815.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim_ws2815);

    // Configure PWM channel
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim_ws2815, &sConfigOC, WS2815_TIM_CHANNEL);

    // Enable DMA request for timer
    __HAL_TIM_ENABLE_DMA(&htim_ws2815, TIM_DMA_CC2);
}

/**
 * @brief Initialize DMA for WS2815 data transfer
 */
static void WS2815_DMA_Init(void)
{
    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    // Configure DMA
    hdma_ws2815.Instance = WS2815_DMA_CHANNEL;
    hdma_ws2815.Init.Request = DMA_REQUEST_TIM3_CH2;
    hdma_ws2815.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_ws2815.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_ws2815.Init.MemInc = DMA_MINC_ENABLE;
    hdma_ws2815.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_ws2815.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_ws2815.Init.Mode = DMA_NORMAL;
    hdma_ws2815.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_ws2815);

    // Link DMA to timer
    __HAL_LINKDMA(&htim_ws2815, hdma[TIM_DMA_ID_CC2], hdma_ws2815);

    // Enable DMA interrupt
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

/**
 * @brief Initialize WS2815 LED driver
 */
int WS2815_Init(void)
{
    // Initialize hardware peripherals
    WS2815_GPIO_Init();
    WS2815_TIM_Init();
    WS2815_DMA_Init();

    // Clear LED buffer
    for (uint16_t i = 0; i < WS2815_LED_COUNT; i++) {
        led_buffer[i].r = 0;
        led_buffer[i].g = 0;
        led_buffer[i].b = 0;
    }

    // Clear DMA buffer (including reset slots)
    for (uint16_t i = 0; i < WS2815_LED_COUNT * 24 + WS2815_RESET_SLOTS; i++) {
        ws2815_dma_buffer[i] = 0;
    }

    ws2815_busy = 0;

    return 0;
}

/**
 * @brief Encode LED buffer data into DMA PWM buffer
 */
static void WS2815_EncodeLEDData(void)
{
    uint16_t dma_idx = 0;

    // For each LED
    for (uint16_t led = 0; led < WS2815_LED_COUNT; led++) {
        // WS2815 expects GRB order
        uint8_t color_bytes[3] = {
            led_buffer[led].g,
            led_buffer[led].r,
            led_buffer[led].b
        };

        // Encode each color byte (8 bits)
        for (uint8_t byte_idx = 0; byte_idx < 3; byte_idx++) {
            uint8_t byte = color_bytes[byte_idx];

            // Encode each bit (MSB first)
            for (int8_t bit = 7; bit >= 0; bit--) {
                if (byte & (1 << bit)) {
                    ws2815_dma_buffer[dma_idx] = WS2815_BIT_1_DUTY;  // '1' bit
                } else {
                    ws2815_dma_buffer[dma_idx] = WS2815_BIT_0_DUTY;  // '0' bit
                }
                dma_idx++;
            }
        }
    }
}

/**
 * @brief Set color of a single LED
 */
int WS2815_SetLED(uint16_t led_index, WS2815_Color_t color)
{
    if (led_index >= WS2815_LED_COUNT) {
        return -1;  // Invalid index
    }

    // Apply brightness scaling
    // Using bit shift for faster calculation: (value * brightness) >> 8
    // This is equivalent to dividing by 256
    if (brightness >= 254) {
        // Full brightness - no scaling needed
        led_buffer[led_index].r = color.r;
        led_buffer[led_index].g = color.g;
        led_buffer[led_index].b = color.b;
    } else {
        led_buffer[led_index].r = ((uint16_t)color.r * brightness) >> 8;
        led_buffer[led_index].g = ((uint16_t)color.g * brightness) >> 8;
        led_buffer[led_index].b = ((uint16_t)color.b * brightness) >> 8;
    }

    return 0;
}

/**
 * @brief Update all LEDs with buffered data
 */
int WS2815_Update(void)
{
    // Check if previous transfer is still in progress
    if (ws2815_busy) {
        return -1;  // Busy
    }

    // Encode LED data into DMA buffer
    WS2815_EncodeLEDData();

    // Mark as busy
    ws2815_busy = 1;

    // Start PWM generation with DMA (including reset slots at the end)
    HAL_TIM_PWM_Start_DMA(&htim_ws2815, WS2815_TIM_CHANNEL,
                          (uint32_t*)ws2815_dma_buffer,
                          WS2815_LED_COUNT * 24 + WS2815_RESET_SLOTS);

    // Note: After DMA completes, signal stays low which serves as RES (reset)
    // DMA complete interrupt will clear ws2815_busy flag

    return 0;
}

/**
 * @brief DMA transfer complete callback
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == WS2815_TIM) {
        // Stop PWM
        HAL_TIM_PWM_Stop_DMA(htim, WS2815_TIM_CHANNEL);

        // Clear busy flag
        ws2815_busy = 0;
    }
}

/**
 * @brief Clear all LEDs
 */
int WS2815_Clear(void)
{
    WS2815_Color_t black = {0, 0, 0};

    for (uint16_t i = 0; i < WS2815_LED_COUNT; i++) {
        WS2815_SetLED(i, black);
    }

    return WS2815_Update();
}

/**
 * @brief Set brightness for all LEDs
 */
void WS2815_SetBrightness(uint8_t new_brightness)
{
    brightness = new_brightness;
}

/**
 * @brief Set color of a single 7-segment display segment
 * @param segment_index Segment index (0-7):
 *                      0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g, 7=decimal point
 * @param color RGB color structure
 * @return 0 on success, negative on error
 */
int WS2815_SetSegment(uint8_t segment_index, WS2815_Color_t color)
{
    if (segment_index > 7) {
        return -1;  // Invalid segment index
    }

    // Segment mapping (each segment has 4 LEDs, except decimal point which has 1)
    // Segment a (0): LED 0-3
    // Segment b (1): LED 4-7
    // Segment c (2): LED 8-11
    // Segment d (3): LED 12-15
    // Segment e (4): LED 16-19
    // Segment f (5): LED 24-27  (swapped with g)
    // Segment g (6): LED 20-23  (swapped with f)
    // Decimal point (7): LED 28

    if (segment_index == 7) {
        // Decimal point - only one LED
        WS2815_SetLED(28, color);
    } else if (segment_index == 5) {
        // Segment f: LED 24-27
        for (uint8_t i = 0; i < 4; i++) {
            WS2815_SetLED(24 + i, color);
        }
    } else if (segment_index == 6) {
        // Segment g: LED 20-23
        for (uint8_t i = 0; i < 4; i++) {
            WS2815_SetLED(20 + i, color);
        }
    } else {
        // Regular segments a,b,c,d,e - 4 LEDs each
        uint16_t start_led = segment_index * 4;
        for (uint8_t i = 0; i < 4; i++) {
            WS2815_SetLED(start_led + i, color);
        }
    }

    return 0;
}

/**
 * @brief Display a digit (0-9) on the 7-segment display
 * @param digit Digit to display (0-9)
 * @param color RGB color structure for the digit
 * @param show_dp Show decimal point (1 = show, 0 = hide)
 * @return 0 on success, negative on error
 */
int WS2815_DisplayDigit(uint8_t digit, WS2815_Color_t color, uint8_t show_dp)
{
    if (digit > 9) {
        return -1;  // Invalid digit
    }

    // 7-segment encoding for digits 0-9
    // Bit mapping: bit 0=a, bit 1=b, bit 2=c, bit 3=d, bit 4=e, bit 5=f, bit 6=g
    //     aaa
    //    f   b
    //     ggg
    //    e   c
    //     ddd  (DP)
    const uint8_t digit_segments[10] = {
        0b00111111,  // 0: a,b,c,d,e,f
        0b00000110,  // 1: b,c
        0b01011011,  // 2: a,b,d,e,g
        0b01001111,  // 3: a,b,c,d,g
        0b01100110,  // 4: b,c,f,g
        0b01101101,  // 5: a,c,d,f,g
        0b01111101,  // 6: a,c,d,e,f,g
        0b00000111,  // 7: a,b,c
        0b01111111,  // 8: a,b,c,d,e,f,g (all)
        0b01101111   // 9: a,b,c,d,f,g
    };

    uint8_t segments = digit_segments[digit];
    WS2815_Color_t black = {0, 0, 0};

    // Set each segment based on the bit pattern
    for (uint8_t i = 0; i < 7; i++) {
        if (segments & (1 << i)) {
            WS2815_SetSegment(i, color);
        } else {
            WS2815_SetSegment(i, black);
        }
    }

    // Handle decimal point
    if (show_dp) {
        WS2815_SetSegment(7, color);
    } else {
        WS2815_SetSegment(7, black);
    }

    return 0;
}

/**
 * @brief Check if WS2815 driver is busy
 */
uint8_t WS2815_IsBusy(void)
{
    return ws2815_busy;
}
