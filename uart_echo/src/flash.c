/**
 * @file flash.c
 * @author Vault Team - Purdue
 * @brief flash interface functions
 * @date 2026
 *
 * Wrapper functions for reading from/writing to flash memory. 
 * Dependencies on custom safety macros have been removed.
 */

#include "flash.h"

/************************ FUNCTIONS ***********************/

/** @brief Flash Erase Page
 *
 * This function erases a page of flash such that it can be updated.
 * Flash memory can only be erased in a large block size called a page.
 * Once erased, memory can only be written one way e.g. 1->0.
 * In order to be re-written the entire page must be erased.
 *
 * @param address address of flash page to erase
 *
 * @return true if success, false if failure
 */
bool flash_erase_page(uint32_t address) {
    DL_FLASHCTL_COMMAND_STATUS cmdStatus;
    bool ret;

    // Unprotect memory so it can be cleared
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, address, DL_FLASHCTL_REGION_SELECT_MAIN);

    // Erase the page and check for failure
    cmdStatus = DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, address, DL_FLASHCTL_COMMAND_SIZE_SECTOR);
    if (cmdStatus == DL_FLASHCTL_COMMAND_STATUS_FAILED) {
        return false;
    }

    // wait until the erase is done and check for failure
    ret = DL_FlashCTL_waitForCmdDone(FLASHCTL);
    if (ret == false) {
        return false;
    }
    
    return true;
}

/** @brief Flash Read
 *
 * This function reads data from the specified flash page into the buffer
 * with the specified amount of bytes
 *
 * @param address address of flash page to read
 * @param buffer pointer to buffer for data to be read into
 * @param size number of bytes to read from flash
 */
void flash_read(uint32_t address, void* buffer, uint32_t size) {
    memcpy(buffer, (void*)address, size);
}

/** @brief Flash Write
 *
 * This function writes data to the specified flash page from the buffer passed
 * with the specified amount of bytes. Flash memory can only be written in one
 * way e.g. 1->0. To rewrite previously written memory see the
 * `flash_erase_page`.
 *
 * @param address address of flash page to write
 * @param buffer pointer to buffer to write data from
 * @param size number of bytes to write from flash
 *
 * @return true if success, false if failure
 */
bool flash_write(uint32_t address, const void* buffer, uint32_t size) {
    DL_FLASHCTL_COMMAND_STATUS cmdStatus;
    uint32_t size_32b;
    bool ret;

    // Unprotect memory so it can be programmed
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, address, DL_FLASHCTL_REGION_SELECT_MAIN);

    // program function expects size to be the number of 32-bit words
    size_32b = (size % 4 == 0) ? (size / 4) : (size / 4) + 1;
    // it also expects it to be an even number
    size_32b = (size_32b % 2 == 0) ? size_32b : size_32b + 1;

    // write the data into a correctly sized region initialized with all bits set to 1
    if (size_32b < 100) {
        uint32_t write_data[size_32b];
        memset(write_data, 0xff, size_32b * 4);
        memcpy(write_data, buffer, size);

        // write to flash with ECC
        cmdStatus = DL_FlashCTL_programMemoryBlockingFromRAM64WithECCGenerated(
            FLASHCTL, address, write_data, size_32b, DL_FLASHCTL_REGION_SELECT_MAIN);
        
        if (cmdStatus == DL_FLASHCTL_COMMAND_STATUS_FAILED) {
            return false;
        }

    } else {
        // write to flash with ECC
        cmdStatus = DL_FlashCTL_programMemoryBlockingFromRAM64WithECCGenerated(
            FLASHCTL, address, (uint32_t*)buffer, size_32b, DL_FLASHCTL_REGION_SELECT_MAIN);
        
        if (cmdStatus == DL_FLASHCTL_COMMAND_STATUS_FAILED) {
            return false;
        }
    }

    // wait until the write is done and check for failure
    ret = DL_FlashCTL_waitForCmdDone(FLASHCTL);
    if (ret == false) {
        return false;
    }
    
    return true;
}