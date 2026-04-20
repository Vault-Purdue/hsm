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
#include "trng.h"
#include "uart_protocol.h"
#include "uart_cmd_router.h"
#include "led_status.h"
#include "aes_adv_gcm_test.h"
#include "aes_adv_gcm.h"

/** @brief Initializes peripherals for system boot.
*/
void init() {
    // Initialize all of the hardware components
    SYS_initPower();
    // TODO: Initialize file manager
    // init_fm();
}

/************************MAIN LOOP ************************/

int main(void) {
    uart_frame_t rx_frame;
    int result = 0;
    //uart_msg_id_t cmd;

    // Initialize device peripherals
    init();
    AESADV_init();
    uart_init();

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

        /*
        uint8_t *debug_write = (uint8_t *)"Test message 1";
        int debug_write_size = 14;
        uart_send_debug_msg(debug_write, debug_write_size);
        */

        // Block until a full frame is received
        while (result == UART_RECV_NO_ERROR) {
            result = uart_receive_frame(&rx_frame);
            //uart_send_debug_msg((uint8_t *)"ABCDEFGHIJ", 10);
            uart_send_frame(0, (uint8_t *)"Test Message", 12);
        }

        uart_send_frame(0, (uint8_t *)"Another.", 8);

        /*
        STATUS_LED_OFF();
        debug_write = (uint8_t *)"Test message 2";
        uart_send_debug_msg(debug_write, debug_write_size);
        */
        
        if (result != UART_RECV_FULL_FRAME_RECEIVED) {
            STATUS_LED_OFF();
            switch (result) { // TODO: The ERROR MSGS are currently missing
            case UART_RECV_ERROR_BAD_SOF://MSG_BAD_SOF:
                //print_debug("Bad SoF\n");
                break;
            case UART_RECV_ERROR_PAYLOAD_TOO_LONG: //MSG_BAD_LEN:
                //print_debug("Bad payload length\n");
                break;
            case UART_RECV_ERROR_BAD_CHECKSUM: //MSG_BAD_CRC:
                //print_debug("CRC mismatch\n");
                break;
            default:
                //print_debug("Failed to receive frame\n");
                break;
            }
            continue;
        }

        STATUS_LED_OFF();

        /* Route by Message ID */
        switch (rx_frame.msg_id) {
        case MSG_SESSION_OPEN:
            //handle_session_open(&rx_frame);
            break;
        case MSG_KEY_EXCHANGE:
            //handle_key_exchange(&rx_frame);
            break;
        case MSG_PIN_EXCHANGE:
            //handle_pin_exchange(&rx_frame);
            break;
        case MSG_SESSION_CLOSE:
            //handle_session_close(&rx_frame);
            break;
        case MSG_STATUS_QUERY:
            //handle_status_query(&rx_frame);
            break;
        case MSG_FILE_TRANSFER_REQUEST:
            //handle_file_transfer_request(&rx_frame);
            break;
        case MSG_FILE_START:
        case MSG_FILE_BLOCK:
        case MSG_FILE_END:
            //handle_file_block(&rx_frame);
            break;
        case MSG_FILE_TRANSFER_COMPLETE:
            //handle_file_transfer_complete(&rx_frame);
            break;
        default:
            //print_debug("Unknown Message ID\n");
            break;
        }

        //For testing: echo back the same message, with payload incremented
        for (int i = 0; i < rx_frame.payload_len; i++) {
            rx_frame.payload[i]++;
        }
        uart_send_frame(0, rx_frame.payload, rx_frame.payload_len);
    }
}