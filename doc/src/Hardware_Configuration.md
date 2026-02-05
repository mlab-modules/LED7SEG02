# LED7SEG02 - Hardware konfigurace

## MCU Specifikace

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

| Pin | Funkce      | Mód           | Popis                      |
|-----|-------------|---------------|----------------------------|
| PA0 | TIM2_CH1    | AF            | **WS2815 Data Output**     |
| PA1 | USART2_DE   | AF            | RS485 Direction Enable     |
| PA2 | USART2_TX   | AF            | RS485 Transmit             |
| PA3 | USART2_RX   | AF            | RS485 Receive              |
| PA4 | addr_0      | Input Pull-up | Adresa bit 0 (active LOW)  |
| PA11| addr_3      | Input Pull-up | Adresa bit 3 (active LOW)  |
| PA12| addr_1      | Input Pull-up | Adresa bit 1 (active LOW)  |
| PA13| SWDIO       | Debug         | SWD Data                   |
| PA14| SWCLK       | Debug         | SWD Clock                  |

### Port B

| Pin | Funkce      | Mód           | Popis                      |
|-----|-------------|---------------|----------------------------|
| PB0 | addr_2      | Input Pull-up | Adresa bit 2 (active LOW)  |

## Konfigurace periférií

### TIM2 (WS2815 PWM)

**Účel:** Generování přesného PWM signálu pro WS2815 LED protokol

```c
htim2.Instance = TIM2;
htim2.Init.Prescaler = 0;              // Bez dělení
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = 79;                // 1.25µs @ 64MHz
htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
```

**Channel 1 (PA0):**
- Mode: PWM Mode 1
- Duty cycle: Variable (24 nebo 52) pro bit encoding
- Řízeno DMA

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

**Účel:** RS485 komunikační rozhraní

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
- Automatické řízení DE pinu (hardware RS485 mode)

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
// TX (PA2) a RX (PA3)
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

**Poznámka:** Adresové piny jsou aktivní v LOW (sepnutý přepínač = 0)

## Interrupt Configuration

### NVIC Priorities

| Přerušení           | Priorita | Popis                      |
|---------------------|----------|----------------------------|
| DMA1_Channel1_IRQn  | 0        | WS2815 DMA (nejvyšší)      |
| USART2_IRQn         | 1        | RS485 komunikace           |
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

## Schéma zapojení

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

**Poznámka:** DE a /RE jsou spojeny - řízeny z PA1

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

**Poznámka:** Přepínače jsou aktivní v LOW (sepnutý = GND = 0)

## Poznámky k připojení

### Napájení

1. **MCU (3.3V):**
   - 100nF keramický kondenzátor na VDD/VSS (blízko MCU)
   - 10µF tantalový kondenzátor na VDD/VSS

2. **WS2815 (12V):**
   - Proud: ~60mA per LED při full white
   - Pro 29 LED: ~1.8A max
   - 1000µF elektrolytický kondenzátor na 12V vstupu

### Datová linka WS2815

- PA0 (3.3V logic) přímo na WS2815 DI
- Volitelně: 330Ω sériový rezistor pro ochranu
- Max délka kabelu: 5 metrů

### RS485 Bus

- Zakončovací odpor 120Ω na obou koncích sběrnice
- Twisted pair pro A/B vodiče
- Společná zem mezi zařízeními

## Spotřeba

### MCU

- Active mode @ 64MHz: ~15 mA
- Sleep mode: ~2 mA
- Stop mode: ~2 µA

### WS2815 LEDs

Per LED při 50% brightness:
- Red: 20 mA
- Green: 20 mA
- Blue: 20 mA
- White: 60 mA

Celkem pro 29 LED:
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

### RAM využití

| Buffer | Velikost | Popis |
|--------|----------|-------|
| DMA buffer | ~2 KB | WS2815 data (29 LED × 24 bit + reset) |
| RX ring buffer | 64 B | RS485 příjem |
| TX buffer | 64 B | RS485 odesílání |
| Frame buffer | 64 B | Příchozí rámec |
| Response buffer | 13 B | Odpověď |

## HAL Configuration

### stm32g0xx_hal_conf.h

Povinné moduly:

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

## Checklist pro oživení

- [ ] 3.3V napájení stabilní
- [ ] System clock 64 MHz ověřen
- [ ] SWD připojení funkční
- [ ] PA0 konfigurován jako TIM2_CH1
- [ ] TIM2 clock povolen
- [ ] DMA1 clock povolen
- [ ] WS2815 12V napájení připojeno
- [ ] Společná zem mezi MCU a LED
- [ ] První LED svítí s test kódem
- [ ] Všech 29 LED adresovatelných
- [ ] RS485 komunikace funkční
- [ ] BCD adresa správně čtena

## Troubleshooting

### PA0 nevysílá signál

1. Zkontrolovat GPIO clock: `__HAL_RCC_GPIOA_CLK_ENABLE()`
2. Zkontrolovat TIM2 clock: `__HAL_RCC_TIM2_CLK_ENABLE()`
3. Zkontrolovat alternate function: `GPIO_AF2_TIM2`
4. Měřit osciloskopem: Měl by být ~800kHz signál

### DMA nefunguje

1. Zkontrolovat DMA1 clock: `__HAL_RCC_DMA1_CLK_ENABLE()`
2. Zkontrolovat DMA request: `DMA_REQUEST_TIM2_CH1`
3. Zkontrolovat NVIC: `HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn)`
4. Zkontrolovat TIM2 DMA: `__HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_CC1)`

### RS485 nekomunikuje

1. Zkontrolovat baudrate (921600)
2. Zkontrolovat A/B zapojení (může být přehozené)
3. Měřit DE pin aktivitu při odesílání
4. Zkontrolovat zakončovací odpory

### Špatná adresa

1. Zkontrolovat pull-up na adresových pinech
2. Zkontrolovat že přepínače spínají k GND
3. Testovat jednotlivé bity

## Reference

- [STM32G031 Datasheet](https://www.st.com/resource/en/datasheet/stm32g031f8.pdf)
- [STM32G0 Reference Manual RM0444](https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x0-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [WS2815 Datasheet](https://cdn-shop.adafruit.com/product-files/2757/WS2815.pdf)
- [MAX3485 Datasheet](https://www.maximintegrated.com/en/products/interface/transceivers/MAX3485.html)
