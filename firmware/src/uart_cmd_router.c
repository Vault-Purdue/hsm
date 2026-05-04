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

/************************* GLOBALS ************************/

static uint8_t pubECDH[CRYPTO_AES_KEY_SIZE] = {0};
static uint8_t sessionKey[CRYPTO_AES_KEY_SIZE] = {0};
static uint8_t sessionIv[CRYPTO_GCM_IV_SIZE] = {0};
static bool sessionEstablished = false;

/************************ FUNCTIONS ***********************/

/**
 * @brief Verifies that a session has been established.
 * 
 * @param rx_frame Pointer to a received UART frame from the Host CLI.
 *
 * @returns true if valid session has been established
 */
bool HSM_UART_ROUTER_validSessionEstablished(uart_frame_t *rx_frame) {

    // Null check (shouldn't happen, but just in case)
    if (!rx_frame) return false;

    // If not session open / close, check against sessionEstablished
    if (
        rx_frame->msg_id == MSG_SESSION_OPEN
        || rx_frame->msg_id == MSG_KEY_EXCHANGE
        || rx_frame->msg_id == MSG_SESSION_CLOSE
    ) return true;
    return sessionEstablished;
}

/**
 * @brief Clears the session data.
 */
void HSM_UART_ROUTER_clearSessionData(void) {

    // Zeroize all buffers related to session
    memset(pubECDH, 0, CRYPTO_AES_KEY_SIZE);
    memset(sessionKey, 0, CRYPTO_AES_KEY_SIZE);
    memset(sessionIv, 0, CRYPTO_GCM_IV_SIZE);

    // Indicates valid session has not been established
    sessionEstablished = false;
}

static int router_decrypt_payload(const uint8_t *payload, size_t payload_len, uint8_t *pt_out, 
                                  size_t *pt_len_out) {
    if(payload_len <= CRYPTO_GCM_TAG_SIZE) return -1;
    size_t ct_len = payload_len - CRYPTO_GCM_TAG_SIZE;
    const uint8_t *auth_tag = payload;
    const uint8_t *ciphertext = payload + CRYPTO_GCM_TAG_SIZE;
    if(HSM_CRYPTO_decryptCommandPayload(
        sessionKey, CRYPTO_AES_KEY_SIZE,
        sessionIv, CRYPTO_GCM_IV_SIZE,
        (uint8_t *) auth_tag, CRYPTO_GCM_TAG_SIZE,
        (uint8_t *) ciphertext, pt_out, ct_len) != HSM_CRYPTO_OK) return -1;
    *pt_len_out = ct_len;
    return 0;
}

static int router_encrypt_payload(const uint8_t *pt, size_t pt_len, uint8_t *payload_out, 
                                  size_t *payload_len_out) {
    uint8_t *auth_tag = payload_out;
    uint8_t *ciphertext = payload_out + CRYPTO_GCM_TAG_SIZE;
    if(HSM_CRYPTO_encryptCommandPayload(
        sessionKey, CRYPTO_AES_KEY_SIZE,
        sessionIv, CRYPTO_GCM_IV_SIZE,
        auth_tag, CRYPTO_GCM_TAG_SIZE,
        (uint8_t *)pt, ciphertext, pt_len) != HSM_CRYPTO_OK) return -1;
    *payload_len_out = pt_len + CRYPTO_GCM_TAG_SIZE;
    return 0;
}

router_status_t router_send_encrypted_frame(uint8_t msg_id, const uint8_t *pt, size_t pt_len) {
    uint8_t enc[pt_len + CRYPTO_GCM_TAG_SIZE];
    size_t enc_len = 0;
    if(router_encrypt_payload(pt, pt_len, enc, &enc_len) != 0) return RT_FAIL;
    (void) uart_send_frame(msg_id, enc, (uint16_t) enc_len);
    return RT_OK;
}

