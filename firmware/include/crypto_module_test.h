#ifndef _CRYPTO_MODULE_TEST_H_
#define _CRYPTO_MODULE_TEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "crypto_module.h"

/**
 * @brief Tests the session functionality of the HSM crypto module.
 *
 * @returns true on success.
 */
bool HSM_CRYPTOTEST_sessionTest(void);

/**
 * @brief Tests encrypting / decrypting AES-256-GCM plaintext keys.
 *
 * @returns true on success.
 */
bool HSM_CRYPTOTEST_fileKeyEncryptionTest(void);

/**
 * @brief Tests encrypting / decrypting message payloads.
 *
 * @returns true on success.
 */
bool HSM_CRYPTOTEST_messagePayloadEncryptionTest(void);

#endif