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

typedef enum direction {
    EAST, NORTH, WEST, SOUTH
}host_message_t;

#endif