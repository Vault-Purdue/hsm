/**
 * @file uart_protocol.h
 * @author Vault Team - Purdue
 * @brief UART Protocol Definitions
 * @date 2026
 *
 * Frame structure, message IDs, and error codes for HSM-Host UART communication
 */

#include <stdint.h>

#define UART_MAX_PAYLOAD_LEN  88U

typedef struct {
    uint8_t  sof;
    uint8_t  msg_id;
    uint8_t  payload_len;// 0-88
    uint8_t  payload[UART_MAX_PAYLOAD_LEN]; // encrypted or plaintext
    uint16_t checksum;
} __attribute__((packed)) uart_frame_t;

// TODO: Add description to this function
void uart_init(void);
