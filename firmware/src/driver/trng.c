/**
 * @file trng.c
 * @author Vault Team - Purdue
 * @brief TRNG Driver
 * @date 2026
 *
 * Functions for interacting with the TRNG module to generate random numbers
 */

#include "trng.h"

void HSM_TRNG_generateNumber(uint32_t *buf, size_t len) {

    // Zeroize the buffer, just in case
    memset(buf, 0, len);

    // Write random 32-bit numbers to TRNG buffer
    for (uint8_t i = 0; i < len; i++) {
        while (!DL_TRNG_isCaptureReady(TRNG));
        buf[i] = DL_TRNG_getCapture(TRNG);
        DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
        DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);
    }
}

void HSM_TRNG_generate32BitNumber(uint32_t *buf, size_t len) {
    if (len / sizeof(uint32_t) == TRNG_32_BIT_BUF_SIZE) HSM_TRNG_generateNumber(buf, TRNG_32_BIT_BUF_SIZE);
}

void HSM_TRNG_generate64BitNumber(uint32_t *buf, size_t len) {
    if (len / sizeof(uint32_t) == TRNG_64_BIT_BUF_SIZE) HSM_TRNG_generateNumber(buf, TRNG_64_BIT_BUF_SIZE);
}

void HSM_TRNG_generate128BitNumber(uint32_t *buf, size_t len) {
    if (len / sizeof(uint32_t) == TRNG_128_BIT_BUF_SIZE) HSM_TRNG_generateNumber(buf, TRNG_128_BIT_BUF_SIZE);
}

void HSM_TRNG_generate256BitNumber(uint32_t *buf, size_t len) {
    if (len / sizeof(uint32_t) == TRNG_256_BIT_BUF_SIZE) HSM_TRNG_generateNumber(buf, TRNG_256_BIT_BUF_SIZE);
}
