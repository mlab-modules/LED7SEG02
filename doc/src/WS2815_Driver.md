# WS2815 LED Driver - Dokumentace

## Přehled

Driver pro ovládání adresovatelných RGB LED WS2815 na platformě STM32G031F8Px. Driver využívá TIM2 + DMA pro generování přesného PWM signálu kompatibilního s WS2815 protokolem.

## Specifikace

- **MCU:** STM32G031F8Px (Cortex-M0+)
- **Systémový takt:** 64 MHz
- **LED typ:** WS2815 RGB addressable LEDs
- **Komunikace:** 1-wire, 800 kHz bitrate
- **Počet LED:** 29 (7 segmentů × 4 LED + 1 desetinná tečka)
- **Barevný formát:** GRB (Green-Red-Blue)

## Hardware konfigurace

### Připojení

| Signál | Pin MCU | Periférie | Popis |
|--------|---------|-----------|-------|
| Data   | PA0     | TIM2_CH1  | Data signál pro WS2815 |
| VCC    | -       | -         | 12V napájení pro LED |
| GND    | GND     | -         | Společná zem |

### Periférie

- **TIM2:** Timer pro PWM generování
  - Channel 1 (PA0)
  - Frekvence: 800 kHz
  - Prescaler: 0
  - Period: 79 (@ 64MHz = 1.25µs)

- **DMA1 Channel 1:** Přenos PWM dat
  - Request: TIM2_CH1
  - Direction: Memory-to-Peripheral
  - Mode: Normal
  - Priority: High (0)

## WS2815 Timing

WS2815 používá následující časování:

| Bit | T0H    | T0L     | T1H     | T1L    | Celkem |
|-----|--------|---------|---------|--------|--------|
| 0   | 375ns  | 875ns   | -       | -      | 1.25µs |
| 1   | -      | -       | 812.5ns | 437.5ns| 1.25µs |
| RES | -      | >280µs  | -       | -      | -      |

### Implementace v driveru

```c
#define WS2815_TIM_PERIOD   80    // 1.25µs @ 64MHz
#define WS2815_BIT_0_DUTY   24    // 375ns (30% duty)
#define WS2815_BIT_1_DUTY   52    // 812.5ns (65% duty)
#define WS2815_RESET_SLOTS  250   // >280µs reset perioda
```

## Struktura souborů

```
LED7SEG02/sw/
├── sw/
│   ├── sw_ws2815.c       # Driver implementace
│   └── sw_ws2815.h       # Public API
├── sw_effects/
│   ├── sw_effects.c      # Knihovna efektů
│   └── sw_effects.h
├── Core/
│   └── Src/
│       └── stm32g0xx_it.c  # DMA interrupt handler
└── src/
    └── app.c             # Aplikační kód
```

## API Reference

### Datové typy

```c
typedef struct {
    uint8_t r;  // Red (0-255)
    uint8_t g;  // Green (0-255)
    uint8_t b;  // Blue (0-255)
} WS2815_Color_t;
```

### Funkce

#### WS2815_Init()

```c
void WS2815_Init(void);
```

Inicializuje WS2815 driver (GPIO, TIM2, DMA).

---

#### WS2815_SetLED()

```c
void WS2815_SetLED(uint8_t index, WS2815_Color_t color);
```

Nastaví barvu jedné LED v bufferu.

**Parametry:**
- `index`: Index LED (0-28)
- `color`: RGB barva

**Poznámka:** Změna se projeví až po volání `WS2815_Update()`

---

#### WS2815_SetSegment()

```c
void WS2815_SetSegment(uint8_t segment, WS2815_Color_t color);
```

Nastaví barvu celého segmentu (4 LED).

**Parametry:**
- `segment`: Index segmentu (0-7)
- `color`: RGB barva

---

#### WS2815_DisplayDigit()

```c
void WS2815_DisplayDigit(uint8_t digit, WS2815_Color_t color, uint8_t show_dp);
```

Zobrazí číslici 0-9 na displeji.

**Parametry:**
- `digit`: Číslice (0-9)
- `color`: RGB barva
- `show_dp`: 1 = zobrazit desetinnou tečku

---

#### WS2815_Update()

```c
void WS2815_Update(void);
```

Odešle buffer do LED pásku pomocí DMA.

**Poznámka:** Funkce je neblokující, používá DMA transfer

---

#### WS2815_Clear()

