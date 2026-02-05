// sw_rs485.c - RS485 driver with interrupt-based RX/TX

#include "sw_rs485.h"
#include <string.h>

typedef struct {
    uint8_t buffer[RS485_RX_BUFFER_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
} RingBuffer_t;

static volatile uint8_t rs485_initialized = 0;
static volatile uint8_t rx_enabled = 0;
static RingBuffer_t rx_buffer = {.head = 0, .tail = 0};
static uint8_t tx_buffer[RS485_TX_BUFFER_SIZE];
static volatile uint16_t tx_length = 0;
static volatile uint16_t tx_index = 0;
static volatile uint8_t tx_busy = 0;
static RS485_FrameCallback_t frame_callback = NULL;
static uint8_t frame_buffer[RS485_RX_BUFFER_SIZE];
static volatile uint8_t frame_length = 0;

#define RS485_LOOPBACK_TEST_SIZE    8
#define RS485_LOOPBACK_TIMEOUT      100

static inline uint8_t RingBuffer_IsFull(RingBuffer_t *rb) {
    return ((rb->head + 1) % RS485_RX_BUFFER_SIZE) == rb->tail;
}

static inline uint8_t RingBuffer_IsEmpty(RingBuffer_t *rb) {
    return rb->head == rb->tail;
}

static inline uint8_t RingBuffer_Count(RingBuffer_t *rb) {
    return (rb->head - rb->tail + RS485_RX_BUFFER_SIZE) % RS485_RX_BUFFER_SIZE;
}

static inline void RingBuffer_Put(RingBuffer_t *rb, uint8_t data) {
    if (!RingBuffer_IsFull(rb)) {
        rb->buffer[rb->head] = data;
        rb->head = (rb->head + 1) % RS485_RX_BUFFER_SIZE;
    }
}

static inline uint8_t RingBuffer_Get(RingBuffer_t *rb) {
    if (RingBuffer_IsEmpty(rb)) return 0;
    uint8_t data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RS485_RX_BUFFER_SIZE;
    return data;
}

static inline void RingBuffer_Clear(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

RS485_Status_t RS485_Init(void) {
    if (huart2.Instance != USART2) return RS485_ERROR;
    RingBuffer_Clear(&rx_buffer);
    tx_length = 0;
    tx_index = 0;
    tx_busy = 0;
    frame_length = 0;
    frame_callback = NULL;
    rx_enabled = 0;
    rs485_initialized = 1;
    return RS485_OK;
}

RS485_Status_t RS485_StartReceive(void) {
    if (!rs485_initialized) return RS485_ERROR;
    RingBuffer_Clear(&rx_buffer);
    frame_length = 0;
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
    rx_enabled = 1;
    return RS485_OK;
}

RS485_Status_t RS485_StopReceive(void) {
    if (!rs485_initialized) return RS485_ERROR;
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_RXNE);
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_IDLE);
    rx_enabled = 0;
    return RS485_OK;
}

RS485_Status_t RS485_TransmitIT(uint8_t *data, uint16_t length) {
    if (!rs485_initialized) return RS485_ERROR;
    if (data == NULL || length == 0 || length > RS485_TX_BUFFER_SIZE) return RS485_ERROR;
    if (tx_busy) return RS485_BUSY;
    memcpy(tx_buffer, data, length);
    tx_length = length;
    tx_index = 0;
    tx_busy = 1;
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_TXE);
    return RS485_OK;
}

RS485_Status_t RS485_Transmit(uint8_t *data, uint16_t length, uint32_t timeout) {
    if (!rs485_initialized) return RS485_ERROR;
    if (data == NULL || length == 0) return RS485_ERROR;
    uint32_t start = HAL_GetTick();
    while (tx_busy) {
        if ((HAL_GetTick() - start) > timeout) return RS485_TIMEOUT;
    }
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, data, length, timeout);
    if (status == HAL_OK) return RS485_OK;
    if (status == HAL_TIMEOUT) return RS485_TIMEOUT;
    if (status == HAL_BUSY) return RS485_BUSY;
    return RS485_ERROR;
}

