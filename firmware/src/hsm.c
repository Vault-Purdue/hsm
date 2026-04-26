/**
 * @file hsm.c
 * @author Vault Team - Purdue
 * @brief main function for the HSM
 * @date 2026
 */

#include "ti_drivers_config.h"
#include "flash.h"
#include <string.h>
#include <stdint.h>
#include "crypto_module_test.h"
#include "crypto_module.h"
#include "uart_protocol.h"
#include "uart_cmd_router.h"
#include "led_status.h"
#include "aes_adv_gcm_test.h"
#include "aes_adv_gcm.h"
#include "file_manager.h"

/** @brief Initializes peripherals for system boot.
*/
void init() {
    // Initialize all of the hardware components
    SYS_initPower();

    // Initialize crypto module
    HSM_CRYPTO_init();
    // TODO: Initialize file manager
    // init_fm();
}

/************************MAIN LOOP ************************/
int main(void) {
    uart_frame_t rx_frame;
    //uart_msg_id_t cmd;

    // Initialize device peripherals
    init();
    AESADV_init();
    uart_init();

    // Set breakpoint if we fail crypto session test
    if (!HSM_CRYPTOTEST_sessionTest()) {
        __BKPT();
    } else if (!HSM_CRYPTOTEST_fileKeyEncryptionTest()) {
        __BKPT();
    } else if (!HSM_CRYPTOTEST_messagePayloadEncryptionTest()) {
        __BKPT();
    }

    __BKPT();

    if (AESADV_GCM_selfTest()) {
        STATUS_LED_ON(); 
    } else {
        while (1) {
            STATUS_LED_ON();
            delay_cycles(2000000);
            STATUS_LED_OFF();
            delay_cycles(2000000);
        }
    }
    while (1) {
        //print_debug("Ready\n");
        STATUS_LED_ON();
        
        // Block until a full frame is received
        int result = 0;
        while (result == UART_RECV_NO_ERROR) {
            result = uart_receive_frame(&rx_frame);
        }

        // Send a debug message if an error was encountered.
        // TOOD: Debug messaging will probably go away in the final version, so this should be handled in a different way?
        if (result != UART_RECV_FULL_FRAME_RECEIVED) {
            STATUS_LED_OFF();
            switch (result) {
            case UART_RECV_ERROR_BAD_SOF:
                uart_send_debug_msg("ERROR: Msg Bad SOF");
                break;
            case UART_RECV_ERROR_PAYLOAD_TOO_LONG:
                uart_send_debug_msg("ERROR: Msg Bad Len");
                break;
            case UART_RECV_ERROR_BAD_CHECKSUM:
                uart_send_debug_msg("ERROR: Msg Bad Checksum");
                break;
            default:
                uart_send_debug_msg_with_error_code("ERROR: Failed to receive frame", result);
                break;
            }
            continue;
        }

        STATUS_LED_OFF();

        uart_send_debug_msg("Frame successfully received.");


        /* Route by Message ID */
        switch (rx_frame.msg_id) {
<<<<<<< Updated upstream
        case MSG_SESSION_OPEN:
            //handle_session_open(&rx_frame);
            uart_send_debug_msg("Session Open message received.");
            break;
        case MSG_KEY_EXCHANGE:
            //TODO: Decrypt payload (if necessary)
            //handle_key_exchange(&rx_frame);
            break;
        case MSG_PIN_EXCHANGE:
            //TODO: Decrypt payload (if necessary)
            //handle_pin_exchange(&rx_frame);
            break;  
        case MSG_SESSION_CLOSE:
            //handle_session_close(&rx_frame);
            break;
        case MSG_FILE_TRANSFER_REQUEST:
            //handle_file_transfer_request(&rx_frame);
            break;
        case MSG_FILE_CONTENTS:
            //TODO: Decrypt payload (if necessary)
            //handle_file_contents(&rx_frame);
            break;
        case MSG_FILE_TRANSFER_COMPLETE:
            //handle_file_transfer_complete(&rx_frame);
            break;
        case MSG_FILE_REQUEST_ACK:
            //handle_file_request_ack(&rx_frame);
            break;
        case MSG_FILE_TRANSFER_COMPLETE_ACK:
            //handle_file_transfer_complet_ack(&rx_frame);
            break;
        case MSG_PIN_EXCHANGE_ACK:
            //handle_pin_exchange_ack(&rx_frame);
            break;
        default:
            //uart_send_debug_msg("Unknown Message ID\n");
            break;
=======
            case MSG_SESSION_OPEN:
                //handle_session_open(&rx_frame);
                uart_send_debug_msg("Session Open message received.");
                break;
            case MSG_KEY_EXCHANGE:
                //TODO: Decrypt payload (if necessary)
                //handle_key_exchange(&rx_frame);
                break;
            case MSG_PIN_EXCHANGE:
                //TODO: Decrypt payload (if necessary)
                //handle_pin_exchange(&rx_frame);
                break;  
            case MSG_SESSION_CLOSE:
                //handle_session_close(&rx_frame);
                break;
            case MSG_FILE_TRANSFER_REQUEST:
                handle_file_transfer_request(&rx_frame);
                break;
            case MSG_FILE_CONTENTS:
                //TODO: Decrypt payload (if necessary)
                //handle_file_contents(&rx_frame);
                break;
            case MSG_FILE_TRANSFER_COMPLETE:
                //handle_file_transfer_complete(&rx_frame);
                break;
            case MSG_FILE_REQUEST_ACK:
                //handle_file_request_ack(&rx_frame);
                break;
            case MSG_FILE_TRANSFER_COMPLETE_ACK:
                //handle_file_transfer_complet_ack(&rx_frame);
                break;
            case MSG_PIN_EXCHANGE_ACK:
                //handle_pin_exchange_ack(&rx_frame);
                break;
            default:
                //uart_send_debug_msg("Unknown Message ID\n");
                break;
>>>>>>> Stashed changes
        }
    }
}