# LED7SEG02 - Dokumentace projektu

Dokumentace pro projekt LED7SEG02 - 7-segmentový displej s WS2815 adresovatelnými RGB LED a RS485 komunikací.

## Přehled projektu

Tento projekt implementuje ovládání 7-segmentového displeje složeného z 29 WS2815 RGB LED pomocí STM32G031F8Px mikrokontroléru. Displej je ovládán přes RS485 sběrnici pomocí definovaného protokolu.

### Základní specifikace

- **MCU:** STM32G031F8Px (Cortex-M0+ @ 64MHz)
- **LED:** 29× WS2815 RGB (7 segmentů × 4 LED + 1 desetinná tečka)
- **Komunikace:** RS485 @ 921600 baud
- **Napájení:** 3.3V (MCU), 12V (LEDs)

## Struktura dokumentace

### [WS2815_Driver.md](WS2815_Driver.md)
Kompletní dokumentace WS2815 driveru včetně:
- API reference
- Knihovna vizuálních efektů
- Příklady použití
- Troubleshooting

### [Hardware_Configuration.md](Hardware_Configuration.md)
Hardware konfigurace a specifikace:
- Pinout MCU
- Periférie nastavení
- Schéma zapojení

### [protocol_cz.md](protocol_cz.md)
RS485 komunikační protokol:
- Struktura rámce
- Seznam příkazů
- CRC-8 výpočet
- Příklady komunikace

### [firmware_cz.md](firmware_cz.md)
Dokumentace firmware:
- Architektura software
- Popis modulů
- Tok dat

## Quick Start

### 1. Kompilace projektu

Otevřít projekt v **STM32CubeIDE**:

```
File → Open Projects from File System
→ Import source: LED7SEG02\sw
```

Build projekt: `Project → Build Project` (Ctrl+B)

### 2. Základní použití

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

    // Zobrazení číslice
    WS2815_DisplayDigit(8, green, 1);
    WS2815_Update();
    while(WS2815_IsBusy());
}
```

### 3. Připojení hardware

```
PA0 (MCU) ──────► DI (WS2815)
GND (MCU) ──────► GND (WS2815)
12V PSU   ──────► +12V (WS2815)

PA2 (MCU) ──────► DI (MAX3485)
PA3 (MCU) ◄────── RO (MAX3485)
PA1 (MCU) ──────► DE/RE (MAX3485)
```

## Struktura projektu

```
LED7SEG02/
├── doc/                          # Dokumentace
│   ├── README.md                 # Tento soubor
│   ├── WS2815_Driver.md          # Driver dokumentace
│   ├── Hardware_Configuration.md # Hardware dokumentace
│   ├── protocol_cz.md            # Protokol (CZ)
│   ├── protocol_en.md            # Protokol (EN)
│   ├── firmware_cz.md            # Firmware (CZ)
│   └── firmware_en.md            # Firmware (EN)
├── hw/                           # Hardware design files
├── sw/                           # Firmware
│   ├── Core/                     # STM32 HAL core
│   ├── Drivers/                  # STM32 HAL drivers
│   ├── src/                      # Aplikační kód
│   │   ├── app.c
│   │   └── app.h
│   ├── sw/                       # Custom drivers
│   │   ├── sw_ws2815.c/.h        # WS2815 driver
│   │   ├── sw_rs485.c/.h         # RS485 driver
│   │   └── sw_protocol.c/.h      # Komunikační protokol
│   └── sw_effects/               # Vizuální efekty
│       ├── sw_effects.c/.h
│       └── README.md
└── app/                          # Python testovací aplikace
    ├── led7seg_test.py
    └── config.json
