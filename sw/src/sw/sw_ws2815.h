/**
 * @file sw_ws2815.h
 * @brief WS2815 addressable LED driver for STM32G030K8T6
 * @author Richard Roztocil
 *
 * Target MCU: STM32G030K8T6
 * - Cortex-M0+ core
 * - 64KB Flash, 8KB RAM
 * - LQFP32 package
 */

#ifndef SW_WS2815_H
#define SW_WS2815_H

#include <stdint.h>
#include "stm32g0xx_hal.h"

/* External variables (needed by IRQ handlers) */
extern TIM_HandleTypeDef htim_ws2815;
extern DMA_HandleTypeDef hdma_ws2815;

/* WS2815 Timing specifications (from datasheet)
 * T0H: 220-380ns   (0-code, High-level)
 * T0L: 580-1600ns  (0-code, Low-level)
 * T1H: 580-1600ns  (1-code, High-level)
 * T1L: 220-420ns   (1-code, Low-level)
 * RES: >280us      (Reset)
 */

/**
 * @brief RGB color structure for WS2815 LED
 */
typedef struct {
    uint8_t r;  // Red component (0-255)
    uint8_t g;  // Green component (0-255)
    uint8_t b;  // Blue component (0-255)
} WS2815_Color_t;

/**
 * @brief Initialize WS2815 LED driver
 * @return 0 on success, negative on error
 */
int WS2815_Init(void);

/**
 * @brief Set color of a single LED
 * @param led_index Index of LED in the chain
 * @param color RGB color structure
 * @return 0 on success, negative on error
 */
int WS2815_SetLED(uint16_t led_index, WS2815_Color_t color);

/**
 * @brief Update all LEDs with buffered data
 * @return 0 on success, negative on error
 */
int WS2815_Update(void);

/**
 * @brief Clear all LEDs (turn off)
 * @return 0 on success, negative on error
 */
int WS2815_Clear(void);

/**
 * @brief Set brightness for all LEDs (0-1, 2-255)
 * @param brightness Brightness level (0-1 = off, 2-255 = on)
 */
void WS2815_SetBrightness(uint8_t brightness);

/**
 * @brief Set color of a single 7-segment display segment
 * @param segment_index Segment index (0-7):
 *                      0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g, 7=decimal point
 * @param color RGB color structure
 * @return 0 on success, negative on error
 */
int WS2815_SetSegment(uint8_t segment_index, WS2815_Color_t color);

/**
 * @brief Display a digit (0-9) on the 7-segment display
 * @param digit Digit to display (0-9)
 * @param color RGB color structure for the digit
 * @param show_dp Show decimal point (1 = show, 0 = hide)
 * @return 0 on success, negative on error
 */
int WS2815_DisplayDigit(uint8_t digit, WS2815_Color_t color, uint8_t show_dp);

/**
 * @brief Check if WS2815 driver is busy transmitting data
 * @return 1 if busy, 0 if ready
 */
uint8_t WS2815_IsBusy(void);

#endif // SW_WS2815_H
