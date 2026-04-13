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

/************************ FUNCTIONS ***********************/

/** TODO: Update function's description
 * @brief Powers on and initializes the TRNG module
 * 
 * This function powers on the TRNG module and
 * initializes the TRNG clock and decimation rates.
 */
void uart_init(void) {
    UART_Params params;
    UART_Handle uartHandle;
    UART_Params_init(&params);
    //UART is configured for default baudrate of 9600bps
    uartHandle = UART_open(CONFIG_UART_0, &params);
    
    if (uartHandle == NULL) {
        // UART_open failed
        __BKPT();
    }
}