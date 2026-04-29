/*
 * Copyright (c) 2021, Texas Instruments Incorporated
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
#ifndef ti_drivers_config_h
#define ti_drivers_config_h

#include <stdio.h>
#include <stdbool.h>

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/m0p/dl_core.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTMSPM0.h>
#include <ti/driverlib/dl_trng.h>

/* clang-format off */
#define POWER_STARTUP_DELAY                                                 (16)

/* Defines for UART 0 */
#define CONFIG_UART_COUNT 1
#define CONFIG_UART_BUFFER_LENGTH 1

/* Defines for GPIO */
#define GPIO_RED_LED_PORT                                                (GPIOA)
#define GPIO_RED_LED_PIN                                         (DL_GPIO_PIN_0)
#define GPIO_RED_LED_IOMUX                                        (IOMUX_PINCM1)

/* Defines for TIMER_0 (Session Timeout)*/
#define TIMER_0_INST                                                    (TIMG12)
#define TIMER_0_INST_IRQHandler                                TIMG12_IRQHandler
#define TIMER_0_INST_INT_IRQN                                  (TIMG12_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                      (39999999U) // (1199999999U)

/* Defines for TIMER_1 (PIN Backoff) */
#define TIMER_1_INST                                                     (TIMG8)
#define TIMER_1_INST_IRQHandler                                 TIMG8_IRQHandler
#define TIMER_1_INST_INT_IRQN                                   (TIMG8_INT_IRQn)
#define TIMER_1_INST_LOAD_VALUE                                            (40U)

/* Defines for TRNG */
#define TRNG_CLOCK_DIVIDE                               (DL_TRNG_CLOCK_DIVIDE_2)
#define TRNG_DECIMATION_RATE                         (DL_TRNG_DECIMATION_RATE_8)
typedef enum {
    HSM_TRNG_OK,
    HSM_TRNG_ERR_DIG_TEST_FAIL,
    HSM_TRNG_ERR_ANA_TEST_FAIL
} HSM_TRNG_STATUS;

/* Defines for LCD */
#define LCD_IRQHandler                                            LCD_IRQHandler
#define LCD_INT_IRQN                                                LCD_INT_IRQn

extern const uint_least8_t CONFIG_UART_0;
extern const uint_least8_t UART_count;

uint8_t rxBuffer[CONFIG_UART_BUFFER_LENGTH];
uint8_t txBuffer[CONFIG_UART_BUFFER_LENGTH];

/* clang-format on */
void SYS_initPower(void);
void UART1_IRQHandler(void);
void GPIO_init(void);
void LCD_init(void);
void TIMER_0_init(void);
void TIMER_1_init(void);

/**
 * @brief Initializes the TRNG module as described in the MSPM0 L-Series Technical Reference Manual.
 * 
 * @retval 0: TRNG successfully initialized.
 * @retval 1: Digital start up tests failed, it is NOT SAFE to use the TRNG.
 * @retval 2: Analog start up tests failed, it is NOT SAFE to use the TRNG.
 */
HSM_TRNG_STATUS HSM_TRNG_init(void);

#endif /* ti_drivers_config_h */
