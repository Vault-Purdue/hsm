/**
 * @file trng.c
 * @author Vault Team - Purdue
 * @brief TRNG Driver
 * @date 2026
 *
 * Functions for interacting with the TRNG module to generate random numbers
 */

#include "trng.h"

/**
 * @brief Powers on and initializes the TRNG module
 * 
 * This function powers on the TRNG module and
 * initializes the TRNG clock and decimation rates.
 */
static void trngInit(void) {

    // Reset TRNG
    DL_TRNG_reset(TRNG);

    // Enable power to TRNG
    DL_TRNG_enablePower(TRNG);

    // Set the TRNG functional clock
    // LP-MSPM0L2228 has a 32MHz system clock, we use the TRNG Clock Divider
    // to establish the frequency of the TRNG clock
    DL_TRNG_setClockDivider(TRNG, TRNG_CLOCK_DIVIDE);

    // Transition to the NORM_FUNC state
    // Wait for the IRQ_CMD_DONE flag to be set,
    // which indicates function completion
    // Command fail interrupt might occur on first command write
    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
    while (!DL_TRNG_isCommandDone(TRNG));
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_FAIL_EVENT);

    // Set the TRNG decimation rate
    DL_TRNG_setDecimationRate(TRNG, TRNG_DECIMATION_RATE);
}

/**
 * @brief Writes random 32-bit numbers to buffer
 * 
 * This function captures responses from the TRNG module and stores them in a specified buffer
 */
static void trngGenerateNumber(uint32_t *buf, size_t len) {
    
    // Initialize the TRNG module
    trngInit();

    // Let the TRNG module warm up
    // Toss out first couple of readings
    for (uint8_t i = 0; i < TRNG_NUM_WARMUP_CYCLES; i++) {
        DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
        while (!DL_TRNG_isCommandDone(TRNG));
        while (!DL_TRNG_isCaptureReady(TRNG)) ;
        (void)DL_TRNG_getCapture(TRNG);
    }

    // Write random 32-bit numbers to TRNG buffer
    for (uint8_t i = 0; i < len; i++) {
        DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
        while (!DL_TRNG_isCommandDone(TRNG));
        while (!DL_TRNG_isCaptureReady(TRNG));
        buf[i] = DL_TRNG_getCapture(TRNG);
    }

    // Poer off TRNG
    DL_TRNG_disablePower(TRNG);
}

void trngGenerate32BitNumber(uint32_t *buf) { trngGenerateNumber(buf, TRNG_32_BIT_BUF_SIZE); }

void trngGenerate64BitNumber(uint32_t *buf) { trngGenerateNumber(buf, TRNG_64_BIT_BUF_SIZE); }

void trngGenerate128BitNumber(uint32_t *buf) { trngGenerateNumber(buf, TRNG_128_BIT_BUF_SIZE); }

void trngGenerate256BitNumber(uint32_t *buf) { trngGenerateNumber(buf, TRNG_256_BIT_BUF_SIZE); }
