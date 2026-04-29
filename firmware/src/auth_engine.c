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

#include <string.h>
#include "state_machine.h"
#include "uart_protocol.h"
#include "file_manager.h"
#include "wolfssl/wolfcrypt/hmac.h"
#include "uart_cmd_router.h"

#define SALT_SIZE 32

void authentication_engine(uart_frame_t *rx_frame) {
    // Initialize Variables
    SystemState state;
    uint8_t uart_payload[1] = {0};
    int exp_res = 0, rcv_res = 0;
    byte exp_pin_hash[MAX_DIGEST_SIZE];
    byte rcv_pin_hash[MAX_DIGEST_SIZE];
    byte salt[SALT_SIZE] = {
        0xDB, 0x3C, 0x85, 0x59, 0x93, 0x24, 0x47, 0x65,
        0xA8, 0xFE, 0xA4, 0xFF, 0x91, 0xBA, 0xA6, 0xCA,
        0xDC, 0x0A, 0xD5, 0xF8, 0x61, 0x46, 0x46, 0x25,
        0xAC, 0x4F, 0x4B, 0x48, 0x9D, 0x7E, 0x17, 0x26
    };
    const byte info[] = "host-cli-pin-payload";

    // Check the system state and do nothing if not in STATE_LOCKED
    state = system_state_machine(EVENT_NONE);
    if (state != STATE_LOCKED) {
        //__BKPT();
        uart_payload[0] = 1;
        uart_send_frame(MSG_PIN_EXCHANGE_ACK, uart_payload, 1);
        return;
    }

    // Get Expected PIN
    fm_read_pin(exp_pin_hash, sizeof(exp_pin_hash));

    // Compute hashes
    //exp_res = wc_HKDF(
    //    WC_SHA256,
    //    exp_pin,
    //    sizeof(exp_pin),
    //    salt,
    //    sizeof(salt),
    //    NULL,
    //    0,
    //    exp_pin_hash,
    //    sizeof(exp_pin_hash)
    //);
    rcv_res = wc_HKDF(
        WC_SHA256,
        rx_frame->payload,
        (word32)rx_frame->payload_len,
        salt,
        sizeof(salt),
        info,
        sizeof(info)-1,
        rcv_pin_hash,
        sizeof(rcv_pin_hash)
    );
    
    if (exp_res != 0 || rcv_res != 0) {
        uart_send_debug_msg("KDF Error");
        uart_payload[0] = 1;
        uart_send_frame(MSG_PIN_EXCHANGE_ACK, uart_payload, 1);
        return;
    }
    
    // Compare the expected and received PINs    
    if (memcmp(exp_pin_hash, rcv_pin_hash, MAX_DIGEST_SIZE) == 0) {
        // PINs are identical
        system_state_machine(EVENT_USER_AUTHENTICATED);
        uart_payload[0] = 0;
        // uart_send_debug_msg("PINs Match");
    } else {
        // PINs do not match
        system_state_machine(EVENT_INVALID_PIN);
        uart_payload[0] = 1;    
        //uart_send_debug_msg("PINs Do Not Match");
    }
    uart_send_frame(MSG_PIN_EXCHANGE_ACK, uart_payload, 1);
}