/**
 * @file csc_boot.c
 * @author Vault Team - Purdue
 * @brief Customer Secure Code bootloader implementation
 * @date 2026
 */

#include "csc_boot.h"
#include "../src/customer_secure_config.h"
#include "../include/driver/flash.h"
#include "../include/driver/trng.h"
#include "../include/driver/keystore.h"

#include <string.h>
#include <ti/driverlib/driverlib.h>

#define PROVISIONING_MAGIC (0xC0FFEE42UL)

static void secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) {
        *p++ = 0x00;
    }
}

// CSC_boot()
int CSC_boot(void)
{
    /* Reject if called after INITDONE — device is no longer privileged */
    if (DL_SYSCTL_isINITDONEIssued()) {
        return CSC_BOOT_ERR_LATE;
    }

    uint32_t magic = 0;
    flash_read(CSC_LOCK_STORAGE_ADDR, &magic, sizeof(magic));

    bool first_boot = (magic != PROVISIONING_MAGIC);

    if (first_boot) {

        uint32_t new_key[TRNG_256_BIT_BUF_SIZE]; /* 8 × uint32_t = 32 bytes */
        HSM_TRNG_generate256BitNumber(new_key, sizeof(new_key));

        if (!flash_erase_page(CSC_SECRET_ADDR)) {
            secure_zero(new_key, sizeof(new_key));
            return CSC_BOOT_ERR_FLASH;
        }

        if (!flash_write(CSC_SECRET_ADDR + CSC_ROOT_KEY_OFFSET,
                         new_key, CSC_ROOT_KEY_SIZE)) {
            secure_zero(new_key, sizeof(new_key));
            return CSC_BOOT_ERR_FLASH;
        }

        /* Wipe key from stack immediately after flash write */
        secure_zero(new_key, sizeof(new_key));

        if (!flash_erase_page(CSC_LOCK_STORAGE_ADDR)) {
            return CSC_BOOT_ERR_FLASH;
        }

        uint32_t sentinel = PROVISIONING_MAGIC;
        if (!flash_write(CSC_LOCK_STORAGE_ADDR, &sentinel, sizeof(sentinel))) {
            return CSC_BOOT_ERR_FLASH;
        }
    }

    uint32_t *root_key_ptr =
        (uint32_t *)(CSC_SECRET_ADDR + CSC_ROOT_KEY_OFFSET);

    HSM_KEYSTORE_STATUS ks = HSM_KEYSTORE_initRootKeyStorage(root_key_ptr);
    if (ks != HSM_KEYSTORE_OK) {
        return CSC_BOOT_ERR_KEYSTORE;
    }

    DL_SYSCTL_setReadExecuteProtectFirewallAddrStart(CSC_SECRET_ADDR);
    DL_SYSCTL_setReadExecuteProtectFirewallAddrEnd(CSC_SECRET_END);
    DL_SYSCTL_enableReadExecuteProtectFirewall();

    DL_SYSCTL_issueINITDONE();

    return CSC_BOOT_OK;
}