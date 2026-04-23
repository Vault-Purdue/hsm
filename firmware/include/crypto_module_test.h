#ifndef _CRYPTO_MODULE_TEST_H_
#define _CRYPTO_MODULE_TEST_H_

#define ENABLE_HSM_CRYPTO_SESSION_TEST

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

#endif