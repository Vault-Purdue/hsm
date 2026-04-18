/**
 * @file trng.h
 * @author Vault Team - Purdue
 * @brief TRNG Driver Header File
 * @date 2026
 *
 * Constants/stubs for the TRNG driver.
 */

#ifndef _HSM_TRNG_H_
#define _HSM_TRNG_H_

#include <stdint.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/dl_trng.h>

/*********************** CONSTANTS ************************/

#define TRNG_32_BIT_BUF_SIZE 1
#define TRNG_64_BIT_BUF_SIZE 2
#define TRNG_128_BIT_BUF_SIZE 4
#define TRNG_256_BIT_BUF_SIZE 8

/********************* FUNCTION STUBS *********************/

/**
 * @brief Populates a number buffer with 32-bit random number
 * 
 * @param buf Buffer to store the TRNG output
 */
void HSM_TRNG_generate32BitNumber(uint32_t *buf);

/**
 * @brief Populates a number buffer with 32-bit random number
 * 
 * @param buf Buffer to store the TRNG output
 */
void HSM_TRNG_generate64BitNumber(uint32_t *buf);

/**
 * @brief Populates a number buffer with 32-bit random number
 * 
 * @param buf Buffer to store the TRNG output
 */
void HSM_TRNG_generate128BitNumber(uint32_t *buf);

/**
 * @brief Populates a number buffer with 32-bit random number
 * 
 * @param buf Buffer to store the TRNG output
 */
void HSM_TRNG_generate256BitNumber(uint32_t *buf);

#endif