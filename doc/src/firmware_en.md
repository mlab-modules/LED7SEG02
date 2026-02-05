# LED7SEG02 - Firmware Documentation

## Overview

Firmware for 7-segment display with addressable WS2815 RGB LEDs and RS485 communication.

### Hardware

- **MCU**: STM32G031F8PX (Cortex-M0+, 64KB Flash, 8KB RAM)
- **Clock**: 64 MHz
- **LEDs**: WS2815 RGB (29 LEDs - 7 segments × 4 LEDs + 1 decimal point)
- **Communication**: RS485 via MAX3485 transceiver
- **Power**: 12V for WS2815

### Pinout

| Pin | Function | Description |
|-----|----------|-------------|
| PA2 | USART2_TX | RS485 TX (to MAX3485 DI) |
| PA3 | USART2_RX | RS485 RX (from MAX3485 RO) |
| PA1 | USART2_DE | RS485 DE/RE control |
| PA0 | TIM2_CH1 | WS2815 data (via DMA) |
| PA4 | GPIO Input | Address bit 0 |
| PA12 | GPIO Input | Address bit 1 |
| PB0 | GPIO Input | Address bit 2 |
| PA11 | GPIO Input | Address bit 3 |

## Software Architecture

### Project Structure

```
sw/
├── Core/                    # STM32 HAL and configuration
│   ├── Inc/
│   └── Src/
│       ├── main.c          # Main loop, initialization
│       ├── stm32g0xx_it.c  # Interrupt handlers
│       └── stm32g0xx_hal_msp.c
├── src/
│   ├── app.c               # Application logic
│   └── app.h
├── sw/
│   ├── sw_ws2815.c/.h      # WS2815 LED driver
│   ├── sw_rs485.c/.h       # RS485 driver
│   └── sw_protocol.c/.h    # Communication protocol
└── sw_effects/
    ├── sw_effects.c/.h     # Visual effects
    └── README.md
```

### Modules

#### sw_ws2815 - WS2815 Driver

Controls addressable RGB LEDs using TIM3 + DMA.

**WS2815 Timing:**
- Period: 1.25µs (800kHz)
- T0H: 375ns, T0L: 875ns
- T1H: 812.5ns, T1L: 437.5ns
- Reset: >280µs

**Main Functions:**
```c
void WS2815_Init(void);
void WS2815_SetLED(uint8_t index, WS2815_Color_t color);
void WS2815_SetSegment(uint8_t segment, WS2815_Color_t color);
void WS2815_DisplayDigit(uint8_t digit, WS2815_Color_t color, uint8_t show_dp);
void WS2815_SetBrightness(uint8_t brightness);
void WS2815_Update(void);
void WS2815_Clear(void);
uint8_t WS2815_IsBusy(void);
```

#### sw_rs485 - RS485 Driver

Interrupt-based RS485 communication with ring buffer.

**Features:**
- RXNE interrupt for byte reception
- IDLE line detection for frame end
- TXE/TC interrupts for transmission
- Hardware RS485 mode (automatic DE control)

**Main Functions:**
```c
RS485_Status_t RS485_Init(void);
RS485_Status_t RS485_StartReceive(void);
RS485_Status_t RS485_TransmitIT(uint8_t *data, uint16_t length);
void RS485_SetFrameCallback(RS485_FrameCallback_t callback);
uint8_t RS485_Available(void);
uint8_t RS485_Read(void);
void RS485_IRQHandler(void);
```

#### sw_protocol - Communication Protocol

Command and response processing.

**Features:**
- Frame processing in IDLE interrupt (minimal latency)
- Deferred display update (DMA safe)
- CRC-8 validation

**Main Functions:**
```c
void Protocol_Init(void);
void Protocol_Process(void);
void Protocol_HandleFrame(uint8_t *data, uint8_t length);
uint8_t Protocol_GetAddress(void);
```

#### sw_effects - Visual Effects

Library of pre-built animations.

**Available Effects:**
- Rainbow - rainbow color cycle
- Spinning - spinning LED
- Breathing - breathing effect
- Segment Chase - running segments
- Knight Rider - K.I.T.T. effect
- Loading Bar - progress bar
- Rainbow Digit - digit with changing color
- Rainbow Gradient - digit with rainbow gradient

## Data Flow

### Command Reception and Processing

```
1. RXNE interrupt → byte to ring buffer and frame buffer
2. IDLE interrupt → Protocol_HandleFrame() called
3. Frame parsed and validated (CRC)
4. Command processed, state updated
5. Response sent (TX interrupt)
6. display_update_pending flag set
7. Main loop → Protocol_Process() → UpdateDisplay()
8. WS2815_Update() → DMA transfer → LEDs updated
```

### Response Timing

- Firmware processing: <100µs
- Total response time: ~1.5ms (limited by USB converter)

## Configuration

### USART2 (RS485)

```c
BaudRate = 921600
WordLength = 8 bits
StopBits = 1
Parity = None
Mode = TX_RX
HwFlowCtl = None
OverSampling = 16
DE Polarity = High
```

### TIM3 (WS2815)

```c
Prescaler = 0
Period = 79 (80 cycles = 1.25µs at 64MHz)
Channel 1 PWM mode
DMA Circular mode
```

### NVIC Priority

| Interrupt | Priority | Description |
|-----------|----------|-------------|
| DMA1_Channel1 | 0 | WS2815 (highest) |
| USART2 | 1 | RS485 communication |
| SysTick | 3 | HAL timing |

## Memory Map

| Region | Size | Usage |
|--------|------|-------|
| Flash | 64 KB | Code + constants |
| RAM | 8 KB | Variables + buffers |

### RAM Usage

| Buffer | Size | Description |
|--------|------|-------------|
| DMA buffer | ~2 KB | WS2815 data (29 LEDs × 24 bits + reset) |
| RX ring buffer | 64 B | RS485 reception |
| TX buffer | 64 B | RS485 transmission |
| Frame buffer | 64 B | Incoming frame |
| Response buffer | 13 B | Response |

## Building and Programming

### Requirements

- STM32CubeIDE or ARM GCC toolchain
- ST-Link programmer

### Building

```bash
cd sw/Debug
make clean
make all
```

### Programming

```bash
st-flash write LED7SEG02.bin 0x08000000
```

Or using STM32CubeIDE: Run → Debug/Run

## Troubleshooting

### LEDs not lighting

1. Check 12V power supply
2. Verify DMA buffer (WS2815_IsBusy)
3. Check signal timing with oscilloscope

### RS485 not communicating

1. Check baud rate (921600)
2. Verify A/B wire connections
3. Check BCD switch address
4. Measure DE pin activity

### Slow response

1. Verify processing in IDLE interrupt
2. Check interrupt priorities
3. USB converter may be bottleneck

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02 | Initial release |
