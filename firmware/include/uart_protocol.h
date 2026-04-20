/**
 * @file uart_protocol.h
 * @author Vault Team - Purdue
 * @brief UART Protocol Definitions
 * @date 2026
 *
 * Frame structure, message IDs, and error codes for HSM-Host UART communication
 */

#include <stdint.h>

#define UART_MAX_PAYLOAD_LEN  1024

#define UART_RECV_NO_ERROR 0
#define UART_RECV_FULL_FRAME_RECEIVED 1
#define UART_RECV_ERROR_PAYLOAD_TOO_LONG 2
#define UART_RECV_ERROR_BAD_SOF 3
#define UART_RECV_ERROR_BAD_CHECKSUM 4

#define UART_SEND_NO_ERROR 0
#define UART_SEND_ERROR_MSG_TOO_LONG 1

typedef struct {
    uint8_t  sof;
    uint8_t  msg_id;
    uint16_t  payload_len; // 0-1024
    uint8_t  payload[UART_MAX_PAYLOAD_LEN]; // encrypted or plaintext
    uint16_t checksum;
} __attribute__((packed)) uart_frame_t;

void uart_init(void);

int uart_receive_frame(uart_frame_t *rx_frame);

int uart_send_frame(uint8_t msg_id, uint8_t *writeBuffer, uint16_t write_length);

void uart_send_debug_msg_with_error_code(const char *message, int errorCode);

void uart_send_debug_msg(const char *message);