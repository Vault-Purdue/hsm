/**
 * @file flash.c
 * @author Vault Team - Purdue
 * @brief flash interface functions
 * @date 2026
 *
 * Wrapper functions for reading from/writing to flash memory. 
 */

#include "ti_drivers_config.h"
#include <string.h>
#include <stdint.h>
#include "uart_protocol.h"
#include "uart_cmd_router.h" // For message ID types

/************************ VARIABLES ***********************/
size_t rdCount;
size_t wrCount;

typedef enum {
    WAIT_START,
    READ_MSGID,
    READ_LENGTH,
    READ_PAYLOAD,
    READ_CHECKSUM_H,
    READ_CHECKSUM_L
} UartReceiverState;

#define MAX_PAYLOAD 128
#define START_BYTE 0xAA

UartReceiverState parserState = WAIT_START;
uint8_t receiverIndex = 0;
UART_Handle uartHandle;

uint8_t gBuffer[CONFIG_UART_BUFFER_LENGTH] = {0};

/************************ FUNCTIONS ***********************/

/**
 * @brief Initializes the UART module
 * 
 * This function powers on the UART module and
 * prepares it for receiving and sending UART messages.
 */
void uart_init(void) {
    rdCount = 0;
    wrCount = 0;
    UART_Params params;

    UART_Params_init(&params);
    // Modify UART params
    params.baudRate = 115200;
    params.readMode = UART_Mode_BLOCKING;
    params.readReturnMode = UART_ReadReturnMode_PARTIAL;
    uartHandle = UART_open(CONFIG_UART_0, &params);
    if (uartHandle == NULL) {
        // UART_open failed
        __BKPT();
    }
}

/**
 * @brief UART Helper function for processing received bytes.
 * 
 * Uses a state machine to track where in the UART frame we're
 * at. Receives one byte at a time for simplicity/safety.
 */
int Uart_Process_Byte(uint8_t byte, uart_frame_t *rx_frame) {
    //printf("Byte received: %02X\n", byte);
    switch(parserState) {
        case WAIT_START:
            if (byte == START_BYTE) {
                rx_frame->sof = byte;
                parserState = READ_MSGID;
                break;
            }
            else {
                return UART_RECV_ERROR_BAD_SOF;
            }
        case READ_MSGID:
            rx_frame->msg_id = byte;
            parserState = READ_LENGTH;
            break;
        case READ_LENGTH:
            rx_frame->payload_len = byte;
            if (rx_frame->payload_len > MAX_PAYLOAD) {
                parserState = WAIT_START;
                receiverIndex = 0;
                return UART_RECV_ERROR_PAYLOAD_TOO_LONG;
            }
            receiverIndex = 0;
            parserState = READ_PAYLOAD;
            break;
        case READ_PAYLOAD:
            rx_frame->payload[receiverIndex++] = byte;
            if (receiverIndex >= rx_frame->payload_len) {
                parserState = READ_CHECKSUM_H;
            }
            break;
        case READ_CHECKSUM_H:
            rx_frame->checksum = byte << 8;
            parserState = READ_CHECKSUM_L;
            break;
        case READ_CHECKSUM_L:
            rx_frame->checksum += byte;
            
            // Full frame received!
            // TODO: Actually calculate checksum for confirmation
            uint16_t calculated_checksum = rx_frame->checksum;
            if (rx_frame->checksum != calculated_checksum) {
                parserState = WAIT_START;
                return UART_RECV_ERROR_BAD_CHECKSUM;
            }
            parserState = WAIT_START;
            return UART_RECV_FULL_FRAME_RECEIVED;
    }
    return UART_RECV_NO_ERROR;
}

/**
 * @brief Receives a UART frame.
 * 
 * Blocks until a whole frame is received successfully, or
 * until an error occurrs (such as incorrect formatting of
 * frame).
 */
int uart_receive_frame(uart_frame_t *rx_frame) {
    // Read from UART stream one byte at a time. Assemble message as bytes come.
    int returnValue;
    while (1) {
        UART_read(uartHandle, gBuffer, CONFIG_UART_BUFFER_LENGTH, &rdCount);
        for (uint32_t i = 0; i < rdCount; i++) {
            returnValue = Uart_Process_Byte(gBuffer[i], rx_frame);
            if (returnValue != UART_RECV_NO_ERROR) {
                // Either we received the whole frame, or we got an error.
                // Todo: mark remainder of gBuffer to be prepended next time. (unneccessary, because we're only reading 1 byte at a time?)                
                return returnValue;
            }
        }
    }
}

