/**
 * @file uart_cmd_router.h
 * @author Vault Team - Purdue
 * @brief router interface functions
 * @date 2026
 *
 * Wrapper functions for...
 */

#ifndef __CMD_ROUTER__
#define __CMD_ROUTER__

#include "ti_drivers_config.h"
#include <string.h>
#include <stdint.h>

// See hsm/assets/docs/uart_protocol.md for list of message types.
typedef enum {
    MSG_SESSION_OPEN               = 0x00,
    MSG_KEY_EXCHANGE               = 0x01,
    MSG_PIN_EXCHANGE               = 0x02,      
    MSG_SESSION_CLOSE              = 0x0F,

    MSG_STATUS_QUERY               = 0x10,
    MSG_STATUS_RESPONSE            = 0x11,

    MSG_FILE_TRANSFER_REQUEST      = 0x20,
    MSG_FILE_START                 = 0x21,
    MSG_FILE_BLOCK                 = 0x22,
    MSG_FILE_END                   = 0x23,
    MSG_FILE_TRANSFER_COMPLETE     = 0x24,

    MSG_FILE_REQUEST_ACK           = 0xF0,
    MSG_FILE_BLOCK_ACK             = 0xF1,
    MSG_FILE_TRANSFER_COMPLETE_ACK = 0xF2,
    MSG_OK                         = 0xF3, // Not present in uart_protocol.md?

    MSG_DEBUG                      = 0xFF
} uart_msg_id_t; // TODO: Change the names as needed

typedef enum direction {
    EAST, NORTH, WEST, SOUTH
}host_message_t;

#endif