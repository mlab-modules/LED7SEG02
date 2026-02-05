# WS2815 LED Driver - Documentation

## Overview

Driver for controlling WS2815 addressable RGB LEDs on STM32G031F8Px platform. The driver uses TIM2 + DMA to generate precise PWM signal compatible with WS2815 protocol.

## Specifications

- **MCU:** STM32G031F8Px (Cortex-M0+)
- **System clock:** 64 MHz
- **LED type:** WS2815 RGB addressable LEDs
- **Communication:** 1-wire, 800 kHz bitrate
- **LED count:** 29 (7 segments × 4 LEDs + 1 decimal point)
- **Color format:** GRB (Green-Red-Blue)

## Hardware Configuration

### Connection

| Signal | MCU Pin | Peripheral | Description |
|--------|---------|------------|-------------|
| Data   | PA0     | TIM2_CH1   | Data signal for WS2815 |
| VCC    | -       | -          | 12V power for LEDs |
| GND    | GND     | -          | Common ground |

### Peripherals

- **TIM2:** Timer for PWM generation
  - Channel 1 (PA0)
  - Frequency: 800 kHz
  - Prescaler: 0
  - Period: 79 (@ 64MHz = 1.25µs)

- **DMA1 Channel 1:** PWM data transfer
  - Request: TIM2_CH1
  - Direction: Memory-to-Peripheral
  - Mode: Normal
  - Priority: High (0)

## WS2815 Timing

WS2815 uses the following timing:

| Bit | T0H    | T0L     | T1H     | T1L    | Total  |
|-----|--------|---------|---------|--------|--------|
| 0   | 375ns  | 875ns   | -       | -      | 1.25µs |
| 1   | -      | -       | 812.5ns | 437.5ns| 1.25µs |
| RES | -      | >280µs  | -       | -      | -      |

### Driver Implementation

```c
#define WS2815_TIM_PERIOD   80    // 1.25µs @ 64MHz
#define WS2815_BIT_0_DUTY   24    // 375ns (30% duty)
#define WS2815_BIT_1_DUTY   52    // 812.5ns (65% duty)
#define WS2815_RESET_SLOTS  250   // >280µs reset period
```

## File Structure

```
LED7SEG02/sw/
├── sw/
│   ├── sw_ws2815.c       # Driver implementation
│   └── sw_ws2815.h       # Public API
├── sw_effects/
│   ├── sw_effects.c      # Effects library
│   └── sw_effects.h
├── Core/
│   └── Src/
│       └── stm32g0xx_it.c  # DMA interrupt handler
└── src/
    └── app.c             # Application code
```

## API Reference

### Data Types

```c
typedef struct {
    uint8_t r;  // Red (0-255)
    uint8_t g;  // Green (0-255)
    uint8_t b;  // Blue (0-255)
} WS2815_Color_t;
```

### Functions

#### WS2815_Init()

```c
void WS2815_Init(void);
```

Initializes WS2815 driver (GPIO, TIM2, DMA).

---

#### WS2815_SetLED()

```c
void WS2815_SetLED(uint8_t index, WS2815_Color_t color);
```

Sets color of single LED in buffer.

**Parameters:**
- `index`: LED index (0-28)
- `color`: RGB color

**Note:** Change takes effect after calling `WS2815_Update()`

---

#### WS2815_SetSegment()

```c
void WS2815_SetSegment(uint8_t segment, WS2815_Color_t color);
```

Sets color of entire segment (4 LEDs).

**Parameters:**
- `segment`: Segment index (0-7)
- `color`: RGB color

---

#### WS2815_DisplayDigit()

```c
void WS2815_DisplayDigit(uint8_t digit, WS2815_Color_t color, uint8_t show_dp);
```

Displays digit 0-9 on display.

**Parameters:**
- `digit`: Digit (0-9)
- `color`: RGB color
- `show_dp`: 1 = show decimal point

---

#### WS2815_Update()

```c
void WS2815_Update(void);
```

Sends buffer to LED strip via DMA.

**Note:** Function is non-blocking, uses DMA transfer

---

#### WS2815_Clear()

```c
void WS2815_Clear(void);
```

Turns off all LEDs (sets to black).

---

#### WS2815_SetBrightness()

```c
void WS2815_SetBrightness(uint8_t brightness);
```

Sets global brightness of all LEDs.

**Parameters:**
- `brightness`: 0 (off), 10-254 (functional range)

**Notes:**
- Values 1-9 are automatically set to 10
- Values > 254 are automatically set to 254

