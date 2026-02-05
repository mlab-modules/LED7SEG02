# LED7SEG02 - Hardware Configuration

## MCU Specifications

- **MCU:** STM32G031F8Px
- **Package:** TSSOP20
- **Core:** ARM Cortex-M0+ @ 64 MHz
- **Flash:** 64 KB
- **RAM:** 8 KB
- **Voltage:** 2.0V - 3.6V

## System Clock Configuration

```
HSI (16 MHz)
  └─> PLL (×8, ÷2)
       └─> SYSCLK: 64 MHz
            ├─> AHB: 64 MHz
            └─> APB: 64 MHz
```

### Clock Settings

```c
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
RCC_OscInitStruct.HSIState = RCC_HSI_ON;
RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
RCC_OscInitStruct.PLL.PLLN = 8;
RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
```

## Pinout

### Port A

| Pin | Function    | Mode          | Description                |
|-----|-------------|---------------|----------------------------|
| PA0 | TIM2_CH1    | AF            | **WS2815 Data Output**     |
| PA1 | USART2_DE   | AF            | RS485 Direction Enable     |
| PA2 | USART2_TX   | AF            | RS485 Transmit             |
| PA3 | USART2_RX   | AF            | RS485 Receive              |
| PA4 | addr_0      | Input Pull-up | Address bit 0 (active LOW) |
| PA11| addr_3      | Input Pull-up | Address bit 3 (active LOW) |
| PA12| addr_1      | Input Pull-up | Address bit 1 (active LOW) |
| PA13| SWDIO       | Debug         | SWD Data                   |
| PA14| SWCLK       | Debug         | SWD Clock                  |

### Port B

| Pin | Function    | Mode          | Description                |
|-----|-------------|---------------|----------------------------|
| PB0 | addr_2      | Input Pull-up | Address bit 2 (active LOW) |

## Peripheral Configuration

### TIM2 (WS2815 PWM)

**Purpose:** Generate precise PWM signal for WS2815 LED protocol

```c
htim2.Instance = TIM2;
htim2.Init.Prescaler = 0;              // No prescaling
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = 79;                // 1.25µs @ 64MHz
htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
```

**Channel 1 (PA0):**
- Mode: PWM Mode 1
- Duty cycle: Variable (24 or 52) for bit encoding
- Driven by DMA

### DMA1 Channel 1 (WS2815)

```c
hdma_ws2815.Instance = DMA1_Channel1;
hdma_ws2815.Init.Request = DMA_REQUEST_TIM2_CH1;
hdma_ws2815.Init.Direction = DMA_MEMORY_TO_PERIPH;
hdma_ws2815.Init.PeriphInc = DMA_PINC_DISABLE;
hdma_ws2815.Init.MemInc = DMA_MINC_ENABLE;
hdma_ws2815.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
hdma_ws2815.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
hdma_ws2815.Init.Mode = DMA_NORMAL;
hdma_ws2815.Init.Priority = DMA_PRIORITY_HIGH;
```

### USART2 (RS485)

**Purpose:** RS485 communication interface

```c
huart2.Instance = USART2;
huart2.Init.BaudRate = 921600;
huart2.Init.WordLength = UART_WORDLENGTH_8B;
huart2.Init.StopBits = UART_STOPBITS_1;
huart2.Init.Parity = UART_PARITY_NONE;
huart2.Init.Mode = UART_MODE_TX_RX;
huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
huart2.Init.OverSampling = UART_OVERSAMPLING_16;
```

**RS485 Mode:**
- DE polarity: High
- DE assertion: 0 bit times
- DE deassertion: 0 bit times
- Automatic DE pin control (hardware RS485 mode)

## GPIO Configuration

### WS2815 Data Pin (PA0)

```c
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // Alternate Function Push-Pull
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF2_TIM2;   // TIM2_CH1
```

### RS485 Pins

```c
// TX (PA2) and RX (PA3)
GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF1_USART2;

// DE (PA1)
GPIO_InitStruct.Pin = GPIO_PIN_1;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
```

### Address Pins (BCD Switch)

```c
// addr_0 (PA4), addr_1 (PA12), addr_3 (PA11)
GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_11 | GPIO_PIN_12;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;

// addr_2 (PB0)
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;
```

**Note:** Address pins are active LOW (closed switch = 0)

## Interrupt Configuration

### NVIC Priorities

| Interrupt           | Priority | Description                |
|---------------------|----------|----------------------------|
| DMA1_Channel1_IRQn  | 0        | WS2815 DMA (highest)       |
| USART2_IRQn         | 1        | RS485 communication        |
| SysTick_IRQn        | 3        | System tick timer          |

### DMA1_Channel1_IRQHandler

```c
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_ws2815);
}
```

### USART2_IRQHandler

```c
void USART2_IRQHandler(void)
{
    RS485_IRQHandler();
}
```

## Wiring Diagrams

### WS2815 LED Strip

```
STM32G031F8Px                    WS2815 LED Strip
┌─────────────┐                  ┌──────────┐
│             │                  │          │
│  PA0 (TIM2) ├─────────────────►│ DI       │
│             │   (Data)         │          │
│         GND ├─────────────────►│ GND      │
│             │                  │          │
└─────────────┘                  │ +12V     │◄─── 12V PSU
                                 └──────────┘
```

### RS485 Transceiver (MAX3485)

