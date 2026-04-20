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

#include "ti_drivers_config.h"

/*
 *  =============================== UART ===============================
 */
#include <ti/drivers/uart/UARTMSPM0.h>

const uint_least8_t CONFIG_UART_0 = 0;
const uint_least8_t UART_count    = CONFIG_UART_COUNT;

static const UARTMSP_HWAttrs UARTMSPHWAttrs[CONFIG_UART_COUNT] = {
    // UART1
    {
        .regs          = UART0,
        .irq           = UART0_INT_IRQn,
        .rxPin          = IOMUX_PINCM26,     
        .rxPinFunction  = IOMUX_PINCM26_PF_UART0_RX,
        .txPin          = IOMUX_PINCM25,     
        .txPinFunction  = IOMUX_PINCM25_PF_UART0_TX,
        .mode          = DL_UART_MODE_NORMAL,
        .direction     = DL_UART_DIRECTION_TX_RX,
        .flowControl   = DL_UART_FLOW_CONTROL_NONE,
        .clockSource   = DL_UART_CLOCK_BUSCLK,
        .clockDivider  = DL_UART_CLOCK_DIVIDE_RATIO_1,
        .rxIntFifoThr  = DL_UART_RX_FIFO_LEVEL_ONE_ENTRY,
        .txIntFifoThr  = DL_UART_TX_FIFO_LEVEL_EMPTY,
    },
};

UART_Data_Object UARTObject[CONFIG_UART_COUNT] = {
    {
        .object =
            {
                .supportFxns        = &UARTMSPSupportFxns,
                .buffersSupported   = true,
                .eventsSupported    = false,
                .callbacksSupported = false,
                .dmaSupported       = false,
            },
        .buffersObject =
            {
                .rxBufPtr  = rxBuffer,
                .txBufPtr  = txBuffer,
                .rxBufSize = sizeof(rxBuffer),
                .txBufSize = sizeof(txBuffer),
            },
    },
};

const UART_Config UART_config[CONFIG_UART_COUNT] = {
    {
        &UARTObject[CONFIG_UART_0],
        &UARTMSPHWAttrs[CONFIG_UART_0],
    },
};

void UART0_IRQHandler(void)
{
    UARTMSP_interruptHandler((UART_Handle) &UART_config[0]);
}

/*
 *  =============================== GPIO ===============================
 */
void SYS_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_GPIO_reset(GPIOC);
    DL_TimerG_reset(TIMER_0_INST);
    DL_TRNG_reset(TRNG);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_GPIO_enablePower(GPIOC);
    DL_TimerG_enablePower(TIMER_0_INST);
    DL_TRNG_enablePower(TRNG);
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
    
    delay_cycles(POWER_STARTUP_DELAY);

    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
}

