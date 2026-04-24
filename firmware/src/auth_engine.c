/**
 * @file auth_engine.c
 * @author Vault Team - Purdue
 * @brief PIN Authentication Engine
 * @date 2026
 *
 * Function for validating the PIN provided by the user. 
 */

/************************ FUNCTIONS ***********************/

/** @brief Authentication Engine
 *
 * This function is called to authenticate the PIN provided 
 * by the user over the UART interface.  The outcome of the 
 * authentication, in both the passed and failed cases,
 * is used to update the system state machine.  The system 
 * state machine runs an exponential holdoff timer after each
 * failed authentication attempt.
 *
 * @param provided_pin PIN received from the UART interface
 *
 * @return None
 */

#include "state_machine.h"
#include <string.h>

#define PIN_LENGTH 6

void authentication_engine(const char *provided_pin) {
    SystemState state;
    const char expected_pin[] = "123456";

    state = system_state_machine(EVENT_NONE);
    if (state != STATE_LOCKED) {
        return;
    }
    
    if (provided_pin && strcmp(expected_pin, provided_pin) == 0) {
        system_state_machine(EVENT_USER_AUTHENTICATED);
        return;
    }
    system_state_machine(EVENT_INVALID_PIN);
}