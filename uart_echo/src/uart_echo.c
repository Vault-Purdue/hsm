/*
 * Copyright (c) 2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_drivers_config.h"
#include "flash.h"

#include <string.h>     // For strlen

#define TEST_FLASH_ADDR 0x0003F000 //Page 252

uint8_t gBuffer[CONFIG_UART_BUFFER_LENGTH] = {0};

int main(void)
{
    size_t rdCount = 0;
    size_t wrCount = 0;
    UART_Params params;
    UART_Handle uartHandle;

    DL_GPIO_enablePower(GPIOA);

    UART_Params_init(&params);
    //UART is configured for default baudrate of 9600bps
    uartHandle = UART_open(CONFIG_UART_0, &params);
    if (uartHandle == NULL) {
        // UART_open failed
        __BKPT();
    }

    // FLASH READ/WRITE TEST
    
    const char* testData = "Flash Test Successful!\r\n";
    uint32_t dataSize = strlen(testData) + 1;
    uint8_t readBuffer[64] = {0};

    // 1. Erase the target page
    if (!flash_erase_page(TEST_FLASH_ADDR)) {
        UART_write(uartHandle, "Flash Erase Failed!\r\n", 21, &wrCount);
    } else {
        // 2. Write the test data to the erased page
        if (!flash_write(TEST_FLASH_ADDR, testData, dataSize)) {
            UART_write(uartHandle, "Flash Write Failed!\r\n", 21, &wrCount);
        } else {
            // 3. Read the data back from flash into our buffer
            flash_read(TEST_FLASH_ADDR, readBuffer, dataSize);

            // 4. Send the read data out over UART to verify it matches
            UART_write(uartHandle, "Read from Flash: ", 17, &wrCount);
            UART_write(uartHandle, readBuffer, dataSize - 1, &wrCount); 
        }
    }

    while (1) {
        UART_read(uartHandle, gBuffer, CONFIG_UART_BUFFER_LENGTH, &rdCount);
        UART_write(uartHandle, gBuffer, CONFIG_UART_BUFFER_LENGTH, &wrCount);
    }
}