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

    while (1) {
        //print_debug("Ready\n");
        STATUS_LED_ON();

        // TODO: Block until a full frame is received
        //result = uart_receive_frame(&rx_frame);

        if (result != 1) {
            STATUS_LED_OFF();
            switch (result) { // TODO: The ERROR MSGS are currently missing
            case 1://MSG_BAD_SOF:
                //print_debug("Bad SoF\n");
                break;
            case 2: //MSG_BAD_LEN:
                //print_debug("Bad payload length\n");
                break;
            case 3://MSG_BAD_CRC:
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
    }
}