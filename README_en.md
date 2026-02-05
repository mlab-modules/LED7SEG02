# LED7SEG02 - Project Documentation

Documentation for LED7SEG02 project - 7-segment display with WS2815 addressable RGB LEDs and RS485 communication.

## Project Overview

This project implements control of a 7-segment display consisting of 29 WS2815 RGB LEDs using STM32G031F8Px microcontroller. The display is controlled via RS485 bus using a defined protocol.

### Basic Specifications

- **MCU:** STM32G031F8Px (Cortex-M0+ @ 64MHz)
- **LEDs:** 29× WS2815 RGB (7 segments × 4 LEDs + 1 decimal point)
- **Communication:** RS485 @ 921600 baud
- **Power:** 3.3V (MCU), 12V (LEDs)

## Documentation Structure

### [WS2815_Driver_en.md](WS2815_Driver_en.md)
Complete WS2815 driver documentation including:
- API reference
- Visual effects library
- Usage examples
- Troubleshooting

### [Hardware_Configuration_en.md](Hardware_Configuration_en.md)
Hardware configuration and specifications:
- MCU pinout
- Peripheral settings
- Wiring diagram

### [protocol_en.md](protocol_en.md)
RS485 communication protocol:
- Frame structure
- Command list
- CRC-8 calculation
- Communication examples

### [firmware_en.md](firmware_en.md)
Firmware documentation:
- Software architecture
- Module descriptions
- Data flow

## Quick Start

### 1. Building the Project

Open project in **STM32CubeIDE**:

```
File → Open Projects from File System
→ Import source: LED7SEG02\sw
```

Build project: `Project → Build Project` (Ctrl+B)

### 2. Basic Usage

```c
#include "sw_ws2815.h"
#include "sw_effects.h"

void APP_Init(void) {
    WS2815_Init();
    WS2815_SetBrightness(50);
    Effects_Init();
    RS485_Init();
    Protocol_Init();
    RS485_StartReceive();
}

void APP_Main(void) {
    WS2815_Color_t green = {0, 255, 0};

    // Display digit
    WS2815_DisplayDigit(8, green, 1);
    WS2815_Update();
    while(WS2815_IsBusy());
}
```

### 3. Hardware Connection

```
PA0 (MCU) ──────► DI (WS2815)
GND (MCU) ──────► GND (WS2815)
12V PSU   ──────► +12V (WS2815)

PA2 (MCU) ──────► DI (MAX3485)
PA3 (MCU) ◄────── RO (MAX3485)
PA1 (MCU) ──────► DE/RE (MAX3485)
```

## Project Structure

```
LED7SEG02/
├── doc/                          # Documentation
│   ├── README.md                 # This file (CZ)
│   ├── README_en.md              # This file (EN)
│   ├── WS2815_Driver.md          # Driver documentation (CZ)
│   ├── WS2815_Driver_en.md       # Driver documentation (EN)
│   ├── Hardware_Configuration.md # Hardware documentation (CZ)
│   ├── Hardware_Configuration_en.md # Hardware documentation (EN)
│   ├── protocol_cz.md            # Protocol (CZ)
│   ├── protocol_en.md            # Protocol (EN)
│   ├── firmware_cz.md            # Firmware (CZ)
│   └── firmware_en.md            # Firmware (EN)
├── hw/                           # Hardware design files
├── sw/                           # Firmware
│   ├── Core/                     # STM32 HAL core
│   ├── Drivers/                  # STM32 HAL drivers
│   ├── src/                      # Application code
│   │   ├── app.c
│   │   └── app.h
│   ├── sw/                       # Custom drivers
│   │   ├── sw_ws2815.c/.h        # WS2815 driver
│   │   ├── sw_rs485.c/.h         # RS485 driver
│   │   └── sw_protocol.c/.h      # Communication protocol
│   └── sw_effects/               # Visual effects
│       ├── sw_effects.c/.h
│       └── README.md
└── app/                          # Python test application
    ├── led7seg_test.py
    └── config.json
```