router_status_t router_dispatch(uart_frame_t *rx_frame) {
    SystemState sys_state;
    uint8_t ack_payload = 0;
    uint8_t err_ack_payload = 1;
    if (rx_frame == NULL) {
        uart_send_debug_msg("ERROR: Null frame in router");
        return RT_FAIL;
    }

    // Session check
    if (!HSM_UART_ROUTER_validSessionEstablished(rx_frame)) {
        uart_send_debug_msg("ERROR: invalid session");
        return RT_FAIL;
    }
    
    //__BKPT();
    sys_state = system_state_machine(EVENT_NONE);
    switch (rx_frame->msg_id) {

        case MSG_SESSION_OPEN:
            uart_send_debug_msg("Session Open message received.");

            // State machine check
            if (sys_state != STATE_WAIT_FOR_UART) {
                HSM_UART_ROUTER_clearSessionData();
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Validate syntax according to UART frame ICD
            if (rx_frame->payload_len != 1 || rx_frame->payload[0] != 0x41) {
                HSM_UART_ROUTER_clearSessionData();
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Clear all session data
            HSM_UART_ROUTER_clearSessionData();
            
            // Generate private key
            uint8_t privECDH[CRYPTO_AES_KEY_SIZE];
            memset(privECDH, 0, CRYPTO_AES_KEY_SIZE);
            if (HSM_CRYPTO_generatePrivateSessionKey(privECDH, CRYPTO_AES_KEY_SIZE) != HSM_CRYPTO_OK) {
                memset(privECDH, 0, CRYPTO_AES_KEY_SIZE);
                HSM_UART_ROUTER_clearSessionData();
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Generate public key
            if (HSM_CRYPTO_generatePublicSessionKey(privECDH, pubECDH, CRYPTO_AES_KEY_SIZE) != HSM_CRYPTO_OK) {
                memset(privECDH, 0, CRYPTO_AES_KEY_SIZE);
                HSM_UART_ROUTER_clearSessionData();
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Send acknoledgement
            memset(privECDH, 0, CRYPTO_AES_KEY_SIZE);
            
            return RT_OK;

        case MSG_KEY_EXCHANGE:
            uart_send_debug_msg("Key Exchange message received.");

            // State machine check
            if (sys_state != STATE_WAIT_FOR_UART) {
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Validate syntax according to UART frame ICD
            if (rx_frame->payload_len != CRYPTO_AES_KEY_SIZE) {
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }
    
            // Load client public key into session
            if (HSM_CRYPTO_loadClientPublicKey(rx_frame->payload, rx_frame->payload_len) != HSM_CRYPTO_OK) {
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Generate shared secret
            uint8_t secret[CRYPTO_AES_KEY_SIZE];
            memset(secret, 0, CRYPTO_AES_KEY_SIZE);
            if (HSM_CRYPTO_generateSharedSecret(secret, CRYPTO_AES_KEY_SIZE) != HSM_CRYPTO_OK) {
                memset(secret, 0, CRYPTO_AES_KEY_SIZE);
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Derive session key and IV
            uint8_t tempKey[CRYPTO_AES_KEY_SIZE];
            uint8_t tempIv[CRYPTO_GCM_IV_SIZE];
            if (HSM_CRYPTO_deriveKeyAndIV(secret, tempKey, CRYPTO_AES_KEY_SIZE, tempIv, CRYPTO_GCM_IV_SIZE) != HSM_CRYPTO_OK) {
                memset(tempKey, 0, CRYPTO_AES_KEY_SIZE);
                memset(tempIv, 0, CRYPTO_GCM_IV_SIZE);
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Load generated values into session variables
            memset(sessionKey, 0, CRYPTO_AES_KEY_SIZE);
            memset(sessionIv, 0, CRYPTO_GCM_IV_SIZE);
            memcpy(sessionKey, tempKey, CRYPTO_AES_KEY_SIZE);
            memcpy(sessionIv, tempIv, CRYPTO_GCM_IV_SIZE);
            memset(tempKey, 0, CRYPTO_AES_KEY_SIZE);
            memset(tempIv, 0, CRYPTO_GCM_IV_SIZE);
            sessionEstablished = true;
            
            // Update state machine
            sys_state = system_state_machine(EVENT_UART_SESSION_ESTABLISHED);
            
            // Send public key on success
            (void) uart_send_frame(rx_frame->msg_id, pubECDH, CRYPTO_AES_KEY_SIZE);
            
            return RT_OK;

        case MSG_PIN_EXCHANGE: {
            uart_send_debug_msg("PIN Exchange message received.");
            
            // State machine check
            if (sys_state != STATE_WAIT_FOR_PIN) {
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // Validate syntax according to UART frame ICD
            if (rx_frame->payload_len <= CRYPTO_GCM_TAG_SIZE) {
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // decrypt payload into pin
            uint8_t pin[rx_frame->payload_len];
            size_t pin_len = 0;
            memset(pin, 0, sizeof(pin));
            if(router_decrypt_payload(rx_frame->payload, rx_frame->payload_len, pin, &pin_len) != 0) {
                (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);
                return RT_FAIL;
            }

            // authenticate and send ack
            uint8_t ack_pt = (authentication_engine(pin, pin_len) == AE_OK) ? 0 : 1;
            router_send_encrypted_frame(MSG_PIN_EXCHANGE_ACK, &ack_pt, 1);

            return RT_OK;
        }

        case MSG_SESSION_CLOSE:
            uart_send_debug_msg("Session Close message received.");
            
            // Clear all session data
            HSM_UART_ROUTER_clearSessionData();

            // Update state machine
            sys_state = system_state_machine(EVENT_SESSION_CLOSE_USER);

            // Send acknowledgement
            (void) uart_send_frame(rx_frame->msg_id, &err_ack_payload, 1);

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
    if(frame->payload_len > FM_MAX_PAYLOAD_SIZE + CRYPTO_GCM_TAG_SIZE) return RT_FAIL;

    // decrypt payload into plaintext
    uint8_t pt[FM_MAX_PAYLOAD_SIZE];
    size_t pt_len = 0;
    if(router_decrypt_payload(frame->payload, frame->payload_len, pt, &pt_len) != 0) {
        return RT_FAIL;
    }
    return fm_handle_file_contents(pt, (uint8_t) pt_len);
}