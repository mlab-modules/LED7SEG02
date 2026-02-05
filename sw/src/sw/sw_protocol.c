// sw_protocol.c - RS485 protocol for LED7SEG02

#include "sw_protocol.h"
#include "sw_rs485.h"
#include "sw_ws2815.h"
#include "sw_effects.h"
#include "main.h"
#include <string.h>

typedef enum {
    PARSE_IDLE,
    PARSE_WAIT_ADDR,
    PARSE_WAIT_CMD,
    PARSE_WAIT_DATA,
    PARSE_WAIT_CRC,
    PARSE_WAIT_END
} ParserState_t;

static uint8_t device_address = 0x01;
static ParserState_t parser_state = PARSE_IDLE;
static Protocol_Frame_t rx_frame;
static uint8_t rx_data_count = 0;
static uint32_t frame_start_time = 0;

static Protocol_DisplayState_t display_state = {
    .digit = 0xFF,
    .show_dp = 0,
    .r = 255, .g = 255, .b = 255,
    .brightness = 50,
    .effect = EFFECT_STOP,
    .effect_speed = 2
};

static uint16_t effect_step = 0;
static uint32_t last_effect_time = 0;
static uint8_t response_buffer[PROTO_FRAME_MAX_LEN];
static volatile uint8_t display_update_pending = 0;

static void ProcessFrame(Protocol_Frame_t *frame);
static void SendResponse(uint8_t status, uint8_t *data, uint8_t data_len);
static int8_t GetExpectedDataLength(uint8_t cmd);
static void RunEffect(void);
static void UpdateDisplay(void);

