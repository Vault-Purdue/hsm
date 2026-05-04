/**
 * @file state_machine.c
 * @author Vault Team - Purdue
 * @brief System State Machine Functions
 * @date 2026
 *
 * Function for getting/setting the System State.
 */

#include "state_machine.h"
#include "ti_drivers_config.h"

/************************ FUNCTIONS ***********************/

/** @brief System State Machine
 *
 * This function is called to get or change the System State.  
 * The function accepts the parameter event, updates the 
 * system state based on the event that occurred and returns
 * the new system state.  Use EVENT_NONE to obtain the current
 * system state without an occurrence of an event.
 *
 * @param event used to update the system state machine
 *
 * @return SystemState ENUM
 */

SystemState system_state_machine(SystemStateEvent event) {
    
#if DEBUG_SKIP_AUTH
    static SystemState state = STATE_UNLOCKED;
#else
    static SystemState state = STATE_WAIT_FOR_UART;
#endif
    static uint8_t invalid_pin_count = 0;
    const uint8_t MAX_RETRIES = 10;
    volatile uint16_t timer_load_val;

    if (state == STATE_LOCKOUT) {
        return state;
    } else {
        switch (event) {
            case EVENT_NONE:
                break;
            case EVENT_SESSION_CLOSE_USER:
            case EVENT_SESSION_CLOSE_TIMEOUT:
                DL_TimerG_stopCounter(TIMER_0_INST);
                DL_TimerG_setTimerCount(TIMER_0_INST, TIMER_0_INST_LOAD_VALUE);
                state = STATE_WAIT_FOR_UART;
                invalid_pin_count = 0;
                break;
            case EVENT_UART_SESSION_ESTABLISHED:
                if(state == STATE_WAIT_FOR_UART) {
                    state = STATE_WAIT_FOR_PIN;
                }
                break;
            case EVENT_USER_AUTHENTICATED:
                if (state == STATE_WAIT_FOR_PIN) {
                    DL_TimerG_stopCounter(TIMER_0_INST);
                    DL_TimerG_setTimerCount(TIMER_0_INST, TIMER_0_INST_LOAD_VALUE);
                    DL_TimerG_startCounter(TIMER_0_INST);
                    state = STATE_UNLOCKED;
                    invalid_pin_count = 0;
                }
                break;
            case EVENT_CMD_RECEIVED:
                //__BKPT();
                if (state == STATE_UNLOCKED) {
                    DL_TimerG_stopCounter(TIMER_0_INST);
                    DL_TimerG_setTimerCount(TIMER_0_INST, TIMER_0_INST_LOAD_VALUE);
                    DL_TimerG_startCounter(TIMER_0_INST);
                }
                break;
            case EVENT_INVALID_PIN:
                if (state == STATE_WAIT_FOR_PIN) {
                    invalid_pin_count++;
                    if (invalid_pin_count == MAX_RETRIES) {
                        state = STATE_LOCKOUT;
                    } else {
                        timer_load_val = TIMER_1_INST_LOAD_VALUE << invalid_pin_count;
                        DL_TimerG_stopCounter(TIMER_1_INST);
                        DL_TimerG_setLoadValue(TIMER_1_INST, timer_load_val);
                        DL_TimerG_setTimerCount(TIMER_1_INST, timer_load_val);
                        DL_TimerG_startCounter(TIMER_1_INST);
                        state = STATE_PIN_HOLDOFF;
                    }
                break;
                }
            case EVENT_HOLDOFF_EXPIRED:
                if (state == STATE_PIN_HOLDOFF) {
                    DL_TimerG_stopCounter(TIMER_1_INST);
                    DL_TimerG_setTimerCount(TIMER_1_INST, TIMER_1_INST_LOAD_VALUE);
                    state = STATE_WAIT_FOR_PIN;
                }
        }
        return state;
    }
}