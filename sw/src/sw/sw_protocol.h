// sw_protocol.h - RS485 protocol for LED7SEG02
// Frame: [0xAA][ADDR][CMD][DATA 0-8B][CRC8][0x55]
// Address from BCD switches: PA4(b0), PA12(b1), PB0(b2), PA11(b3)
// Addr 0 = broadcast only, Addr 1-15 = unicast + broadcast

#ifndef SW_PROTOCOL_H
#define SW_PROTOCOL_H

#include <stdint.h>

#define PROTO_START_BYTE        0xAA
#define PROTO_END_BYTE          0x55
#define PROTO_MAX_DATA_LEN      8
#define PROTO_FRAME_MIN_LEN     5
#define PROTO_FRAME_MAX_LEN     (PROTO_FRAME_MIN_LEN + PROTO_MAX_DATA_LEN)
#define PROTO_BROADCAST_ADDR    0x00
#define PROTO_FRAME_TIMEOUT     100
#define PROTO_VERSION_MAJOR     1
#define PROTO_VERSION_MINOR     0

typedef enum {
    CMD_SET_DIGIT       = 0x01,
    CMD_SET_COLOR       = 0x02,
    CMD_SET_BRIGHTNESS  = 0x03,
    CMD_SET_EFFECT      = 0x04,
    CMD_SET_SEGMENT     = 0x05,
    CMD_SET_LED         = 0x06,
    CMD_CLEAR           = 0x10,
    CMD_UPDATE          = 0x11,
    CMD_GET_STATUS      = 0xF0,
    CMD_GET_VERSION     = 0xF1,
    CMD_GET_ADDRESS     = 0xF2,
    CMD_PING            = 0xFF
} Protocol_Command_t;

typedef enum {
    EFFECT_STOP             = 0x00,
    EFFECT_RAINBOW          = 0x01,
    EFFECT_SPINNING         = 0x02,
    EFFECT_BREATHING        = 0x03,
    EFFECT_SEGMENT_CHASE    = 0x04,
    EFFECT_KNIGHT_RIDER     = 0x05,
    EFFECT_LOADING_BAR      = 0x06,
    EFFECT_RAINBOW_DIGIT    = 0x07,
    EFFECT_RAINBOW_GRADIENT = 0x08
} Protocol_Effect_t;

typedef enum {
    STATUS_OK               = 0x00,
    STATUS_UNKNOWN_CMD      = 0x01,
    STATUS_INVALID_PARAM    = 0x02,
    STATUS_BUSY             = 0x03,
    STATUS_CRC_ERROR        = 0x04
} Protocol_Status_t;

typedef struct {
    uint8_t address;
    uint8_t command;
    uint8_t data[PROTO_MAX_DATA_LEN];
    uint8_t data_length;
} Protocol_Frame_t;

typedef struct {
    uint8_t digit;
    uint8_t show_dp;
    uint8_t r, g, b;
    uint8_t brightness;
    Protocol_Effect_t effect;
    uint8_t effect_speed;
} Protocol_DisplayState_t;

void Protocol_Init(void);
void Protocol_Process(void);
void Protocol_HandleFrame(uint8_t *data, uint8_t length);
uint8_t Protocol_ReadAddressFromGPIO(void);
uint8_t Protocol_GetAddress(void);
const Protocol_DisplayState_t* Protocol_GetState(void);
uint8_t Protocol_CalcCRC8(uint8_t *data, uint8_t length);

#endif
