/**
 * @file uart_cmd_router.h
 * @author Vault Team - Purdue
 * @brief Router interface
 * @date 2026
 */

#ifndef UART_CMD_ROUTER_H
#define UART_CMD_ROUTER_H

#include "ti_drivers_config.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart_protocol.h"
#include "crypto_module.h"

// See hsm/assets/docs/uart_protocol.md for list of message types.
typedef enum {
    MSG_SESSION_OPEN               = 0x01,
    MSG_KEY_EXCHANGE               = 0x02,
    MSG_PIN_EXCHANGE               = 0x03,
    MSG_SESSION_CLOSE              = 0x0F,
    MSG_FILE_TRANSFER_REQUEST      = 0x20,
    MSG_FILE_CONTENTS              = 0x21,
    MSG_FILE_REQUEST_ACK           = 0xF0,
    MSG_FILE_TRANSFER_COMPLETE_ACK = 0xF1,
    MSG_PIN_EXCHANGE_ACK           = 0xF2,
    MSG_DEBUG                      = 0xFF
} uart_msg_id_t;

const char* msg_id_to_str(uint8_t msg_id);

typedef enum {
    RT_OK,
    RT_FAIL, // routing/parsing failure
    RT_REJECTED // logical operation rejected
} router_status_t;

router_status_t router_dispatch(uart_frame_t *rx_frame);
router_status_t handle_file_transfer_request(uart_frame_t *frame);
router_status_t handle_file_contents(uart_frame_t *frame);
router_status_t router_send_encrypted_frame(uint8_t msg_id, const uint8_t *pt, size_t pt_len);

#endif /* UART_CMD_ROUTER_H */