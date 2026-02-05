# LED7SEG02 - Komunikační protokol RS485

## Přehled

Protokol pro ovládání 7-segmentového displeje s WS2815 LED přes RS485 sběrnici.

- **Fyzická vrstva**: RS485 half-duplex
- **Rychlost**: 921600 baud (konfigurovatelná)
- **Formát**: 8N1 (8 datových bitů, bez parity, 1 stop bit)
- **Transceiver**: MAX3485 s automatickým řízením DE pinu

## Struktura rámce

```
[START][ADDR][CMD][DATA 0-8B][CRC8][END]
```

| Pole | Velikost | Hodnota | Popis |
|------|----------|---------|-------|
| START | 1 byte | 0xAA | Začátek rámce |
| ADDR | 1 byte | 0x00-0x0F | Adresa zařízení |
| CMD | 1 byte | viz tabulka | Příkaz |
| DATA | 0-8 bytů | - | Data příkazu |
| CRC8 | 1 byte | - | Kontrolní součet |
| END | 1 byte | 0x55 | Konec rámce |

## Adresování

- **0x00**: Broadcast (všechna zařízení, bez odpovědi)
- **0x01-0x0F**: Unicast (konkrétní zařízení s odpovědí)

Adresa zařízení je nastavena pomocí BCD přepínačů na pinech:
- PA4 = bit 0 (váha 1)
- PA12 = bit 1 (váha 2)
- PB0 = bit 2 (váha 4)
- PA11 = bit 3 (váha 8)

Přepínače jsou aktivní v LOW (sepnutý = 0).

## Příkazy

### Ovládání displeje

| Příkaz | Kód | Data | Popis |
|--------|-----|------|-------|
| SET_DIGIT | 0x01 | [digit][dp] | Zobrazí číslici 0-9, dp=desetinná tečka |
| SET_COLOR | 0x02 | [R][G][B] | Nastaví barvu RGB |
| SET_BRIGHTNESS | 0x03 | [brightness] | Nastaví jas 0-255 |
| SET_EFFECT | 0x04 | [effect][speed] | Spustí efekt |
| SET_SEGMENT | 0x05 | [seg][R][G][B] | Nastaví barvu segmentu 0-7 |
| SET_LED | 0x06 | [idx][R][G][B] | Nastaví barvu LED 0-28 |
| CLEAR | 0x10 | - | Vypne displej |
| UPDATE | 0x11 | - | Vynutí aktualizaci LED |

### Dotazy

| Příkaz | Kód | Odpověď | Popis |
|--------|-----|---------|-------|
| GET_STATUS | 0xF0 | 8 bytů stavu | Vrátí aktuální stav |
| GET_VERSION | 0xF1 | [major][minor] | Vrátí verzi FW |
| GET_ADDRESS | 0xF2 | [addr] | Vrátí adresu zařízení |
| PING | 0xFF | - | Test komunikace |

## Efekty

| Efekt | Kód | Popis |
|-------|-----|-------|
| STOP | 0x00 | Zastaví efekt |
| RAINBOW | 0x01 | Duhový cyklus |
| SPINNING | 0x02 | Kroužící LED |
| BREATHING | 0x03 | Dýchací efekt |
| SEGMENT_CHASE | 0x04 | Běžící segmenty |
| KNIGHT_RIDER | 0x05 | K.I.T.T. efekt |
| LOADING_BAR | 0x06 | Progress bar |
| RAINBOW_DIGIT | 0x07 | Číslice s měnící se barvou |
| RAINBOW_GRADIENT | 0x08 | Číslice s rainbow gradientem |

## Stavové kódy odpovědi

| Status | Kód | Popis |
|--------|-----|-------|
| OK | 0x00 | Příkaz úspěšně proveden |
| UNKNOWN_CMD | 0x01 | Neznámý příkaz |
| INVALID_PARAM | 0x02 | Neplatný parametr |
| BUSY | 0x03 | Zařízení zaneprázdněno |
| CRC_ERROR | 0x04 | Chyba CRC |

## Výpočet CRC-8

Polynom: **0x07** (x^8 + x^2 + x + 1)

CRC se počítá z polí ADDR + CMD + DATA.

### Algoritmus (C)

```c
uint8_t crc8(uint8_t *data, uint8_t length) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
```

### Algoritmus (Python)

```python
def crc8(data: bytes) -> int:
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x07
            else:
                crc <<= 1
            crc &= 0xFF
    return crc
```

### Příklad výpočtu

Příkaz PING na adresu 0x01:
```
Data pro CRC: [0x01, 0xFF]  (ADDR=0x01, CMD=0xFF)
CRC = 0xD3

Kompletní rámec: AA 01 FF D3 55
```

## Příklady komunikace

### PING

```
TX: AA 01 FF D3 55
RX: AA 01 00 3E 55
```

### SET_DIGIT (zobrazit "5" s tečkou)

```
TX: AA 01 01 05 01 XX 55   (XX = CRC)
RX: AA 01 00 XX 55
```

### SET_COLOR (zelená)

```
TX: AA 01 02 00 FF 00 XX 55
RX: AA 01 00 XX 55
```

### Broadcast SET_BRIGHTNESS

```
TX: AA 00 03 32 XX 55      (adresa 0x00 = broadcast)
(žádná odpověď)
```

## Časování

- **Timeout rámce**: 100ms (pokud rámec není kompletní)
- **Typická odezva**: <2ms (závisí na USB převodníku)
- **Minimální mezera mezi příkazy**: doporučeno čekat na odpověď

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