void handle_uart_error(int err_code) {
    switch (err_code) {
        case UART_RECV_ERROR_BAD_SOF:
            uart_send_debug_msg("ERROR: Msg Bad SOF");
            break;
        case UART_RECV_ERROR_PAYLOAD_TOO_LONG:
            uart_send_debug_msg("ERROR: Msg Bad Len");
            break;
        case UART_RECV_ERROR_BAD_CHECKSUM:
            uart_send_debug_msg("ERROR: Msg Bad Checksum");
            break;
        default:
            uart_send_debug_msg_with_error_code("ERROR: Failed to receive frame", err_code);
            break;
    }
}

/**
 * @brief Send a UART frame with a given message ID and payload.
 * 
 * Assembles a UART frame for the given payload and sends it.
 */
int uart_send_frame(uint8_t msg_id, uint8_t *payload, uint16_t payload_len) {
    //TODO: Make a way for users to encrypt their payload BEFORE it is passed to this function.
    if (payload_len > UART_MAX_PAYLOAD_LEN) {
        return UART_SEND_ERROR_MSG_TOO_LONG;
    }

    uint8_t header[3];
    header[0] = START_BYTE;
    // TODO: add checking to confirm msg_id is valid
    header[1] = msg_id;
    header[2] = payload_len;

    // TODO: generate checksum
    uint16_t checksum = 0xFFFF;
    uint8_t swapped_checksum[2];
    // Swapping checksum length bits for proper endianness
    swapped_checksum[0] = (uint8_t)(checksum >> 8) & 0xFF;
    swapped_checksum[1] = (uint8_t)(checksum & 0xFF);

    // Send write frame
    uint8_t send_buffer[5 + payload_len];
    memcpy(&send_buffer[0], header, 3);
    memcpy(&send_buffer[3], payload, payload_len);
    memcpy(&send_buffer[3+payload_len], swapped_checksum, 2);
    UART_write(uartHandle, send_buffer, 5+payload_len, &wrCount);

    return UART_SEND_SUCCESS;
}

/**
 * @brief Send debug message over UART, with a string message and an int error code.
 * 
 * The other uart_send_debug_msg() function only takes a str as input. I added this function
 * for cases where you want to add an error code or other integer to be inserted into
 * your debug message.
 */
void uart_send_debug_msg_with_error_code(const char *message, int errorCode) {
    //TODO: Currently only accepts debug messages under 64 characters
    char final_string[64];

    // Calculate length of message
    // Format of string: "Your Message [errorCode]"
    int formatted_len = snprintf(final_string, sizeof(final_string), "%s [%d]", message, errorCode);
    if (formatted_len > 0) {
        uint16_t send_len = (uint16_t)formatted_len;
        
        // Cap at protocol's max payload size
        if (send_len > UART_MAX_PAYLOAD_LEN) {
            send_len = UART_MAX_PAYLOAD_LEN;
        }

        // MSG ID 0xFF for Debug Message
        uart_send_frame(MSG_DEBUG, (uint8_t *)final_string, send_len);
    }
}

void uart_send_debug_msg_with_str(const char *message, const char *value) {
    char final_string[64];

    int formatted_len = snprintf(final_string, sizeof(final_string),
                                 "%s [%s]", message, value);
    if (formatted_len > 0) {
        uint16_t send_len = (uint16_t)formatted_len;
        if (send_len > UART_MAX_PAYLOAD_LEN) {
            send_len = UART_MAX_PAYLOAD_LEN;
        }
        uart_send_frame(MSG_DEBUG, (uint8_t *)final_string, send_len);
    }
}

/**
 * @brief Send debug message over UART, with a string message.
 * 
 * If you also want to send an integer error code, use 
 * uart_send_debug_msg_with_error_code().
 */
void uart_send_debug_msg(const char *message) {
    //TODO: Currently only accepts debug messages under 64 characters
    char final_string[64];

    // Calculate length of message
    int formatted_len = snprintf(final_string, sizeof(final_string), "%s", message);
    if (formatted_len > 0) {
        uint16_t send_len = (uint16_t)formatted_len;
        
        // Cap at protocol's max payload size
        if (send_len > UART_MAX_PAYLOAD_LEN) {
            send_len = UART_MAX_PAYLOAD_LEN;
        }

        // MSG ID 0xFF for Debug Message
        uart_send_frame(MSG_DEBUG, (uint8_t *)final_string, send_len);
    }
}