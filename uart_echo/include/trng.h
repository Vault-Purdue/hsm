/**
 * @file trng.h
 * @author Vault Team - Purdue
 * @brief TRNG Driver Header File
 * @date 2026
 *
 * Constants/stubs for the TRNG driver.
 */

#ifndef _TRNG_H_
#define _TRNG_H_

#include <stdint.h>
#include <string.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/dl_trng.h>

/*********************** CONSTANTS ************************/

#define TRNG_32_BIT_BUF_SIZE 1
#define TRNG_64_BIT_BUF_SIZE 2
#define TRNG_128_BIT_BUF_SIZE 4
#define TRNG_256_BIT_BUF_SIZE 8
#define TRNG_CLOCK_DIVIDE DL_TRNG_CLOCK_DIVIDE_8
#define TRNG_DECIMATION_RATE DL_TRNG_DECIMATION_RATE_8
#define TRNG_NUM_WARMUP_CYCLES 4

/********************* FUNCTION STUBS *********************/

// Populates a number buffer with 32-bit random number
void trngGenerate32BitNumber(uint32_t *buf);

// Populates a number buffer with 64-bit random number
void trngGenerate64BitNumber(uint32_t *buf);

// Populates a number buffer with 128-bit random number
void trngGenerate128BitNumber(uint32_t *buf);

// Populates a number buffer with 256-bit random number
void trngGenerate256BitNumber(uint32_t *buf);

#endif