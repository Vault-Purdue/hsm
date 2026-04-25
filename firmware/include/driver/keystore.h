/**
 * @file keystore.h
 * @author Vault Team - Purdue
 * @brief KEYSTORE Driver Header File
 * @date 2026
 *
 * Constants/stubs for the KEYSTORE driver.
 */

#ifndef _HSM_KEYSTORE_H_
#define _HSM_KEYSTORE_H_

#include <stdbool.h>
#include <stdint.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/dl_keystorectl.h>

/*********************** CONSTANTS ************************/

#define KEYSTORE_KEY_SIZE DL_KEYSTORECTL_KEY_SIZE_256_BITS
#define KEYSTORE_NUM_256_KEYS DL_KEYSTORECTL_NUM_256_KEYS_TWO
#define KEYSTORE_ROOT_KEY_SLOT DL_KEYSTORECTL_KEY_SLOT_0
typedef enum {
    HSM_KEYSTORE_OK,
    HSM_KEYSTORE_ERR_NON_SEC_STATE,
    HSM_KEYSTORE_ERR_INVALID_CFG,
    HSM_KEYSTORE_ERR_NO_KEY_FOUND,
    HSM_KEYSTORE_ERR_BAD_RX,
    HSM_KEYSTORE_ERR_BAD_TX
} HSM_KEYSTORE_STATUS;

/********************* FUNCTION STUBS *********************/

/**
 * @brief Initializes the KEYSTORE configurations and writes the root key to storage.
 * 
 * @param buf Root key buffer.
 *
 * @retval 0: root key was successfully written to KEYSTORE
 * @retval 1: INITDONE was issued
 * @retval 2: non-valid status after setting config
 * @retval 4: non-valid status after writing to KEYSTORE
 */
HSM_KEYSTORE_STATUS HSM_KEYSTORE_initRootKeyStorage(uint32_t *buf);

/**
 * @brief Transfers the root key in storage to the AESADV.
 *
 * @retval 0: root key was successfully transferred to AESADV
 * @retval 2: non-valid status before transfer
 * @retval 3: root key not in KEYSTORE
 * @retval 5: non-valid status after transferring to AESADV
 */
HSM_KEYSTORE_STATUS HSM_KEYSTORE_transferRootKeyToAES(void);

#endif