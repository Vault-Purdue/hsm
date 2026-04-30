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
    byte rcv_pin_hash[MAX_DIGEST_SIZE];
    byte salt[SALT_SIZE] = {
        0xDB, 0x3C, 0x85, 0x59, 0x93, 0x24, 0x47, 0x65,
        0xA8, 0xFE, 0xA4, 0xFF, 0x91, 0xBA, 0xA6, 0xCA,
        0xDC, 0x0A, 0xD5, 0xF8, 0x61, 0x46, 0x46, 0x25,
        0xAC, 0x4F, 0x4B, 0x48, 0x9D, 0x7E, 0x17, 0x26
    };
    const byte info[] = "host-cli-pin-payload";

    const byte exp_pin_hash[MAX_DIGEST_SIZE] = {
        0x3D, 0xC1, 0x71, 0x7E, 0x8E, 0xD8, 0x20, 0x92, 
        0x1E, 0xE4, 0x79, 0x42, 0x32, 0xEB, 0x11, 0xD6, 
        0x1C, 0x2E, 0x5C, 0xE7, 0xDE, 0xCC, 0xF7, 0xC5, 
        0xD6, 0x99, 0xC8, 0x2B, 0xE2, 0x4F, 0x8B, 0x79
    };

    // Check the system state and do nothing if not in STATE_WAIT_FOR_PIN
    state = system_state_machine(EVENT_NONE);
    if (state != STATE_WAIT_FOR_PIN) {
        //__BKPT();
        uart_payload[0] = 1;
        uart_send_frame(MSG_PIN_EXCHANGE_ACK, uart_payload, 1);
        return;
    }

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