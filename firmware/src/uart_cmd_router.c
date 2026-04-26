/**
 * @file hsm.c
 * @author Vault Team - Purdue
 * @brief main function for the HSM
 * @date 2026
 */

#include "uart_cmd_router.h"

/************************ FUNCTIONS ***********************/
router_status_t router_dispatch(uart_frame_t *rx_frame) {
    if (rx_frame == NULL) {
        uart_send_debug_msg("ERROR: Null frame in router");
        return RT_FAIL;
    }

    switch (rx_frame->msg_id) {
        case MSG_SESSION_OPEN:
            // return handle_session_open(rx_frame);
            uart_send_debug_msg("Session Open message received.");
            return RT_OK;

        case MSG_KEY_EXCHANGE:
            // return handle_key_exchange(rx_frame);
            uart_send_debug_msg("Key Exchange message received.");
            return RT_OK;

        case MSG_PIN_EXCHANGE:
            // return handle_pin_exchange(rx_frame);
            uart_send_debug_msg("PIN Exchange message received.");
            return RT_OK;

        case MSG_SESSION_CLOSE:
            // return handle_session_close(rx_frame);
            uart_send_debug_msg("Session Close message received.");
            return RT_OK;

        case MSG_FILE_TRANSFER_REQUEST:
            return handle_file_transfer_request(rx_frame);

        case MSG_FILE_CONTENTS:
            // return handle_file_contents(rx_frame);
            uart_send_debug_msg("File Contents message received.");
            return RT_OK;

        case MSG_FILE_TRANSFER_COMPLETE:
            // return handle_file_transfer_complete(rx_frame);
            uart_send_debug_msg("File Transfer Complete message received.");
            return RT_OK;

        case MSG_FILE_REQUEST_ACK:
            // return handle_file_request_ack(rx_frame);
            uart_send_debug_msg("File Request ACK received.");
            return RT_OK;

        case MSG_FILE_TRANSFER_COMPLETE_ACK:
            // return handle_file_transfer_complete_ack(rx_frame);
            uart_send_debug_msg("File Transfer Complete ACK received.");
            return RT_OK;

        case MSG_PIN_EXCHANGE_ACK:
            // return handle_pin_exchange_ack(rx_frame);
            uart_send_debug_msg("PIN Exchange ACK received.");
            return RT_OK;

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

    if (direction != 0x77 && direction != 0x72) return RT_FAIL;

    return fm_file_transfer_request(direction, (uint16_t)file_id);
}