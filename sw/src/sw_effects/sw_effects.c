/**
 * @file sw_effects.c
 * @brief Visual effects implementation for 7-segment WS2815 LED display
 * @author LED7SEG02 Project
 */

#include "sw_effects.h"
#include "sw_ws2815.h"
#include <math.h>

/**
 * @brief Initialize effects module
 */
void Effects_Init(void)
{
    // Nothing to initialize yet
    // Future: Could initialize timers for automatic animation
}

/**
 * @brief Convert HSV color to RGB
 * @param h Hue (0-255)
 * @param s Saturation (0-255)
 * @param v Value/Brightness (0-255)
 * @return WS2815_Color_t RGB color structure
 */
WS2815_Color_t Effects_HSV_to_RGB(uint8_t h, uint8_t s, uint8_t v)
{
    WS2815_Color_t rgb;
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        // Achromatic (grey)
        rgb.r = v;
        rgb.g = v;
        rgb.b = v;
        return rgb;
    }

    region = h / 43;  // 0-5
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.r = v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = v;
            break;
        default:  // case 5:
            rgb.r = v; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
}

/**
 * @brief Rainbow effect - cycles through all colors on the display
 */
int Effects_Rainbow(uint8_t step)
{
    // Create rainbow effect where each LED has a different hue
    // All LEDs cycle through the rainbow together

    for (uint8_t i = 0; i < 29; i++) {
        // Offset each LED's hue slightly for a gradient effect
        uint8_t hue = step + (i * 8);  // 8 = 255/29 for even distribution
        WS2815_Color_t color = Effects_HSV_to_RGB(hue, 255, 255);
        WS2815_SetLED(i, color);
    }

    return 0;
}

/**
 * @brief Spinning single LED effect for stand-by indication
 * Spins around the perimeter (segments forming digit 0: a,b,c,d,e,f)
 */
int Effects_SpinningLED(uint8_t position, WS2815_Color_t color)
{
    // LED mapping for perimeter (forming digit 0)
    // Segments: a -> b -> c -> d -> e -> f -> back to a
    const uint8_t perimeter_leds[24] = {
        // Segment a (top)
        0, 1, 2, 3,
        // Segment b (top right)
        4, 5, 6, 7,
        // Segment c (bottom right)
        8, 9, 10, 11,
        // Segment d (bottom)
        12, 13, 14, 15,
        // Segment e (bottom left)
        16, 17, 18, 19,
        // Segment f (top left) - reversed order for smooth motion
        27, 26, 25, 24
    };

    // Turn off all LEDs
    WS2815_Color_t black = {0, 0, 0};
    for (uint8_t i = 0; i < 29; i++) {
        WS2815_SetLED(i, black);
    }

    // Light up only the LED at the specified position on perimeter
    position = position % 24;  // Wrap around the perimeter (24 LEDs)
    WS2815_SetLED(perimeter_leds[position], color);

    return 0;
}

/**
 * @brief Breathing effect - fades in and out
 */
int Effects_Breathing(uint8_t step, WS2815_Color_t color)
{
    // Calculate brightness using sine wave for smooth breathing
    // step 0-255 maps to 0-360 degrees (or 0-2*PI radians)

    // Use triangle wave instead of sine for simplicity
    uint8_t brightness;
    if (step < 128) {
        // Fade in: 0 to 255
        brightness = step * 2;
    } else {
        // Fade out: 255 to 0
        brightness = (255 - step) * 2;
    }

    // Apply brightness to color
    WS2815_Color_t dimmed_color;
    dimmed_color.r = (color.r * brightness) / 255;
    dimmed_color.g = (color.g * brightness) / 255;
    dimmed_color.b = (color.b * brightness) / 255;

    // Set all LEDs to the dimmed color
    for (uint8_t i = 0; i < 29; i++) {
        WS2815_SetLED(i, dimmed_color);
    }

    return 0;
}

/**
 * @brief Segment chase effect - lights up segments in sequence
 */
int Effects_SegmentChase(uint8_t step, WS2815_Color_t color)
{
    WS2815_Color_t black = {0, 0, 0};

    // Turn off all segments
    for (uint8_t i = 0; i < 7; i++) {
        WS2815_SetSegment(i, black);
    }

    // Light up current segment
    uint8_t segment = step % 7;  // Wrap around after segment g
    WS2815_SetSegment(segment, color);

    return 0;
}

/**
 * @brief Loading bar effect - fills segments progressively
 */
int Effects_LoadingBar(uint8_t progress, WS2815_Color_t color)
{
    // progress: 0-100 percentage
    // Maps to segments: 0% = none, 100% = all 7 lit

    WS2815_Color_t black = {0, 0, 0};
    uint8_t segments_lit = (progress * 7) / 100;

    // Segment order for loading: a, b, c, d, e, f, g (clockwise)
    const uint8_t segment_order[7] = {0, 1, 2, 3, 4, 5, 6};

    for (uint8_t i = 0; i < 7; i++) {
        if (i < segments_lit) {
            WS2815_SetSegment(segment_order[i], color);
        } else {
            WS2815_SetSegment(segment_order[i], black);
        }
    }

    return 0;
}

/**
 * @brief Knight Rider effect - LED bounces back and forth
 */