## Key Features

### WS2815 Driver

- Hardware PWM + DMA (no bit-banging)
- 800 kHz bitrate
- Non-blocking operations
- Adjustable brightness (10-254)
- Precise timing (TIM2 @ 64MHz)
- Low CPU usage

### Visual Effects

- Rainbow - rainbow color cycle
- Spinning - spinning LED
- Breathing - breathing effect
- Segment Chase - running segments
- Knight Rider - K.I.T.T. effect
- Loading Bar - progress bar
- Rainbow Digit - digit with changing color
- Rainbow Gradient - digit with rainbow gradient

### RS485 Communication

- 921600 baud, 8N1
- Hardware RS485 mode (automatic DE control)
- Interrupt-based receive/transmit
- IDLE line detection for frame end
- CRC-8 validation
- Response time < 2ms

## Peripheral Configuration

| Peripheral | Pin   | Function         |
|------------|-------|------------------|
| TIM2_CH1   | PA0   | WS2815 Data      |
| USART2_TX  | PA2   | RS485 TX         |
| USART2_RX  | PA3   | RS485 RX         |
| USART2_DE  | PA1   | RS485 Direction  |
| GPIO       | PA4   | Address bit 0    |
| GPIO       | PA12  | Address bit 1    |
| GPIO       | PB0   | Address bit 2    |
| GPIO       | PA11  | Address bit 3    |

## Segment Mapping

```
    aaaa
   f    b
   f    b
    gggg
   e    c
   e    c
    dddd  dp
```

| Segment | Index | LED positions |
|---------|-------|---------------|
| a | 0 | 0-3 |
| b | 1 | 4-7 |
| c | 2 | 8-11 |
| d | 3 | 12-15 |
| e | 4 | 16-19 |
| f | 5 | 20-23 |
| g | 6 | 24-27 |
| dp | 7 | 28 |

## API Quick Reference

### Initialization

```c
WS2815_Init();                      // Initialize driver
WS2815_SetBrightness(brightness);   // Set brightness (10-254)
Effects_Init();                     // Initialize effects
```

### Display Control

```c
WS2815_DisplayDigit(digit, color, show_dp);  // Display digit 0-9
WS2815_SetSegment(segment, color);           // Set segment
WS2815_SetLED(index, color);                 // Set LED
WS2815_Clear();                              // Turn off all LEDs
```

### Update

```c
WS2815_Update();                    // Send data (non-blocking)
WS2815_IsBusy();                    // Check if busy
```

### Effects

```c
Effects_Rainbow(step);                          // Rainbow effect
Effects_RainbowDigit(digit, show_dp, step);     // Rainbow on digit
Effects_Breathing(step, color);                 // Breathing
Effects_KnightRider(pos, color);                // K.I.T.T.
```

## Troubleshooting

### LEDs not lighting

1. Check 12V power supply
2. Check PA0 → DI wiring
3. Check common ground
4. Verify DMA buffer (WS2815_IsBusy)

### RS485 not communicating

1. Check baud rate (921600)
2. Verify A/B wire connections
3. Check BCD switch address
4. Measure DE pin activity

### Wrong colors

- Check brightness setting (range 10-254)
- Check power supply voltage (min 11V)

## Version History

### v1.0 (2026-02)

- WS2815 driver implementation
- RS485 communication with protocol
- Visual effects library
- Documentation

## License and Author

- **Project:** LED7SEG02
- **Author:** Richard Roztocil
- **Date:** February 2026

## References

- [WS2815 Datasheet](https://cdn-shop.adafruit.com/product-files/2757/WS2815.pdf)
- [STM32G031 Datasheet](https://www.st.com/resource/en/datasheet/stm32g031f8.pdf)
- [MAX3485 Datasheet](https://www.maximintegrated.com/en/products/interface/transceivers/MAX3485.html)
