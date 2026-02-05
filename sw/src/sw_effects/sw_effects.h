/**
 * @file sw_effects.h
 * @brief Visual effects for 7-segment WS2815 LED display
 * @author LED7SEG02 Project
 */

#ifndef SW_EFFECTS_H
#define SW_EFFECTS_H

#include <stdint.h>
#include "sw_ws2815.h"

/**
 * @brief Initialize effects module
 */
void Effects_Init(void);

/**
 * @brief Rainbow effect - cycles through all colors on the display
 * @param step Current animation step (0-255, increment this each frame)
 * @return 0 on success, negative on error
 *
 * Usage: Call repeatedly with incrementing step value to animate
 * Example:
 *   for (step = 0; step < 255; step++) {
 *     Effects_Rainbow(step);
 *     WS2815_Update();
 *     while(WS2815_IsBusy());
 *     HAL_Delay(20);
 *   }
 */
int Effects_Rainbow(uint8_t step);

/**
 * @brief Spinning single LED effect for stand-by indication
 * @param position Current LED position (0-23, moves around perimeter clockwise)
 * @param color RGB color for the spinning LED
 * @return 0 on success, negative on error
 *
 * Note: Spins only around the perimeter (segments a,b,c,d,e,f forming digit 0)
 *
 * Usage: Call repeatedly with incrementing position to animate
 * Example:
 *   WS2815_Color_t blue = {0, 0, 255};
 *   for (pos = 0; pos < 24; pos++) {
 *     Effects_SpinningLED(pos, blue);
 *     WS2815_Update();
 *     while(WS2815_IsBusy());
 *     HAL_Delay(50);
 *   }
 */
int Effects_SpinningLED(uint8_t position, WS2815_Color_t color);

/**
 * @brief Breathing effect - fades in and out
 * @param step Current animation step (0-255, increment this each frame)
 * @param color Base RGB color for breathing effect
 * @return 0 on success, negative on error
 *
 * Usage: Call repeatedly with incrementing step value
 * The brightness will automatically cycle from dim to bright and back
 */
int Effects_Breathing(uint8_t step, WS2815_Color_t color);

/**
 * @brief Segment chase effect - lights up segments in sequence
 * @param step Current animation step (0-6 for 7 segments)
 * @param color RGB color for the lit segment
 * @return 0 on success, negative on error
 */
int Effects_SegmentChase(uint8_t step, WS2815_Color_t color);

/**
 * @brief Loading bar effect - fills segments progressively
 * @param progress Progress percentage (0-100)
 * @param color RGB color for lit segments
 * @return 0 on success, negative on error
 *
 * Usage: Display a loading progress bar
 * Example:
 *   WS2815_Color_t green = {0, 255, 0};
 *   for (uint8_t i = 0; i <= 100; i++) {
 *     Effects_LoadingBar(i, green);
 *     WS2815_Update();
 *     while(WS2815_IsBusy());
 *     HAL_Delay(50);
 *   }
 */
int Effects_LoadingBar(uint8_t progress, WS2815_Color_t color);

/**
 * @brief Knight Rider effect - LED bounces back and forth with trail
 * @param position Current position (0-56, bounces at 0 and 56)
 * @param color RGB color for the LED
 * @return 0 on success, negative on error
 *
 * Usage: Call repeatedly with incrementing position
 * Position 0-28: forward motion, 29-56: backward motion
 * Example:
 *   WS2815_Color_t red = {255, 0, 0};
 *   for (uint8_t pos = 0; pos < 57; pos++) {
 *     Effects_KnightRider(pos, red);
 *     WS2815_Update();
 *     while(WS2815_IsBusy());
 *     HAL_Delay(20);
 *   }
 */
int Effects_KnightRider(uint8_t position, WS2815_Color_t color);

/**
 * @brief Display digit with color cycling through spectrum (non-blocking)
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0 = hide, 1 = show)
 * @param step Current color step (0-255, cycles through HSV spectrum)
 * @return 0 on success, negative on error
 *
 * This is a non-blocking function - call it repeatedly with incrementing step
 * to create a rainbow color cycling effect on the displayed digit.
 *
 * Usage: Call in loop with incrementing step for continuous color change
 * Example:
 *   uint8_t step = 0;
 *   while(1) {
 *     Effects_RainbowDigit(8, 1, step);  // Show "8." with rainbow colors
 *     WS2815_Update();
 *     while(WS2815_IsBusy());
 *     HAL_Delay(20);
 *     step++;
 *   }
 */
int Effects_RainbowDigit(uint8_t digit, uint8_t show_dp, uint8_t step);

/**
 * @brief Display digit with rainbow gradient across active LEDs
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0 = hide, 1 = show)
 * @param step Current animation step (0-255, shifts the gradient)
 * @return 0 on success, negative on error
 *
 * Unlike Effects_RainbowDigit which uses one color for all segments,
 * this effect creates a rainbow gradient across all active LEDs of the digit.
 * Each LED has a different hue, creating a colorful gradient effect.
 *
 * Usage: Call in loop with incrementing step for animated gradient
 * Example:
 *   uint8_t step = 0;
 *   while(1) {
 *     Effects_RainbowGradientDigit(8, 1, step);  // Show "8." with rainbow gradient
 *     WS2815_Update();
 *     while(WS2815_IsBusy());
 *     HAL_Delay(20);
 *     step++;
 *   }
 */
int Effects_RainbowGradientDigit(uint8_t digit, uint8_t show_dp, uint8_t step);

/**
 * @brief Convert HSV color to RGB
 * @param h Hue (0-255)
 * @param s Saturation (0-255)
 * @param v Value/Brightness (0-255)
 * @return WS2815_Color_t RGB color structure
 */
WS2815_Color_t Effects_HSV_to_RGB(uint8_t h, uint8_t s, uint8_t v);


/**
 * @brief Continuous rainbow effect on a single digit (blocking)
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0 = hide, 1 = show)
 *
 * Displays the specified digit with continuously cycling rainbow colors.
 * This is a blocking function that runs an infinite loop.
 */
void Demo_RainbowDigit_Continuous(uint8_t digit, uint8_t show_dp);

/**
 * @brief Demo of rainbow gradient digit effect (0-9 sequence)
 *
 * Cycles through digits 0-9, displaying each with a rainbow gradient
 * where each LED has a different hue. This is a blocking demo function.
 */
void Demo_RainbowGradientDigit(void);

/**
 * @brief Continuous rainbow gradient effect on a single digit (blocking)
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0 = hide, 1 = show)
 *
 * Displays the specified digit with an animated rainbow gradient
 * where each active LED has a different hue from the spectrum.
 * This is a blocking function that runs an infinite loop.
 */
void Demo_RainbowGradientDigit_Continuous(uint8_t digit, uint8_t show_dp);

#endif // SW_EFFECTS_H