```

## Klíčové vlastnosti

### WS2815 Driver

- Hardware PWM + DMA (žádné bit-banging)
- 800 kHz bitrate
- Neblokující operace
- Nastavitelný jas (10-254)
- Přesné timing (TIM2 @ 64MHz)
- Nízká CPU zátěž

### Vizuální efekty

- Rainbow - duhový cyklus
- Spinning - kroužící LED
- Breathing - dýchací efekt
- Segment Chase - běžící segmenty
- Knight Rider - K.I.T.T. efekt
- Loading Bar - progress bar
- Rainbow Digit - číslice s měnící se barvou
- Rainbow Gradient - číslice s duhovým gradientem

### RS485 Komunikace

- 921600 baud, 8N1
- Hardware RS485 mód (automatické DE řízení)
- Interrupt-based příjem/odesílání
- IDLE line detection pro detekci konce rámce
- CRC-8 validace
- Odezva < 2ms

## Konfigurace periférií

| Periférie | Pin   | Funkce           |
|-----------|-------|------------------|
| TIM2_CH1  | PA0   | WS2815 Data      |
| USART2_TX | PA2   | RS485 TX         |
| USART2_RX | PA3   | RS485 RX         |
| USART2_DE | PA1   | RS485 Direction  |
| GPIO      | PA4   | Address bit 0    |
| GPIO      | PA12  | Address bit 1    |
| GPIO      | PB0   | Address bit 2    |
| GPIO      | PA11  | Address bit 3    |

## Mapování segmentů

```
    aaaa
   f    b
   f    b
    gggg
   e    c
   e    c
    dddd  dp
```

| Segment | Index | LED pozice |
|---------|-------|------------|
| a | 0 | 0-3 |
| b | 1 | 4-7 |
| c | 2 | 8-11 |
| d | 3 | 12-15 |
| e | 4 | 16-19 |
| f | 5 | 20-23 |
| g | 6 | 24-27 |
| dp | 7 | 28 |

## API Quick Reference

### Inicializace

```c
WS2815_Init();                      // Inicializace driveru
WS2815_SetBrightness(brightness);   // Nastavení jasu (10-254)
Effects_Init();                     // Inicializace efektů
```

### Nastavení displeje

```c
WS2815_DisplayDigit(digit, color, show_dp);  // Zobrazení číslice 0-9
WS2815_SetSegment(segment, color);           // Nastavení segmentu
WS2815_SetLED(index, color);                 // Nastavení LED
WS2815_Clear();                              // Vypnutí všech LED
```

### Aktualizace

```c
WS2815_Update();                    // Odeslání dat (neblokující)
WS2815_IsBusy();                    // Kontrola zaneprázdnění
```

### Efekty

```c
Effects_Rainbow(step);                          // Duhový efekt
Effects_RainbowDigit(digit, show_dp, step);     // Rainbow na číslici
Effects_Breathing(step, color);                 // Dýchání
Effects_KnightRider(pos, color);                // K.I.T.T.
```

## Troubleshooting

### LED se nerozsvěcují

1. Zkontrolovat 12V napájení
2. Zkontrolovat zapojení PA0 → DI
3. Zkontrolovat společnou zem
4. Ověřit DMA buffer (WS2815_IsBusy)

### RS485 nekomunikuje

1. Zkontrolovat baudrate (921600)
2. Ověřit zapojení A/B vodičů
3. Zkontrolovat adresu BCD přepínačů
4. Měřit DE pin aktivitu

### Špatné barvy

- Zkontrolovat brightness nastavení (rozsah 10-254)
- Zkontrolovat napájecí napětí (min 11V)

## Verze a změny

### v1.0 (2026-02)

- Implementace WS2815 driveru
- RS485 komunikace s protokolem
- Knihovna vizuálních efektů
- Dokumentace

## Licence a autor

- **Projekt:** LED7SEG02
- **Autor:** Richard Roztocil
- **Datum:** February 2026

## Reference

- [WS2815 Datasheet](https://cdn-shop.adafruit.com/product-files/2757/WS2815.pdf)
- [STM32G031 Datasheet](https://www.st.com/resource/en/datasheet/stm32g031f8.pdf)
- [MAX3485 Datasheet](https://www.maximintegrated.com/en/products/interface/transceivers/MAX3485.html)
