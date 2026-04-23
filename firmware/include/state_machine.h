/**
 * @file state_machine.h
 * @author Vault Team - Purdue
 * @brief System State Machine Functions
 * @date 2026
 *
 * Functions for navigating the system state machine.
 */

/******************* CUSTOM DATA TYPES ********************/

typedef enum {
    EVENT_NONE,
    EVENT_USER_AUTHENTICATED,
    EVENT_SESSION_CLOSE_USER,
    EVENT_SESSION_CLOSE_TIMEOUT,
    EVENT_CMD_RECEIVED,
    EVENT_INVALID_PIN,
    EVENT_HOLDOFF_EXPIRED
} SystemStateEvent;

typedef enum {
    STATE_LOCKED,
    STATE_UNLOCKED,
    STATE_PIN_HOLDOFF,
    STATE_LOCKOUT
} SystemState;

/************************ FUNCTIONS ***********************/

SystemState system_state_machine(SystemStateEvent event);