int Effects_KnightRider(uint8_t position, WS2815_Color_t color)
{
    WS2815_Color_t black = {0, 0, 0};

    // Clear all LEDs
    for (uint8_t i = 0; i < 29; i++) {
        WS2815_SetLED(i, black);
    }

    // Map position 0-56 to LED 0-28 and back (bounce effect)
    uint8_t led_pos;
    if (position < 29) {
        led_pos = position;  // Forward
    } else {
        led_pos = 56 - position;  // Backward
    }

    // Main LED at full brightness
    WS2815_SetLED(led_pos, color);

    // Add trailing LEDs with decreasing brightness
    if (led_pos > 0) {
        WS2815_Color_t trail1 = {color.r/2, color.g/2, color.b/2};
        WS2815_SetLED(led_pos - 1, trail1);
    }
    if (led_pos > 1) {
        WS2815_Color_t trail2 = {color.r/4, color.g/4, color.b/4};
        WS2815_SetLED(led_pos - 2, trail2);
    }

    return 0;
}

/**
 * @brief Display digit with color cycling through spectrum (non-blocking)
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0 = hide, 1 = show)
 * @param step Current color step (0-255, cycles through HSV spectrum)
 * @return 0 on success, negative on error
 *
 * This is a non-blocking function - call it repeatedly with incrementing step
 * to create a rainbow color cycling effect on the displayed digit.
 */
int Effects_RainbowDigit(uint8_t digit, uint8_t show_dp, uint8_t step)
{
    if (digit > 9) {
        return -1;  // Invalid digit
    }

    // Convert HSV hue to RGB color
    // step 0-255 cycles through full color spectrum
    WS2815_Color_t color = Effects_HSV_to_RGB(step, 255, 255);

    // Display the digit with current rainbow color
    WS2815_DisplayDigit(digit, color, show_dp);

    return 0;
}

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
 */
int Effects_RainbowGradientDigit(uint8_t digit, uint8_t show_dp, uint8_t step)
{
    if (digit > 9) {
        return -1;  // Invalid digit
    }

    // 7-segment encoding for digits 0-9
    // Bit mapping: bit 0=a, bit 1=b, bit 2=c, bit 3=d, bit 4=e, bit 5=f, bit 6=g
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

    // LED mapping for segments
    // Segment a (0): LED 0-3
    // Segment b (1): LED 4-7
    // Segment c (2): LED 8-11
    // Segment d (3): LED 12-15
    // Segment e (4): LED 16-19
    // Segment f (5): LED 24-27
    // Segment g (6): LED 20-23
    const uint8_t segment_start[7] = {0, 4, 8, 12, 16, 24, 20};

    uint8_t segments = digit_segments[digit];
    WS2815_Color_t black = {0, 0, 0};

    // Set rainbow gradient on active LEDs, black on inactive
    for (uint8_t seg = 0; seg < 7; seg++) {
        uint8_t start_led = segment_start[seg];

        if (segments & (1 << seg)) {
            // Segment is active - set rainbow gradient
            for (uint8_t i = 0; i < 4; i++) {
                // Each LED gets different hue based on its index
                uint8_t hue = step + (start_led + i) * 9;  // ~9 = 255/29 for distribution
                WS2815_Color_t color = Effects_HSV_to_RGB(hue, 255, 255);
                WS2815_SetLED(start_led + i, color);
            }
        } else {
            // Segment is inactive - turn off
            for (uint8_t i = 0; i < 4; i++) {
                WS2815_SetLED(start_led + i, black);
            }
        }
    }

    // Handle decimal point (LED 28)
    if (show_dp) {
        uint8_t hue = step + 28 * 9;
        WS2815_Color_t color = Effects_HSV_to_RGB(hue, 255, 255);
        WS2815_SetLED(28, color);
    } else {
        WS2815_SetLED(28, black);
    }

    return 0;
}

/**
 * @brief Demo: Continuous rainbow on single digit (infinite)
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0/1)
 */
void Demo_RainbowDigit_Continuous(uint8_t digit, uint8_t show_dp)
{
    uint8_t step = 0;
    while(1) {
        Effects_RainbowDigit(digit, show_dp, step);
        WS2815_Update();
        while(WS2815_IsBusy());
        HAL_Delay(10);
        step++;
    }
}

/**
 * @brief Demo: Rainbow gradient digit - each LED has different color
 * Cycles through digits 0-9, each with animated rainbow gradient
 */
void Demo_RainbowGradientDigit(void)
{
    for (uint8_t digit = 0; digit <= 9; digit++) {
        uint8_t show_dp = (digit % 2 == 0) ? 1 : 0;

        // Animate through color spectrum
        for (uint16_t step = 0; step < 256; step++) {
            Effects_RainbowGradientDigit(digit, show_dp, step);
            WS2815_Update();
            while(WS2815_IsBusy());
            HAL_Delay(10);
        }
    }
}

/**
 * @brief Demo: Continuous rainbow gradient on single digit (infinite)
 * @param digit Digit to display (0-9)
 * @param show_dp Show decimal point (0/1)
 */
void Demo_RainbowGradientDigit_Continuous(uint8_t digit, uint8_t show_dp)
{
    uint8_t step = 0;
    while(1) {
        Effects_RainbowGradientDigit(digit, show_dp, step);
        WS2815_Update();
        while(WS2815_IsBusy());
        HAL_Delay(10);
        step++;
    }
}