```
STM32G031F8Px                    MAX3485
┌─────────────┐                  ┌──────────┐
│             │                  │          │
│  PA2 (TX)   ├─────────────────►│ DI       │
│             │                  │          │
│  PA3 (RX)   │◄─────────────────┤ RO       │
│             │                  │          │
│  PA1 (DE)   ├─────────────────►│ DE       │
│             │        ┌────────►│ /RE      │
│             │        │         │          │
│         GND ├────────┴────────►│ GND      │
│             │                  │          │
│        3.3V ├─────────────────►│ VCC      │
│             │                  │          │
└─────────────┘                  │ A  ◄────►│ RS485 Bus
                                 │ B  ◄────►│
                                 └──────────┘
```

**Note:** DE and /RE are connected - controlled from PA1

### BCD Address Switch

```
STM32G031F8Px                    BCD Switch
┌─────────────┐                  ┌──────────┐
│             │                  │          │
│  PA4 (addr0)│◄─────────────────┤ bit 0    │
│             │                  │          │
│  PA12(addr1)│◄─────────────────┤ bit 1    │
│             │                  │          │
│  PB0 (addr2)│◄─────────────────┤ bit 2    │
│             │                  │          │
│  PA11(addr3)│◄─────────────────┤ bit 3    │
│             │                  │          │
│         GND ├─────────────────►│ Common   │
│             │                  │          │
└─────────────┘                  └──────────┘
```

**Note:** Switches are active LOW (closed = GND = 0)

## Connection Notes

### Power Supply

1. **MCU (3.3V):**
   - 100nF ceramic capacitor on VDD/VSS (close to MCU)
   - 10µF tantalum capacitor on VDD/VSS

2. **WS2815 (12V):**
   - Current: ~60mA per LED at full white
   - For 29 LEDs: ~1.8A max
   - 1000µF electrolytic capacitor on 12V input

### WS2815 Data Line

- PA0 (3.3V logic) directly to WS2815 DI
- Optional: 330Ω series resistor for protection
- Max cable length: 5 meters

### RS485 Bus

- 120Ω termination resistor at both ends of bus
- Twisted pair for A/B wires
- Common ground between devices

## Power Consumption

### MCU

- Active mode @ 64MHz: ~15 mA
- Sleep mode: ~2 mA
- Stop mode: ~2 µA

### WS2815 LEDs

Per LED at 50% brightness:
- Red: 20 mA
- Green: 20 mA
- Blue: 20 mA
- White: 60 mA

Total for 29 LEDs:
- All off: 0 mA
- All white @ 50%: ~0.9 A
- All white @ 100%: ~1.8 A

## Memory Map

### Flash (64 KB)

```
0x0800 0000 - 0x0800 FFFF    Flash memory
0x0800 0000 - 0x0800 0200    Interrupt vectors
0x0800 0200 - 0x0800 XXXX    Application code
0x0800 XXXX - 0x0800 FFFF    Constants/data
```

### RAM (8 KB)

```
0x2000 0000 - 0x2000 1FFF    SRAM
```

### RAM Usage

| Buffer | Size | Description |
|--------|------|-------------|
| DMA buffer | ~2 KB | WS2815 data (29 LEDs × 24 bits + reset) |
| RX ring buffer | 64 B | RS485 reception |
| TX buffer | 64 B | RS485 transmission |
| Frame buffer | 64 B | Incoming frame |
| Response buffer | 13 B | Response |

## HAL Configuration

### stm32g0xx_hal_conf.h

Required modules:

```c
#define HAL_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED      // Required for TIM2
#define HAL_DMA_MODULE_ENABLED      // Required for DMA
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED     // For RS485
```

## Debug Configuration

### SWD

- **SWDIO:** PA13
- **SWCLK:** PA14

## Board Bring-up Checklist

- [ ] 3.3V power supply stable
- [ ] System clock 64 MHz verified
- [ ] SWD connection working
- [ ] PA0 configured as TIM2_CH1
- [ ] TIM2 clock enabled
- [ ] DMA1 clock enabled
- [ ] WS2815 12V power supply connected
- [ ] Common ground between MCU and LEDs
- [ ] First LED lights up with test code
- [ ] All 29 LEDs addressable
- [ ] RS485 communication working
- [ ] BCD address read correctly

## Troubleshooting

### PA0 not outputting signal

1. Check GPIO clock: `__HAL_RCC_GPIOA_CLK_ENABLE()`
2. Check TIM2 clock: `__HAL_RCC_TIM2_CLK_ENABLE()`
3. Check alternate function: `GPIO_AF2_TIM2`
4. Measure with oscilloscope: Should see ~800kHz signal

### DMA not working

1. Check DMA1 clock: `__HAL_RCC_DMA1_CLK_ENABLE()`
2. Check DMA request: `DMA_REQUEST_TIM2_CH1`
3. Check NVIC: `HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn)`
4. Check TIM2 DMA: `__HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_CC1)`

### RS485 not communicating

1. Check baud rate (921600)
2. Check A/B wiring (may be swapped)
3. Measure DE pin activity during transmission
4. Check termination resistors

### Wrong address

1. Check pull-up on address pins
2. Check that switches connect to GND
3. Test individual bits

## References

- [STM32G031 Datasheet](https://www.st.com/resource/en/datasheet/stm32g031f8.pdf)
- [STM32G0 Reference Manual RM0444](https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x0-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [WS2815 Datasheet](https://cdn-shop.adafruit.com/product-files/2757/WS2815.pdf)
- [MAX3485 Datasheet](https://www.maximintegrated.com/en/products/interface/transceivers/MAX3485.html)
