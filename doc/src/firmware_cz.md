# LED7SEG02 - Dokumentace firmware

## Přehled

Firmware pro 7-segmentový displej s adresovatelnými RGB LED WS2815 a RS485 komunikací.

### Hardware

- **MCU**: STM32G031F8PX (Cortex-M0+, 64KB Flash, 8KB RAM)
- **Taktování**: 64 MHz
- **LED**: WS2815 RGB (29 LED - 7 segmentů × 4 LED + 1 desetinná tečka)
- **Komunikace**: RS485 přes MAX3485 transceiver
- **Napájení**: 12V pro WS2815

### Pinout

| Pin | Funkce | Popis |
|-----|--------|-------|
| PA2 | USART2_TX | RS485 TX (k MAX3485 DI) |
| PA3 | USART2_RX | RS485 RX (z MAX3485 RO) |
| PA1 | USART2_DE | RS485 DE/RE řízení |
| PA0 | TIM2_CH1 | WS2815 data (přes DMA) |
| PA4 | GPIO Input | Adresa bit 0 |
| PA12 | GPIO Input | Adresa bit 1 |
| PB0 | GPIO Input | Adresa bit 2 |
| PA11 | GPIO Input | Adresa bit 3 |

## Architektura software

### Struktura projektu

```
sw/
├── Core/                    # STM32 HAL a konfigurace
│   ├── Inc/
│   └── Src/
│       ├── main.c          # Hlavní smyčka, inicializace
│       ├── stm32g0xx_it.c  # Interrupt handlery
│       └── stm32g0xx_hal_msp.c
├── src/
│   ├── app.c               # Aplikační logika
│   └── app.h
├── sw/
│   ├── sw_ws2815.c/.h      # WS2815 LED driver
│   ├── sw_rs485.c/.h       # RS485 driver
│   └── sw_protocol.c/.h    # Komunikační protokol
└── sw_effects/
    ├── sw_effects.c/.h     # Vizuální efekty
    └── README.md
```

### Moduly

#### sw_ws2815 - WS2815 Driver

Ovládání adresovatelných RGB LED pomocí TIM3 + DMA.

**Časování WS2815:**
- Perioda: 1.25µs (800kHz)
- T0H: 375ns, T0L: 875ns
- T1H: 812.5ns, T1L: 437.5ns
- Reset: >280µs

**Hlavní funkce:**
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

Interrupt-based RS485 komunikace s ring bufferem.

**Vlastnosti:**
- RXNE interrupt pro příjem bytů
- IDLE line detection pro detekci konce rámce
- TXE/TC interrupts pro odesílání
- Hardware RS485 mód (automatické DE řízení)

**Hlavní funkce:**
```c
RS485_Status_t RS485_Init(void);
RS485_Status_t RS485_StartReceive(void);
RS485_Status_t RS485_TransmitIT(uint8_t *data, uint16_t length);
void RS485_SetFrameCallback(RS485_FrameCallback_t callback);
uint8_t RS485_Available(void);
uint8_t RS485_Read(void);
void RS485_IRQHandler(void);
```

#### sw_protocol - Komunikační protokol

Zpracování příkazů a odpovědí.

**Vlastnosti:**
- Zpracování rámce v IDLE přerušení (minimální latence)
- Odložená aktualizace displeje (DMA safe)
- CRC-8 validace

**Hlavní funkce:**
```c
void Protocol_Init(void);
void Protocol_Process(void);
void Protocol_HandleFrame(uint8_t *data, uint8_t length);
uint8_t Protocol_GetAddress(void);
```

#### sw_effects - Vizuální efekty

Knihovna předpřipravených animací.

**Dostupné efekty:**
- Rainbow - duhový cyklus
- Spinning - kroužící LED
- Breathing - dýchací efekt
- Segment Chase - běžící segmenty
- Knight Rider - K.I.T.T. efekt
- Loading Bar - progress bar
- Rainbow Digit - číslice s měnící se barvou
- Rainbow Gradient - číslice s duhovým gradientem

## Tok dat

### Příjem a zpracování příkazu

```
1. RXNE interrupt → byte do ring bufferu a frame bufferu
2. IDLE interrupt → Protocol_HandleFrame() volán
3. Rámec parsován a validován (CRC)
4. Příkaz zpracován, stav aktualizován
5. Odpověď odeslána (TX interrupt)
6. display_update_pending flag nastaven
7. Main loop → Protocol_Process() → UpdateDisplay()
8. WS2815_Update() → DMA transfer → LED aktualizovány
```

### Časování odpovědi

- Firmware zpracování: <100µs
- Celková odezva: ~1.5ms (limitováno USB převodníkem)

## Konfigurace

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
Period = 79 (80 cyklů = 1.25µs při 64MHz)
Channel 1 PWM mode
DMA Circular mode
```

### NVIC Priority

| Přerušení | Priorita | Popis |
|-----------|----------|-------|
| DMA1_Channel1 | 0 | WS2815 (nejvyšší) |
| USART2 | 1 | RS485 komunikace |
| SysTick | 3 | HAL timing |

## Paměťová mapa

| Oblast | Velikost | Použití |
|--------|----------|---------|
| Flash | 64 KB | Kód + konstanty |
| RAM | 8 KB | Proměnné + buffery |

### RAM využití

| Buffer | Velikost | Popis |
|--------|----------|-------|
| DMA buffer | ~2 KB | WS2815 data (29 LED × 24 bit + reset) |
| RX ring buffer | 64 B | RS485 příjem |
| TX buffer | 64 B | RS485 odesílání |
| Frame buffer | 64 B | Příchozí rámec |
| Response buffer | 13 B | Odpověď |

## Kompilace a programování

### Požadavky

- STM32CubeIDE nebo ARM GCC toolchain
- ST-Link programmer

### Kompilace

```bash
cd sw/Debug
make clean
make all
```

### Programování

```bash
st-flash write LED7SEG02.bin 0x08000000
```

Nebo pomocí STM32CubeIDE: Run → Debug/Run

## Řešení problémů

### LED nesvítí

1. Zkontrolujte 12V napájení
2. Ověřte DMA buffer (WS2815_IsBusy)
3. Zkontrolujte timing signálu osciloskopem

### RS485 nekomunikuje

1. Zkontrolujte baudrate (921600)
2. Ověřte zapojení A/B vodičů
3. Zkontrolujte adresu BCD přepínačů
4. Změřte DE pin aktivitu

### Pomalá odezva

1. Ověřte zpracování v IDLE přerušení
2. Zkontrolujte priority přerušení
3. USB převodník může být bottleneck

## Verze

| Verze | Datum | Změny |
|-------|-------|-------|
| 1.0 | 2026-02 | První verze |
