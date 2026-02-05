# LED7SEG02 - RS485 Communication Protocol

## Overview

Protocol for controlling 7-segment display with WS2815 LEDs over RS485 bus.

- **Physical layer**: RS485 half-duplex
- **Baud rate**: 921600 (configurable)
- **Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Transceiver**: MAX3485 with automatic DE pin control

## Frame Structure

```
[START][ADDR][CMD][DATA 0-8B][CRC8][END]
```

| Field | Size | Value | Description |
|-------|------|-------|-------------|
| START | 1 byte | 0xAA | Frame start marker |
| ADDR | 1 byte | 0x00-0x0F | Device address |
| CMD | 1 byte | see table | Command |
| DATA | 0-8 bytes | - | Command data |
| CRC8 | 1 byte | - | Checksum |
| END | 1 byte | 0x55 | Frame end marker |

## Addressing

- **0x00**: Broadcast (all devices, no response)
- **0x01-0x0F**: Unicast (specific device with response)

Device address is set using BCD switches on pins:
- PA4 = bit 0 (weight 1)
- PA12 = bit 1 (weight 2)
- PB0 = bit 2 (weight 4)
- PA11 = bit 3 (weight 8)

Switches are active LOW (closed = 0).

## Commands

### Display Control

| Command | Code | Data | Description |
|---------|------|------|-------------|
| SET_DIGIT | 0x01 | [digit][dp] | Display digit 0-9, dp=decimal point |
| SET_COLOR | 0x02 | [R][G][B] | Set RGB color |
| SET_BRIGHTNESS | 0x03 | [brightness] | Set brightness 0-255 |
| SET_EFFECT | 0x04 | [effect][speed] | Start effect |
| SET_SEGMENT | 0x05 | [seg][R][G][B] | Set segment 0-7 color |
| SET_LED | 0x06 | [idx][R][G][B] | Set LED 0-28 color |
| CLEAR | 0x10 | - | Turn off display |
| UPDATE | 0x11 | - | Force LED update |

### Queries

| Command | Code | Response | Description |
|---------|------|----------|-------------|
| GET_STATUS | 0xF0 | 8 bytes status | Returns current state |
| GET_VERSION | 0xF1 | [major][minor] | Returns FW version |
| GET_ADDRESS | 0xF2 | [addr] | Returns device address |
| PING | 0xFF | - | Communication test |

## Effects

| Effect | Code | Description |
|--------|------|-------------|
| STOP | 0x00 | Stop effect |
| RAINBOW | 0x01 | Rainbow cycle |
| SPINNING | 0x02 | Spinning LED |
| BREATHING | 0x03 | Breathing effect |
| SEGMENT_CHASE | 0x04 | Running segments |
| KNIGHT_RIDER | 0x05 | K.I.T.T. effect |
| LOADING_BAR | 0x06 | Progress bar |
| RAINBOW_DIGIT | 0x07 | Digit with changing color |
| RAINBOW_GRADIENT | 0x08 | Digit with rainbow gradient |

## Response Status Codes

| Status | Code | Description |
|--------|------|-------------|
| OK | 0x00 | Command executed successfully |
| UNKNOWN_CMD | 0x01 | Unknown command |
| INVALID_PARAM | 0x02 | Invalid parameter |
| BUSY | 0x03 | Device busy |
| CRC_ERROR | 0x04 | CRC error |

## CRC-8 Calculation

Polynomial: **0x07** (x^8 + x^2 + x + 1)

CRC is calculated over ADDR + CMD + DATA fields.

### Algorithm (C)

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

### Algorithm (Python)

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

### Calculation Example

PING command to address 0x01:
```
Data for CRC: [0x01, 0xFF]  (ADDR=0x01, CMD=0xFF)
CRC = 0xD3

Complete frame: AA 01 FF D3 55
```

## Communication Examples

### PING

```
TX: AA 01 FF D3 55
RX: AA 01 00 3E 55
```

### SET_DIGIT (display "5" with decimal point)

```
TX: AA 01 01 05 01 XX 55   (XX = CRC)
RX: AA 01 00 XX 55
```

### SET_COLOR (green)

```
TX: AA 01 02 00 FF 00 XX 55
RX: AA 01 00 XX 55
```

### Broadcast SET_BRIGHTNESS

```
TX: AA 00 03 32 XX 55      (address 0x00 = broadcast)
(no response)
```

## Timing

- **Frame timeout**: 100ms (if frame is incomplete)
- **Typical response time**: <2ms (depends on USB converter)
- **Minimum gap between commands**: recommended to wait for response

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
