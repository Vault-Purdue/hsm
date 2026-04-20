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

/************************ VARIABLES ***********************/
size_t rdCount;
size_t wrCount;

typedef enum {
    WAIT_START,
    READ_MSGID,
    READ_LENGTH_H,
    READ_LENGTH_L,
    READ_PAYLOAD,
    READ_CHECKSUM_H,
    READ_CHECKSUM_L
} UartReceiverState;

#define MAX_PAYLOAD 1024
#define START_BYTE 0xAA

UartReceiverState parserState = WAIT_START;
uint8_t receiverIndex = 0;
UART_Handle uartHandle;

uint8_t gBuffer[CONFIG_UART_BUFFER_LENGTH] = {0};

/************************ FUNCTIONS ***********************/

/** TODO: Update function's description
 * @brief Initializes the UART module
 * 
 * This function powers on the UART module and
 * prepares it for receiving and sending UART messages.
 */
void uart_init(void) {
    rdCount = 0;
    wrCount = 0;
    UART_Params params;

    DL_GPIO_enablePower(GPIOA);

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

int Uart_Process_Byte(uint8_t byte, uart_frame_t *rx_frame) {
    printf("Byte received: %02X\n", byte);
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
            parserState = READ_LENGTH_H;
            break;
        case READ_LENGTH_H:
            rx_frame->payload_len = byte << 8;
            break;
        case READ_LENGTH_L:
            rx_frame->payload_len += byte;
            if (rx_frame->payload_len > MAX_PAYLOAD) {
                parserState = WAIT_START;
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
            // TODO: Confirm checksum
            /*
            if (rx_frame->checksum) {
                parserState = WAIT_START;
                return UART_RECV_ERROR_BAD_CHECKSUM
            }
            */
            return UART_RECV_FULL_FRAME_RECEIVED;
    }
    return UART_RECV_NO_ERROR;
}

int uart_receive_frame(uart_frame_t *rx_frame) {
    // Read from UART stream one byte at a time. Assemble message as bytes come.
    UART_read(uartHandle, gBuffer, CONFIG_UART_BUFFER_LENGTH, &rdCount);
    int returnValue = UART_RECV_NO_ERROR;
    for (uint32_t i = 0; i < rdCount; i++) {
        printf("%02X ", gBuffer[i]);  // debug
        int processByteResult = Uart_Process_Byte(gBuffer[i], rx_frame);
        if (processByteResult != UART_RECV_NO_ERROR) {
            returnValue = processByteResult;
        }
        if (processByteResult == UART_RECV_FULL_FRAME_RECEIVED) {
            // Return frame!
            // Todo: mark remainder of gBuffer to be prepended next time.
            return UART_RECV_FULL_FRAME_RECEIVED;
        }
    }
    return returnValue;
}

int uart_send_frame(uint8_t msg_id, uint8_t *writeBuffer, uint16_t write_length) {
    if (write_length > UART_MAX_PAYLOAD_LEN) {
        return UART_SEND_ERROR_MSG_TOO_LONG;
    }

    // Prepare write frame
    uart_frame_t write_frame;
    write_frame.sof = START_BYTE;
    // TODO: add checking to confirm msg_id is valid
    write_frame.msg_id = msg_id;
    // Need to manually enforce big-endianness
    write_frame.payload_len = write_frame.payload_len = (write_length >> 8) | (write_length << 8);
    //TODO: passing a 1KB buffer may cause memory issues?
    memcpy(write_frame.payload, writeBuffer, write_length);
    // TODO: generate checksum
    int checksum = 0xABCD;
    write_frame.checksum = (checksum >> 8) | (checksum << 8);

    // Send write frame
    uint16_t totalSize = 6 + write_frame.payload_len; // SOF + ID + Len + Payload + Checksum
    UART_write(uartHandle, (uint8_t *)&write_frame, totalSize, &wrCount);
}

void uart_send_debug_msg(uint8_t *message, uint16_t write_length) {
    UART_write(uartHandle, message, write_length, &wrCount);
}

void uart_debug_echo_mode() {
    while(1) {
        UART_read(uartHandle, gBuffer, CONFIG_UART_BUFFER_LENGTH, &rdCount);
        for (uint32_t i = 0; i < rdCount; i++) {
            gBuffer[i] = gBuffer[i]+1;
        }
        //UART_write(uartHandle, uartWriteBuffer, uartWriteLength, &wrCount);
        uart_send_debug_msg(gBuffer, rdCount);
    }
}