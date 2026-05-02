/**
 * @file hsm.c
 * @author Vault Team - Purdue
 * @brief main function for the HSM
 * @date 2026
 */

#include "uart_cmd_router.h"
#include "file_manager.h"
#include "auth_engine.h"
#include "state_machine.h"

/************************ FUNCTIONS ***********************/
router_status_t router_dispatch(uart_frame_t *rx_frame) {
    SystemState sys_state;
    if (rx_frame == NULL) {
        uart_send_debug_msg("ERROR: Null frame in router");
        return RT_FAIL;
    }
    //__BKPT();
    sys_state = system_state_machine(EVENT_NONE);
    switch (rx_frame->msg_id) {
        case MSG_SESSION_OPEN:
            uart_send_debug_msg("Session Open message received.");
            if (sys_state == STATE_WAIT_FOR_UART) {
                // TODO (Aidan): Open UART Session
                // Aidan: The call below is a stub to keep state flow moving until the ECDH session is established.  
                //        Please move the call to whatever module you feel is the most appropriate location.
                //        See auth_engine.c for an example of how I issue EVENT_USER_AUTHENTICATED.
                sys_state = system_state_machine(EVENT_UART_SESSION_ESTABLISHED);
            }
            return RT_OK;

        case MSG_KEY_EXCHANGE:
            uart_send_debug_msg("Key Exchange message received.");
            if (sys_state == STATE_WAIT_FOR_UART) {
                // TODO (Aidan): Open UART Session
                // Aidan: The call below is a stub to keep state flow moving until the ECDH session is established.  
                //        Please move the call to whatever module you feel is the most appropriate location.
                //        See auth_engine.c for an example of how I issue EVENT_USER_AUTHENTICATED.
                sys_state = system_state_machine(EVENT_UART_SESSION_ESTABLISHED);
            }
            return RT_OK;

        case MSG_PIN_EXCHANGE:
            uart_send_debug_msg("PIN Exchange message received.");
            if (sys_state == STATE_WAIT_FOR_PIN) {
                authentication_engine(rx_frame);
            }
            return RT_OK;

        case MSG_SESSION_CLOSE:
            uart_send_debug_msg("Session Close message received.");
            // TODO (Alex): Need to tear down the ECDH session here
            sys_state = system_state_machine(EVENT_SESSION_CLOSE_USER);
            return RT_OK;

        case MSG_FILE_TRANSFER_REQUEST:
            sys_state = system_state_machine(EVENT_CMD_RECEIVED);
            if (sys_state == STATE_UNLOCKED) {
                return handle_file_transfer_request(rx_frame);
            }
            
        case MSG_FILE_CONTENTS:
            sys_state = system_state_machine(EVENT_CMD_RECEIVED);
            if (sys_state == STATE_UNLOCKED) {
                return handle_file_contents(rx_frame);
            }

        default:
            uart_send_debug_msg_with_str("ERROR: Unknown Message ID", msg_id_to_str(rx_frame->msg_id));
            return RT_FAIL;
    }
}

const char* msg_id_to_str(uint8_t msg_id) {
    switch (msg_id) {
        case MSG_SESSION_OPEN:               return "SESSION_OPEN";
        case MSG_KEY_EXCHANGE:               return "KEY_EXCHANGE";
        case MSG_PIN_EXCHANGE:               return "PIN_EXCHANGE";
        case MSG_SESSION_CLOSE:              return "SESSION_CLOSE";
        case MSG_FILE_TRANSFER_REQUEST:      return "FILE_TRANSFER_REQUEST";
        case MSG_FILE_CONTENTS:              return "FILE_CONTENTS";
        case MSG_FILE_TRANSFER_COMPLETE:     return "FILE_TRANSFER_COMPLETE";
        case MSG_FILE_REQUEST_ACK:           return "FILE_REQUEST_ACK";
        case MSG_FILE_TRANSFER_COMPLETE_ACK: return "FILE_TRANSFER_COMPLETE_ACK";
        case MSG_PIN_EXCHANGE_ACK:           return "PIN_EXCHANGE_ACK";
        case MSG_DEBUG:                      return "DEBUG";
        default:                             return "UNKNOWN";
    }
}

router_status_t handle_file_transfer_request(uart_frame_t *frame) {
    if (frame == NULL)           return RT_FAIL;
    if (frame->payload_len != 2) return RT_FAIL;

    uint8_t direction = frame->payload[0];
    uint8_t file_id   = frame->payload[1]; // 1B

    if (direction != FM_DIR_WRITE && direction != FM_DIR_READ) return RT_FAIL;

    return fm_file_transfer_request(direction, file_id);
}

router_status_t handle_file_contents(uart_frame_t *frame) {
    if (frame == NULL) return RT_FAIL;
    if (frame->payload_len == 0 || frame->payload_len > FM_MAX_PAYLOAD_SIZE) return RT_FAIL;
    return fm_handle_file_contents(frame->payload, frame->payload_len);
}