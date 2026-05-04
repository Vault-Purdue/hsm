/**
 * @file csc_boot.h
 * @author Vault Team - Purdue
 * @brief Customer Secure Code bootloader
 * @date 2026
 *
 * Runs before INITDONE in privileged state. Handles root key
 * provisioning on first boot and KEYSTORE + firewall setup on every boot.
 *
 * Boot flow - first boot:
 *  1. TRNG generates 256-bit root key
 *  2. Root key written to .secret flash sector
 *  3. Provisioning magic written to .lockStg sector
 *  4. Root key loaded into KEYSTORE slot 0
 *  5. Hardware firewall enabled over .secret
 *  6. INITDONE issued — device leaves privileged state
 *
 * Boot flow — subsequent boots:
 *  1. Root key read from .secret (already provisioned)
 *  2. Root key loaded into KEYSTORE slot 0
 *  3. Hardware firewall enabled over .secret
 *  4. INITDONE issued
 *
 * After CSC_boot() returns:
 *  - INITDONE has been issued
 *  - .secret is hardware-firewalled
 *  - Root key is live in KEYSTORE, ready for AESADV transfer
 */

#ifndef CSC_BOOT_H
#define CSC_BOOT_H

/* Return codes */
#define CSC_BOOT_OK (0)
#define CSC_BOOT_ERR_LATE (-1)  /* Called after INITDONE already issued  */
#define CSC_BOOT_ERR_TRNG (-2)  /* TRNG generation failed */
#define CSC_BOOT_ERR_FLASH (-3)  /* Flash erase or write failed */
#define CSC_BOOT_ERR_KEYSTORE (-4)  /* KEYSTORE write rejected*/

/**
 * @brief Run the CSC bootloader.
 *
 * Must be called from main() after SYSCFG_DL_init() (or SYS_initPower +
 * peripheral init) befre any HSM application logic
 *
 * @return CSC_BOOT_OK on success, negative error code otherwise
 */
int CSC_boot(void);

#endif /* CSC_BOOT_H */