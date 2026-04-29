/**
 * @file csc_keyload.h
 * @author Vault Team - Purdue
 * @brief Vault CSC root key load
 * @date 2026
 *
 * Customer Secure Code routine that pre-loads the vault root key
 * into the KEYSTORE peripheral before INITDONE is asserted. After
 * this completes, the main HSM application can transfer the key
 * to AESADV but cannot read or rewrite it.
 *
 * NOTE: This is the development-mode loader using a fixed key
 * value matching the test vector currently hardcoded in
 * aes_adv_gcm.c. For production, replace with a TRNG-generated +
 * flash-persisted key (see file_manager.md, "Dependency 3").
 */

#ifndef _CSC_KEYLOAD_H_
#define _CSC_KEYLOAD_H_

#include "keystore.h"

/**
 * @brief Load the vault root key into KEYSTORE slot 0.
 *
 * Must be called from the CSC after peripheral init and BEFORE
 * DL_SYSCTL_issueINITDONE(). KEYSTORE only accepts writes while
 * the device is in the secure init state.
 *
 * @retval HSM_KEYSTORE_OK on success.
 * @retval Other HSM_KEYSTORE_STATUS values on failure (see keystore.h).
 */
HSM_KEYSTORE_STATUS CSC_loadVaultRootKey(void);

#endif  /* _CSC_KEYLOAD_H_ */
