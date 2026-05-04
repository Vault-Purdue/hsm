/**
 * @file customer_secure_config.h
 * @author Vault Team - Purdue
 * @brief Memory map constants shared between CSC bootloader and HSM application
 * @date 2026
 */

#ifndef CUSTOMER_SECURE_CONFIG_H
#define CUSTOMER_SECURE_CONFIG_H

/* .secret sector holds the 256-bit TRNG-generated root key
 * Firewalled by hardware immediately after INITDONE is issued
 * Size matches one flash sector (1 kB) */
#define CSC_SECRET_ADDR       (0x00004000UL)
#define CSC_SECRET_SIZE       (0x00000400UL)
#define CSC_SECRET_END        (CSC_SECRET_ADDR + CSC_SECRET_SIZE - 1UL)

/* .lockStg sector holds the provisioning magic flag
 * Remains readable after INITDONE (not firewalled)
 * Set on first boot to prevent TRNG re-generation on subsequent boots */
#define CSC_LOCK_STORAGE_ADDR (0x00004400UL)
#define CSC_LOCK_STORAGE_SIZE (0x00000400UL)

/* Root key layout within .secret */
#define CSC_ROOT_KEY_OFFSET   (0x00UL)
#define CSC_ROOT_KEY_SIZE     (32U) /* 256-bit AES key */

#endif /* CUSTOMER_SECURE_CONFIG_H */