uint8_t Protocol_CalcCRC8(uint8_t *data, uint8_t length) {
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

uint8_t Protocol_ReadAddressFromGPIO(void) {
    uint8_t addr = 0;
    if (HAL_GPIO_ReadPin(addr_0_GPIO_Port, addr_0_Pin) == GPIO_PIN_RESET) addr |= 0x01;
    if (HAL_GPIO_ReadPin(addr_1_GPIO_Port, addr_1_Pin) == GPIO_PIN_RESET) addr |= 0x02;
    if (HAL_GPIO_ReadPin(addr_2_GPIO_Port, addr_2_Pin) == GPIO_PIN_RESET) addr |= 0x04;
    if (HAL_GPIO_ReadPin(addr_3_GPIO_Port, addr_3_Pin) == GPIO_PIN_RESET) addr |= 0x08;
    return addr;
}

static int8_t GetExpectedDataLength(uint8_t cmd) {
    switch (cmd) {
        case CMD_SET_DIGIT:      return 2;
        case CMD_SET_COLOR:      return 3;
        case CMD_SET_BRIGHTNESS: return 1;
        case CMD_SET_EFFECT:     return 2;
        case CMD_SET_SEGMENT:    return 4;
        case CMD_SET_LED:        return 4;
        case CMD_CLEAR:          return 0;
        case CMD_UPDATE:         return 0;
        case CMD_GET_STATUS:     return 0;
        case CMD_GET_VERSION:    return 0;
        case CMD_GET_ADDRESS:    return 0;
        case CMD_PING:           return 0;
        default:                 return -1;
    }
}

void Protocol_Init(void) {
    device_address = Protocol_ReadAddressFromGPIO();
    parser_state = PARSE_IDLE;
    rx_data_count = 0;
    frame_start_time = 0;
    effect_step = 0;
    last_effect_time = 0;
    display_update_pending = 0;

    display_state.digit = 0xFF;
    display_state.show_dp = 0;
    display_state.r = 255;
    display_state.g = 255;
    display_state.b = 255;
    display_state.brightness = 50;
    display_state.effect = EFFECT_STOP;
    display_state.effect_speed = 2;

    WS2815_SetBrightness(display_state.brightness);

    // Register frame callback for immediate processing in IDLE interrupt
    RS485_SetFrameCallback(Protocol_HandleFrame);
}

void Protocol_Process(void) {
    // Handle pending display update (deferred from ISR)
    if (display_update_pending) {
        display_update_pending = 0;
        if (display_state.digit <= 9) {
            UpdateDisplay();
        } else {
            WS2815_Update();
        }
    }

    // Run effects
    RunEffect();
}

// Called from IDLE interrupt - parse and process frame immediately
void Protocol_HandleFrame(uint8_t *data, uint8_t length) {
    if (length < PROTO_FRAME_MIN_LEN) return;
    if (data[0] != PROTO_START_BYTE) return;
    if (data[length-1] != PROTO_END_BYTE) return;

    uint8_t addr = data[1];
    uint8_t cmd = data[2];
    int8_t expected_data_len = GetExpectedDataLength(cmd);
    if (expected_data_len < 0) expected_data_len = 0;

    uint8_t frame_len = PROTO_FRAME_MIN_LEN + expected_data_len;
    if (length < frame_len) return;

    // Verify CRC
    uint8_t crc_calc = Protocol_CalcCRC8(&data[1], 2 + expected_data_len);
    if (crc_calc != data[frame_len - 2]) {
        if (addr == device_address) {
            rx_frame.address = addr;
            SendResponse(STATUS_CRC_ERROR, NULL, 0);
        }
        return;
    }

    // Check if addressed to us
    if (addr != device_address && addr != PROTO_BROADCAST_ADDR) return;

    // Build frame structure
    rx_frame.address = addr;
    rx_frame.command = cmd;
    rx_frame.data_length = expected_data_len;
    if (expected_data_len > 0) {
        memcpy(rx_frame.data, &data[3], expected_data_len);
    }

    // Process frame immediately
    ProcessFrame(&rx_frame);
}

static void SendResponse(uint8_t status, uint8_t *data, uint8_t data_len) {
    if (rx_frame.address == PROTO_BROADCAST_ADDR) return;

    uint8_t idx = 0;
    response_buffer[idx++] = PROTO_START_BYTE;
    response_buffer[idx++] = device_address;
    response_buffer[idx++] = status;

    if (data != NULL && data_len > 0) {
        memcpy(&response_buffer[idx], data, data_len);
        idx += data_len;
    }

    uint8_t crc_data[2 + PROTO_MAX_DATA_LEN];
    crc_data[0] = device_address;
    crc_data[1] = status;
    if (data != NULL && data_len > 0) {
        memcpy(&crc_data[2], data, data_len);
    }
    response_buffer[idx++] = Protocol_CalcCRC8(crc_data, 2 + data_len);
    response_buffer[idx++] = PROTO_END_BYTE;

    RS485_TransmitIT(response_buffer, idx);
}

static void ProcessFrame(Protocol_Frame_t *frame) {
    uint8_t resp_data[8];
    uint8_t resp_len = 0;

    switch (frame->command) {
        case CMD_SET_DIGIT:
            if (frame->data_length >= 2) {
                display_state.digit = frame->data[0];
                display_state.show_dp = frame->data[1];
                display_state.effect = EFFECT_STOP;
                SendResponse(STATUS_OK, NULL, 0);
                display_update_pending = 1;
            } else {
                SendResponse(STATUS_INVALID_PARAM, NULL, 0);
            }
            break;

        case CMD_SET_COLOR:
            if (frame->data_length >= 3) {
                display_state.r = frame->data[0];
                display_state.g = frame->data[1];
                display_state.b = frame->data[2];
                SendResponse(STATUS_OK, NULL, 0);
                if (display_state.effect == EFFECT_STOP) {
                    display_update_pending = 1;
                }
            } else {
                SendResponse(STATUS_INVALID_PARAM, NULL, 0);
            }
            break;

        case CMD_SET_BRIGHTNESS:
            if (frame->data_length >= 1) {
                display_state.brightness = frame->data[0];
                WS2815_SetBrightness(display_state.brightness);
                SendResponse(STATUS_OK, NULL, 0);
                if (display_state.effect == EFFECT_STOP) {
                    display_update_pending = 1;
                }
            } else {
                SendResponse(STATUS_INVALID_PARAM, NULL, 0);
            }
            break;

        case CMD_SET_EFFECT:
            if (frame->data_length >= 2) {
                display_state.effect = (Protocol_Effect_t)frame->data[0];
                display_state.effect_speed = frame->data[1];
                effect_step = 0;
                last_effect_time = HAL_GetTick();
                SendResponse(STATUS_OK, NULL, 0);
            } else {
                SendResponse(STATUS_INVALID_PARAM, NULL, 0);
            }
            break;

        case CMD_SET_SEGMENT:
            if (frame->data_length >= 4) {
                WS2815_Color_t color = {frame->data[1], frame->data[2], frame->data[3]};
                if (WS2815_SetSegment(frame->data[0], color) == 0) {
                    SendResponse(STATUS_OK, NULL, 0);
                } else {
                    SendResponse(STATUS_INVALID_PARAM, NULL, 0);
                }
            } else {
                SendResponse(STATUS_INVALID_PARAM, NULL, 0);
            }
            break;

        case CMD_SET_LED:
            if (frame->data_length >= 4) {
                WS2815_Color_t color = {frame->data[1], frame->data[2], frame->data[3]};
                if (WS2815_SetLED(frame->data[0], color) == 0) {
                    SendResponse(STATUS_OK, NULL, 0);
                } else {
                    SendResponse(STATUS_INVALID_PARAM, NULL, 0);
                }
            } else {
                SendResponse(STATUS_INVALID_PARAM, NULL, 0);
            }
            break;

        case CMD_CLEAR:
            display_state.digit = 0xFF;
            display_state.effect = EFFECT_STOP;
            WS2815_Clear();
            SendResponse(STATUS_OK, NULL, 0);
            display_update_pending = 1;
            break;

        case CMD_UPDATE:
            SendResponse(STATUS_OK, NULL, 0);
            display_update_pending = 1;
            break;

        case CMD_GET_STATUS:
            resp_data[0] = display_state.digit;
            resp_data[1] = display_state.show_dp;
            resp_data[2] = display_state.r;
            resp_data[3] = display_state.g;
            resp_data[4] = display_state.b;
            resp_data[5] = display_state.brightness;
            resp_data[6] = display_state.effect;
            resp_data[7] = display_state.effect_speed;
            resp_len = 8;
            SendResponse(STATUS_OK, resp_data, resp_len);
            break;

        case CMD_GET_VERSION:
            resp_data[0] = PROTO_VERSION_MAJOR;
            resp_data[1] = PROTO_VERSION_MINOR;
            resp_len = 2;
            SendResponse(STATUS_OK, resp_data, resp_len);
            break;

        case CMD_GET_ADDRESS:
            resp_data[0] = device_address;
            resp_len = 1;
            SendResponse(STATUS_OK, resp_data, resp_len);
            break;

        case CMD_PING:
            SendResponse(STATUS_OK, NULL, 0);
            break;

        default:
            SendResponse(STATUS_UNKNOWN_CMD, NULL, 0);
            break;
    }
}

static void UpdateDisplay(void) {
    if (display_state.digit <= 9) {
        WS2815_Color_t color = {display_state.r, display_state.g, display_state.b};
        WS2815_DisplayDigit(display_state.digit, color, display_state.show_dp);
        WS2815_Update();
    }
}

static void RunEffect(void) {
    if (display_state.effect == EFFECT_STOP) return;

    uint32_t current_time = HAL_GetTick();
    uint32_t delay = display_state.effect_speed * 10;
    if (delay == 0) delay = 10;

    if ((current_time - last_effect_time) < delay) return;
    last_effect_time = current_time;

    WS2815_Color_t color = {display_state.r, display_state.g, display_state.b};

    switch (display_state.effect) {
        case EFFECT_RAINBOW:
            Effects_Rainbow(effect_step);
            break;
        case EFFECT_SPINNING:
            Effects_SpinningLED(effect_step % 24, color);
            break;
        case EFFECT_BREATHING:
            Effects_Breathing(effect_step, color);
            break;
        case EFFECT_SEGMENT_CHASE:
            Effects_SegmentChase(effect_step % 7, color);
            break;
        case EFFECT_KNIGHT_RIDER:
            Effects_KnightRider(effect_step % 57, color);
            break;
        case EFFECT_LOADING_BAR:
            Effects_LoadingBar(effect_step % 101, color);
            break;
        case EFFECT_RAINBOW_DIGIT:
            if (display_state.digit <= 9) {
                Effects_RainbowDigit(display_state.digit, display_state.show_dp, effect_step);
            }
            break;
        case EFFECT_RAINBOW_GRADIENT:
            if (display_state.digit <= 9) {
                Effects_RainbowGradientDigit(display_state.digit, display_state.show_dp, effect_step);
            }
            break;
        default:
            return;
    }

    WS2815_Update();
    effect_step++;
}

uint8_t Protocol_GetAddress(void) {
    return device_address;
}

const Protocol_DisplayState_t* Protocol_GetState(void) {
    return &display_state;
}
