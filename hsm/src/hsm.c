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
#include "lcd.h"

/* Convenience: clear LCD then print 6-char string. Without the clear,
 * residual segments from previous prints stay lit. */
static void lcd_status(const char *s) {
#if LCD_ENABLE
    DL_LCD_clearAllMemoryRegs(LCD);
    LCD_init();
    LCD_print((char *)s);
#else
    (void)s;  /* unused when LCD disabled */
#endif
}

/** @brief Initializes peripherals for system boot. */
void init() {
    SYS_initPower();
    GPIO_init();
#if LCD_ENABLE
    LCD_init();
#endif
    uart_init();
    HSM_CRYPTO_init();
    init_fm();
}

int main(void) {
    uart_frame_t rx_frame;

    init();    
    STATUS_LED_ON();

    lcd_status("INITOK");
    delay_cycles(16000000);  /* ~500ms */

#if CRYPTO_TEST
    /* SKIPPED: session test (ECDH) fails post-INITDONE due to the chip's
     * security model */
    // if (!HSM_CRYPTOTEST_sessionTest()) {
    //     lcd_status("FAIL_1");
    //     while (1) {
    //         STATUS_LED_ON();  delay_cycles(2000000);
    //         STATUS_LED_OFF(); delay_cycles(16000000);
    //     }
    // }

    lcd_status("TST2__");
    delay_cycles(8000000);
    if (!HSM_CRYPTOTEST_fileKeyEncryptionTest()) {
        lcd_status("FAIL_2");
        while (1) {
            STATUS_LED_ON();  delay_cycles(8000000);
            STATUS_LED_OFF(); delay_cycles(8000000);
            STATUS_LED_ON();  delay_cycles(8000000);
            STATUS_LED_OFF(); delay_cycles(48000000);
        }
    }

    lcd_status("TST3__");
    delay_cycles(8000000);
    if (!HSM_CRYPTOTEST_messagePayloadEncryptionTest()) {
        lcd_status("FAIL_3");
        while (1) {
            STATUS_LED_ON();  delay_cycles(8000000);
            STATUS_LED_OFF(); delay_cycles(8000000);
            STATUS_LED_ON();  delay_cycles(8000000);
            STATUS_LED_OFF(); delay_cycles(8000000);
            STATUS_LED_ON();  delay_cycles(8000000);
            STATUS_LED_OFF(); delay_cycles(48000000);
        }
    }

    lcd_status("TST4__");
    delay_cycles(8000000);
    if (AESADV_GCM_selfTest()) {
        /* All passed: show PASSED for ~3 seconds, then switch to HSM
         * permanent display, with heartbeat LED. */
        lcd_status("PASSED");
        delay_cycles(96000000);   /* ~3 seconds */
        lcd_status("__HSM_");

        while (1) {
            STATUS_LED_ON();  delay_cycles(16000000);
            STATUS_LED_OFF(); delay_cycles(16000000);
        }
    } else {
        lcd_status("FAIL_4");
        while (1) {
            STATUS_LED_ON();  delay_cycles(2000000);
            STATUS_LED_OFF(); delay_cycles(2000000);
        }
    }
#endif

    while (1) {
        int result = uart_receive_frame(&rx_frame);
        if (result != UART_RECV_FULL_FRAME_RECEIVED) {
            handle_uart_error(result);
            continue;
        }
        STATUS_LED_ON();
        router_status_t status = router_dispatch(&rx_frame);
        if (status == RT_FAIL) {
            uart_send_debug_msg_with_str("ERROR: Router dispatch failed for msg_id", msg_id_to_str(rx_frame.msg_id));
        }
    }
}