RS485_Status_t RS485_Receive(uint8_t *data, uint16_t length, uint32_t timeout) {
    if (!rs485_initialized) return RS485_ERROR;
    if (data == NULL || length == 0) return RS485_ERROR;
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, data, length, timeout);
    if (status == HAL_OK) return RS485_OK;
    if (status == HAL_TIMEOUT) return RS485_TIMEOUT;
    if (status == HAL_BUSY) return RS485_BUSY;
    return RS485_ERROR;
}

void RS485_SetFrameCallback(RS485_FrameCallback_t callback) {
    frame_callback = callback;
}

uint8_t RS485_Available(void) {
    return RingBuffer_Count(&rx_buffer);
}

uint8_t RS485_Read(void) {
    return RingBuffer_Get(&rx_buffer);
}

uint8_t RS485_ReadBuffer(uint8_t *buffer, uint8_t max_length) {
    uint8_t count = 0;
    while (!RingBuffer_IsEmpty(&rx_buffer) && count < max_length) {
        buffer[count++] = RingBuffer_Get(&rx_buffer);
    }
    return count;
}

void RS485_FlushRx(void) {
    RingBuffer_Clear(&rx_buffer);
    frame_length = 0;
}

uint8_t RS485_IsTxBusy(void) {
    return tx_busy;
}

uint8_t RS485_IsReady(void) {
    if (!rs485_initialized) return 0;
    return (huart2.gState == HAL_UART_STATE_READY && !tx_busy) ? 1 : 0;
}

RS485_Status_t RS485_LoopbackTest(void) {
    if (!rs485_initialized) return RS485_ERROR;
    uint8_t tx_data[RS485_LOOPBACK_TEST_SIZE] = {0xAA, 0x55, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    huart2.Instance->CR1 &= ~USART_CR1_UE;
    huart2.Instance->CR2 |= USART_CR2_LBCL;
    huart2.Instance->CR1 |= USART_CR1_UE;
    return RS485_Transmit(tx_data, RS485_LOOPBACK_TEST_SIZE, RS485_LOOPBACK_TIMEOUT);
}

RS485_Status_t RS485_EchoTest(uint8_t test_byte, uint32_t timeout) {
    if (!rs485_initialized) return RS485_ERROR;
    uint8_t rx_byte = 0;
    RS485_Status_t status = RS485_Transmit(&test_byte, 1, timeout);
    if (status != RS485_OK) return status;
    HAL_Delay(1);
    status = RS485_Receive(&rx_byte, 1, timeout);
    if (status != RS485_OK) return status;
    return (rx_byte == test_byte) ? RS485_OK : RS485_ERROR;
}

void RS485_IRQHandler(void) {
    uint32_t isrflags = huart2.Instance->ISR;
    uint32_t cr1its = huart2.Instance->CR1;

    // RX
    if ((isrflags & USART_ISR_RXNE_RXFNE) && (cr1its & USART_CR1_RXNEIE_RXFNEIE)) {
        uint8_t data = (uint8_t)(huart2.Instance->RDR & 0xFF);
        RingBuffer_Put(&rx_buffer, data);
        if (frame_length < RS485_RX_BUFFER_SIZE) {
            frame_buffer[frame_length++] = data;
        }
    }

    // IDLE
    if ((isrflags & USART_ISR_IDLE) && (cr1its & USART_CR1_IDLEIE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);
        if (frame_callback != NULL && frame_length > 0) {
            frame_callback(frame_buffer, frame_length);
        }
        frame_length = 0;
    }

    // TXE
    if ((isrflags & USART_ISR_TXE_TXFNF) && (cr1its & USART_CR1_TXEIE_TXFNFIE)) {
        if (tx_index < tx_length) {
            huart2.Instance->TDR = tx_buffer[tx_index++];
        } else {
            __HAL_UART_DISABLE_IT(&huart2, UART_IT_TXE);
            __HAL_UART_ENABLE_IT(&huart2, UART_IT_TC);
        }
    }

    // TC
    if ((isrflags & USART_ISR_TC) && (cr1its & USART_CR1_TCIE)) {
        __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_TCF);
        __HAL_UART_DISABLE_IT(&huart2, UART_IT_TC);
        tx_busy = 0;
        tx_index = 0;
        tx_length = 0;
    }

    // Errors
    if (isrflags & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE)) {
        __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);
    }
}