void GPIO_init(void)
{
    DL_GPIO_initDigitalOutput(GPIO_RED_LED_IOMUX);
    DL_GPIO_clearPins(GPIOA, GPIO_RED_LED_PIN);
    DL_GPIO_enableOutput(GPIOA, GPIO_RED_LED_PIN);
}
/*
 *  =============================== TIMER ===============================
 */
 /*
 * Timer clock configuration to be sourced by BUSCLK /  (8000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   40000 Hz = 8000000 Hz / (4 * (199 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_0ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_4,
    .prescale    = 199U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_0_INST_LOAD_VALUE = (500ms * 40000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_0TimerConfig = {
    .period     = TIMER_0_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC_UP,
    .startTimer = DL_TIMER_STOP,
};

void TIMER_0_init(void) {

    DL_TimerG_setClockConfig(TIMER_0_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_0ClockConfig);

    DL_TimerG_initTimerMode(TIMER_0_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_0TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_0_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(TIMER_0_INST);

    DL_TimerG_enableEvent(TIMER_0_INST, DL_TIMERG_EVENT_ROUTE_1, (DL_TIMERG_EVENT_ZERO_EVENT));

    DL_TimerG_setPublisherChanID(TIMER_0_INST, DL_TIMERG_PUBLISHER_INDEX_0, TIMER_0_INST_PUB_0_CH);
}

HSM_TRNG_STATUS HSM_TRNG_init(void) {
    // Set the TRNG functional clock
    // LP-MSPM0L2228 has a 32MHz system clock, we use the TRNG Clock Divider
    // to establish the frequency of the TRNG clock
    DL_TRNG_setClockDivider(TRNG, TRNG_CLOCK_DIVIDE);

    // Clear interrupts
    DL_TRNG_disableInterrupt(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);
    DL_TRNG_disableInterrupt(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);
    DL_TRNG_disableInterrupt(TRNG, DL_TRNG_INTERRUPT_HEALTH_FAIL_EVENT);
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);

    // Transition to the NORM_FUNC state
    // Wait for the IRQ_CMD_DONE flag to be set,
    // which indicates function completion
    // Small chance of INTERRUPT_CMD_FAIL being returned
    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
    while (!DL_TRNG_isCommandDone(TRNG));
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);
    DL_TRNG_enableInterrupt(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);
        
    // Perform digital tests
    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_TEST_DIG);
    while (!DL_TRNG_isCommandDone(TRNG));
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
    if (DL_TRNG_getDigitalHealthTestResults(TRNG) != DL_TRNG_DIGITAL_HEALTH_TEST_SUCCESS) return HSM_TRNG_ERR_DIG_TEST_FAIL;

    // Perform analog tests
    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_TEST_ANA);
    while (!DL_TRNG_isCommandDone(TRNG));
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);

    // If the analog tests fail, it is recommended to perform this sequence
    // If we fail the tests 3 times, the TRNG module CANNOT BE USED
    if (DL_TRNG_getAnalogHealthTestResults(TRNG) != DL_TRNG_ANALOG_HEALTH_TEST_SUCCESS) {
        for (uint8_t i = 0; i < 2; i++) {

            // Clear health fail interrupt
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_HEALTH_FAIL_EVENT);
            
            // Disable power
            DL_TRNG_disablePower(TRNG);
            while(DL_TRNG_isPowerEnabled(TRNG));
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);

            // Enable power again
            DL_TRNG_enablePower(TRNG);
            while(!DL_TRNG_isPowerEnabled(TRNG));
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);

            // Repeat the startup sequence again
            DL_TRNG_setClockDivider(TRNG, TRNG_CLOCK_DIVIDE);
            DL_TRNG_disableInterrupt(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);
            DL_TRNG_disableInterrupt(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);
            DL_TRNG_disableInterrupt(TRNG, DL_TRNG_INTERRUPT_HEALTH_FAIL_EVENT);
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
            DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
            while (!DL_TRNG_isCommandDone(TRNG));
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);
            DL_TRNG_enableInterrupt(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);
            DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_TEST_DIG);
            while (!DL_TRNG_isCommandDone(TRNG));
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
            if (DL_TRNG_getDigitalHealthTestResults(TRNG) != DL_TRNG_DIGITAL_HEALTH_TEST_SUCCESS) return HSM_TRNG_ERR_DIG_TEST_FAIL;

            // Perform analog tests
            DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_TEST_ANA);
            while (!DL_TRNG_isCommandDone(TRNG));
            DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
            if (DL_TRNG_getAnalogHealthTestResults(TRNG) == DL_TRNG_ANALOG_HEALTH_TEST_SUCCESS) break;
            
            // Ran three times and still failed
            if (i >= 1) return HSM_TRNG_ERR_ANA_TEST_FAIL;
        }
    }

    // Clear capture ready status leftover from tests
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);

    // Set the TRNG decimation rate
    DL_TRNG_setDecimationRate(TRNG, TRNG_DECIMATION_RATE);

    // Enable the remaining interrupts
    DL_TRNG_enableInterrupt(TRNG, DL_TRNG_INTERRUPT_HEALTH_FAIL_EVENT);
    DL_TRNG_enableInterrupt(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);

    // Throw out the first capture received, this is left over due to tests and not random
    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
    while (!DL_TRNG_isCommandDone(TRNG));
    while (!DL_TRNG_isCaptureReady(TRNG));
    (void) DL_TRNG_getCapture(TRNG);
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);
    
    return HSM_TRNG_OK;
}