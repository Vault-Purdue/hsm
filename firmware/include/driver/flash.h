/**
 * @file flash.h
 * @author Vault Team - Purdue
 * @brief flash interface functions
 * @date 2026
 *
 * Wrapper functions for reading from/writing to flash memory.
 */

#ifndef __FLASH__
#define __FLASH__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ti/driverlib/driverlib.h>
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/m0p/dl_core.h>

/********************** MISC CONSTS ***********************/

#define FLASH_PAGE_SIZE DL_FLASHCTL_SECTOR_SIZE  // 1024 bytes

/************************ FUNCTIONS ***********************/

// Erase a page of flash
bool flash_erase_page(uint32_t address);

// Read data from flash
void flash_read(uint32_t address, void* buffer, uint32_t size);

// Write data to flash
bool flash_write(uint32_t address, const void* buffer, uint32_t size);

#endif