```c
void WS2815_Clear(void);
```

Vypne všechny LED (nastaví na černou).

---

#### WS2815_SetBrightness()

```c
void WS2815_SetBrightness(uint8_t brightness);
```

Nastaví globální jas všech LED.

**Parametry:**
- `brightness`: 0 (vypnuto), 10-254 (funkční rozsah)

**Poznámky:**
- Hodnoty 1-9 se automaticky nastaví na 10
- Hodnoty > 254 se automaticky nastaví na 254

---

#### WS2815_IsBusy()

```c
uint8_t WS2815_IsBusy(void);
```

Zkontroluje, zda probíhá DMA transfer.

**Return:** 1 pokud je zaneprázdněn, 0 pokud je volný

## Knihovna efektů (sw_effects)

### Dostupné efekty

| Funkce | Popis |
|--------|-------|
| `Effects_Rainbow(step)` | Duhový cyklus na všech LED |
| `Effects_SpinningLED(pos, color)` | Kroužící LED (stand-by) |
| `Effects_Breathing(step, color)` | Dýchací efekt |
| `Effects_SegmentChase(step, color)` | Běžící segmenty |
| `Effects_KnightRider(pos, color)` | K.I.T.T. efekt s ocasem |
| `Effects_LoadingBar(progress, color)` | Progress bar |
| `Effects_RainbowDigit(digit, show_dp, step)` | Číslice s měnící se rainbow barvou |
| `Effects_RainbowGradientDigit(digit, show_dp, step)` | Číslice s rainbow gradientem |

### Pomocné funkce

```c
WS2815_Color_t Effects_HSV_to_RGB(uint8_t h, uint8_t s, uint8_t v);
```

### Rozdíl mezi rainbow efekty

| Efekt | Popis |
|-------|-------|
| `Effects_RainbowDigit` | Celá číslice svítí jednou barvou, barva se mění v čase |
| `Effects_RainbowGradientDigit` | Každá LED má jinou barvu z duhy (gradient), gradient se pohybuje |

## Použití

### Příklad 1: Základní použití

```c
#include "sw_ws2815.h"

void app_main(void) {
    WS2815_Init();
    WS2815_SetBrightness(50);

    WS2815_Color_t green = {0, 255, 0};

    // Zobrazení číslice "8" s desetinnou tečkou
    WS2815_DisplayDigit(8, green, 1);
    WS2815_Update();
    while(WS2815_IsBusy());
}
```

### Příklad 2: Rainbow animace

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

### Příklad 3: Rainbow gradient na číslici

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

## Mapování segmentů

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

## DMA Buffer struktura

```
DMA buffer obsahuje:
- 29 LED × 24 bitů = 696 slotů pro data
- 250 slotů pro reset periodu (>280µs LOW)
- Celkem: ~1940 bytů
```

## Troubleshooting

### LED se nerozsvěcují

1. **Zkontrolovat napájení:** WS2815 potřebuje 12V
2. **Zkontrolovat zapojení:** Data pin na PA0
3. **Zkontrolovat system clock:** Musí být 64 MHz
4. **Zkontrolovat HAL_TIM_MODULE_ENABLED** v `stm32g0xx_hal_conf.h`

### Špatné barvy

- WS2815 používá GRB pořadí (driver to už řeší)
- Zkontrolovat brightness nastavení (rozsah 10-254)
- Zkontrolovat napájecí napětí (při nízkém napětí špatné barvy)

### Blikání při změně dat

- Příčina: Chyběla explicitní reset perioda v DMA bufferu
- Řešení: Driver obsahuje 250 reset slotů na konci DMA bufferu
- WS2815 potřebuje >280µs LOW signál pro latch dat do LED

### DMA chyby

- Zkontrolovat, že DMA1_Channel1 není použit jinde
- Zkontrolovat interrupt prioritu (0 = nejvyšší)

## Výkonnost

### Timing

- Refresh rate: ~275 Hz pro 29 LED
- Transfer time: ~0.9 ms pro 29 LED (29 × 24 bits × 1.25µs + reset)
- CPU usage: Minimální (používá DMA)

### Paměť

- RAM: ~2 KB (led_buffer + dma_buffer)
- Flash: ~1.5 KB (driver + effects code)

## Reference

- [WS2815 Datasheet](https://cdn-shop.adafruit.com/product-files/2757/WS2815.pdf)
- [STM32G0 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x0-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
