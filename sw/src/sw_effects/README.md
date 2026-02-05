# LED7SEG02 - Visual Effects Library

Knihovna vizuálních efektů pro 7-segmentový displej s WS2815 LED.

## Soubory
- `sw_effects.h` - Hlavičkový soubor s deklaracemi
- `sw_effects.c` - Implementace efektů

## Dostupné efekty

### 1. Rainbow (Duha)
Cyklicky mění barvy přes celé barevné spektrum.

```c
for (uint16_t step = 0; step < 256; step++) {
    Effects_Rainbow(step);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(10);
}
```

### 2. SpinningLED (Kroužící LED)
Jedna LED se pohybuje kolem dokola po obvodu (segmenty tvořící nulu) - ideální pro stand-by režim.

```c
WS2815_Color_t blue = {0, 0, 255};
for (uint8_t pos = 0; pos < 24; pos++) {  // 24 LEDs around perimeter
    Effects_SpinningLED(pos, blue);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(30);
}
```

### 3. Breathing (Dýchání)
Plynulé rozsvěcování a zhasínání - simuluje dýchání.

```c
WS2815_Color_t green = {0, 255, 0};
for (uint16_t step = 0; step < 256; step++) {
    Effects_Breathing(step, green);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(10);
}
```

### 4. SegmentChase (Běžící segment)
Postupně rozsvěcuje jednotlivé segmenty 7-segmentového displeje.

```c
WS2815_Color_t red = {255, 0, 0};
for (uint8_t step = 0; step < 7; step++) {
    Effects_SegmentChase(step, red);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(150);
}
```

### 5. KnightRider (K.I.T.T)
LED se pohybuje tam a zpět s ocasem - inspirováno seriálem Knight Rider.

```c
WS2815_Color_t red = {255, 0, 0};
for (uint8_t pos = 0; pos < 57; pos++) {
    Effects_KnightRider(pos, red);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(20);
}
```

### 6. LoadingBar (Načítací lišta)
Zobrazuje progress bar na segmentech (0-100%).

```c
WS2815_Color_t cyan = {0, 255, 255};
for (uint8_t progress = 0; progress <= 100; progress++) {
    Effects_LoadingBar(progress, cyan);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(50);
}
```

### 7. RainbowDigit (Duhová číslice)
**Neblokující** efekt - zobrazí číslici s barvou plynule se měnící přes celé spektrum.

```c
// Zobrazí číslo 8 s tečkou, cyklující přes všechny barvy
uint8_t step = 0;
while(1) {
    Effects_RainbowDigit(8, 1, step);  // digit=8, show_dp=1, step
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(20);
    step++;
}
```

## Pomocné funkce

### HSV to RGB konverze
Převádí barvy z HSV (Hue, Saturation, Value) do RGB.

```c
// Červená barva
WS2815_Color_t red = Effects_HSV_to_RGB(0, 255, 255);

// Zelená barva
WS2815_Color_t green = Effects_HSV_to_RGB(85, 255, 255);

// Modrá barva
WS2815_Color_t blue = Effects_HSV_to_RGB(170, 255, 255);
```

## Integrace do projektu

### 1. Přidání do projektu
V souboru `app.c` přidejte:
```c
#include "sw_effects.h"
```

### 2. Inicializace
```c
void APP_Init(){
    WS2815_Init();
    WS2815_SetBrightness(100);
    Effects_Init();
}
```

### 3. Použití v hlavní smyčce
```c
void APP_Main(){
    while(1){
        // Vaše efekty zde
        Demo_Rainbow();
        HAL_Delay(500);

        Demo_StandBy();
        HAL_Delay(500);
    }
}
```

## Demo režimy

V souboru `app.c` jsou připraveny demo funkce:

- `Demo_Rainbow()` - Duhový efekt
- `Demo_StandBy()` - Kroužící LED pro stand-by
- `Demo_Breathing()` - Dýchací efekt
- `Demo_KnightRider()` - Bouncing efekt
- `Demo_SegmentChase()` - Běžící segmenty
- `Demo_LoadingBar()` - Loading bar (0-100% a zpět)
- `Demo_DigitDisplay()` - Zobrazení číslic
- `Demo_RainbowDigit()` - Číslice 0-9 s duhovou animací
- `Demo_RainbowDigit_Continuous(digit, show_dp)` - Nekonečná duhová animace pro jedno číslo

## Tipy pro použití

### Nastavení rychlosti animace
Rychlost animace se řídí pomocí `HAL_Delay()`:
- Menší delay = rychlejší animace
- Větší delay = pomalejší animace

### Nekonečná smyčka pro stand-by
```c
WS2815_Color_t blue = {0, 0, 255};
uint8_t position = 0;
while(1){
    Effects_SpinningLED(position, blue);
    WS2815_Update();
    while(WS2815_IsBusy());
    HAL_Delay(30);

    position = (position + 1) % 24;  // 24 LEDs around perimeter
}
```

### Kombinace efektů
```c
// Přepínání mezi efekty každých 5 sekund
while(1){
    for(int i = 0; i < 500; i++) {
        Effects_Rainbow(i % 256);
        WS2815_Update();
        while(WS2815_IsBusy());
        HAL_Delay(10);
    }

    for(int i = 0; i < 166; i++) {
        Effects_Breathing(i % 256, {0, 255, 0});
        WS2815_Update();
        while(WS2815_IsBusy());
        HAL_Delay(30);
    }
}
```

## Pokročilé příklady

### Neblokující duhová animace na číslici
```c
void APP_Main(){
    uint8_t step = 0;

    while(1){
        // Zobrazí číslo 3 s tečkou, barva se plynule mění
        Effects_RainbowDigit(3, 1, step);
        WS2815_Update();
        while(WS2815_IsBusy());
        HAL_Delay(20);

        step++;  // Automatické přetečení 255 -> 0
    }
}
```

### Změna číslice s kontinuální duhovou animací
```c
void APP_Main(){
    uint8_t step = 0;
    uint8_t digit = 0;
    uint16_t counter = 0;

    while(1){
        Effects_RainbowDigit(digit, 1, step);
        WS2815_Update();
        while(WS2815_IsBusy());
        HAL_Delay(20);

        step++;
        counter++;

        // Změň číslici každých ~5 sekund (256 * 20ms)
        if (counter >= 256) {
            counter = 0;
            digit = (digit + 1) % 10;  // 0-9
        }
    }
}
```

## Poznámky

- Všechny efekty respektují nastavený jas pomocí `WS2815_SetBrightness()`
- Rozsah jasu: 10-254 (0 = vypnuto)
- Celkem 29 LED (28 pro segmenty, 1 pro tečku)
- Segmenty jsou mapovány 0-6 (a-g), tečka je 7
- **Effects_RainbowDigit je neblokující** - ideální pro použití v real-time aplikacích

## Autor
LED7SEG02 Project - 2026
