/*
 * app.c
 *
 *  Created on: Jan 9, 2026
 *      Author: Richard Roztocil
 */

#include "app.h"
#include "sw_ws2815.h"
#include "sw_effects.h"
#include "sw_rs485.h"
#include "sw_protocol.h"
#include "stm32g0xx_hal.h"

void APP_Init(void)
{
    // Initialize WS2815 LED driver
    WS2815_Init();

    // Set initial brightness
    WS2815_SetBrightness(50);

    // Initialize effects module
    Effects_Init();

    // Initialize RS485 driver
    RS485_Init();

    // Initialize protocol handler (reads address from BCD switches)
    Protocol_Init();

    // Start receiving on RS485
    RS485_StartReceive();
}


void APP_Main(void)
{
    while(1) {
        // Process incoming RS485 commands and run effects
        Protocol_Process();
    }
}
