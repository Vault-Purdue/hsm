/**
 * @file auth_engine.h
 * @author Vault Team - Purdue
 * @brief PIN Authentication Engine
 * @date 2026
 *
 * Function for validating the PIN provided by the user.
 */

#ifndef AUTH_ENGINE_H
#define AUTH_ENGINE_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    AE_OK,
    AE_FAIL,
} auth_eng_status_t;

/************************ FUNCTIONS ***********************/

auth_eng_status_t authentication_engine(uint8_t *pin, size_t len);

#endif