// sw_rs485.h - RS485 driver for STM32G031F8PX (USART2 + MAX3485)

#ifndef SW_RS485_H
#define SW_RS485_H

#include <stdint.h>
#include "stm32g0xx_hal.h"

extern UART_HandleTypeDef huart2;

#define RS485_RX_BUFFER_SIZE    64
#define RS485_TX_BUFFER_SIZE    64

typedef enum {
    RS485_OK = 0,
    RS485_ERROR = -1,
    RS485_TIMEOUT = -2,
    RS485_BUSY = -3
} RS485_Status_t;

typedef void (*RS485_FrameCallback_t)(uint8_t *data, uint8_t length);

RS485_Status_t RS485_Init(void);
RS485_Status_t RS485_StartReceive(void);
RS485_Status_t RS485_StopReceive(void);
RS485_Status_t RS485_TransmitIT(uint8_t *data, uint16_t length);
RS485_Status_t RS485_Transmit(uint8_t *data, uint16_t length, uint32_t timeout);
RS485_Status_t RS485_Receive(uint8_t *data, uint16_t length, uint32_t timeout);
void RS485_SetFrameCallback(RS485_FrameCallback_t callback);
uint8_t RS485_Available(void);
uint8_t RS485_Read(void);
uint8_t RS485_ReadBuffer(uint8_t *buffer, uint8_t max_length);
void RS485_FlushRx(void);
uint8_t RS485_IsTxBusy(void);
RS485_Status_t RS485_LoopbackTest(void);
uint8_t RS485_IsReady(void);
RS485_Status_t RS485_EchoTest(uint8_t test_byte, uint32_t timeout);
void RS485_IRQHandler(void);

#endif