---

#### WS2815_IsBusy()

```c
uint8_t WS2815_IsBusy(void);
```

Checks if DMA transfer is in progress.

**Return:** 1 if busy, 0 if ready

## Effects Library (sw_effects)

### Available Effects

| Function | Description |
|----------|-------------|
| `Effects_Rainbow(step)` | Rainbow cycle on all LEDs |
| `Effects_SpinningLED(pos, color)` | Spinning LED (stand-by) |
| `Effects_Breathing(step, color)` | Breathing effect |
| `Effects_SegmentChase(step, color)` | Running segments |
| `Effects_KnightRider(pos, color)` | K.I.T.T. effect with tail |
| `Effects_LoadingBar(progress, color)` | Progress bar |
| `Effects_RainbowDigit(digit, show_dp, step)` | Digit with changing rainbow color |
| `Effects_RainbowGradientDigit(digit, show_dp, step)` | Digit with rainbow gradient |

### Helper Functions

```c
WS2815_Color_t Effects_HSV_to_RGB(uint8_t h, uint8_t s, uint8_t v);
```

### Difference Between Rainbow Effects

| Effect | Description |
|--------|-------------|
| `Effects_RainbowDigit` | Entire digit lights with one color, color changes over time |
| `Effects_RainbowGradientDigit` | Each LED has different rainbow color (gradient), gradient moves |

## Usage

### Example 1: Basic Usage

```c
#include "sw_ws2815.h"

void app_main(void) {
    WS2815_Init();
    WS2815_SetBrightness(50);

    WS2815_Color_t green = {0, 255, 0};

    // Display digit "8" with decimal point
    WS2815_DisplayDigit(8, green, 1);
    WS2815_Update();
    while(WS2815_IsBusy());
}
```

### Example 2: Rainbow Animation

```c
#include "sw_ws2815.h"
#include "sw_effects.h"

void rainbow_animation(void) {
    Effects_Init();

    while(1) {
        for(uint16_t step = 0; step < 256; step++) {
            Effects_Rainbow(step);
            WS2815_Update();
            while(WS2815_IsBusy());
            HAL_Delay(20);
        }
    }
}
```

### Example 3: Rainbow Gradient on Digit

```c
void rainbow_digit_animation(void) {
    while(1) {
        for(uint16_t step = 0; step < 256; step++) {
            Effects_RainbowGradientDigit(8, 1, step);
            WS2815_Update();
            while(WS2815_IsBusy());
            HAL_Delay(10);
        }
    }
}
```

## Segment Mapping

```
    aaaa        Segment a: LED 0-3
   f    b       Segment b: LED 4-7
   f    b       Segment c: LED 8-11
    gggg        Segment d: LED 12-15
   e    c       Segment e: LED 16-19
   e    c       Segment f: LED 20-23
    dddd  dp    Segment g: LED 24-27
                Decimal point: LED 28
```

## DMA Buffer Structure

```
DMA buffer contains:
- 29 LEDs × 24 bits = 696 slots for data
- 250 slots for reset period (>280µs LOW)
- Total: ~1940 bytes
```

## Troubleshooting

### LEDs not lighting

1. **Check power supply:** WS2815 needs 12V
2. **Check wiring:** Data pin on PA0
3. **Check system clock:** Must be 64 MHz
4. **Check HAL_TIM_MODULE_ENABLED** in `stm32g0xx_hal_conf.h`

### Wrong colors

- WS2815 uses GRB order (driver handles this)
- Check brightness setting (range 10-254)
- Check power supply voltage (low voltage causes color issues)

### Flickering when changing data

- Cause: Missing explicit reset period in DMA buffer
- Solution: Driver includes 250 reset slots at end of DMA buffer
- WS2815 needs >280µs LOW signal to latch data to LEDs

### DMA errors

- Check that DMA1_Channel1 is not used elsewhere
- Check interrupt priority (0 = highest)

## Performance

### Timing

- Refresh rate: ~275 Hz for 29 LEDs
- Transfer time: ~0.9 ms for 29 LEDs (29 × 24 bits × 1.25µs + reset)
- CPU usage: Minimal (uses DMA)

### Memory

- RAM: ~2 KB (led_buffer + dma_buffer)
- Flash: ~1.5 KB (driver + effects code)

## References

- [WS2815 Datasheet](https://cdn-shop.adafruit.com/product-files/2757/WS2815.pdf)
- [STM32G0 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x0-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
