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
    uart_init();
    // Initialize crypto module
    HSM_CRYPTO_init();
    init_fm();
}

/************************MAIN LOOP ************************/
int main(void) {
    uart_frame_t rx_frame;
    //uart_msg_id_t cmd;

    // Initialize device peripherals
    init();

#if CRYPTO_TEST
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
#endif
    while (1) {
        int result = uart_receive_frame(&rx_frame);
        if (result != UART_RECV_FULL_FRAME_RECEIVED) {
            handle_uart_error(result);
            continue;
        }

        STATUS_LED_OFF();

        router_status_t status = router_dispatch(&rx_frame);
        if (status == RT_FAIL) {
            uart_send_debug_msg_with_str("ERROR: Router dispatch failed for msg_id", msg_id_to_str(rx_frame.msg_id));
        }
    }
}