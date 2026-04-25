/**
 * @file keystore.c
 * @author Vault Team - Purdue
 * @brief KEYSTORE Driver
 * @date 2026
 *
 * Functions for interacting with the KEYSTORE module to store the root key for AES encryption.
 */

#include "keystore.h"

static bool initialized = false;
static const DL_KEYSTORECTL_Config rootKeyTransferConfig = {
    .keySlot = KEYSTORE_ROOT_KEY_SLOT,
    .keySize = KEYSTORE_KEY_SIZE,
    .cryptoSel = DL_KEYSTORECTL_CRYPTO_SEL_AES,
};

HSM_KEYSTORE_STATUS HSM_KEYSTORE_initRootKeyStorage(uint32_t *buf) {
    
    // If for some reason we attempt to call this function again
    if (initialized) return HSM_KEYSTORE_OK;

    // "This ... can only be written in a secure operating state before the INITDONE signal is asserted."
    if (DL_SYSCTL_isINITDONEIssued()) return HSM_KEYSTORE_ERR_NON_SEC_STATE;

    // Set the number of 256-bit keys
    DL_KEYSTORECTL_setNumberOf256Keys(KEYSTORECTL, KEYSTORE_NUM_256_KEYS);

    // Validate status
    while (DL_KEYSTORECTL_getStatus(KEYSTORECTL) == DL_KEYSTORECTL_STATUS_NO_CONFIG);
    if (DL_KEYSTORECTL_getStatus(KEYSTORECTL) != DL_KEYSTORECTL_STATUS_VALID) return HSM_KEYSTORE_ERR_INVALID_CFG;

    // Write the key to KEYSTORE
    DL_KEYSTORECTL_KeyWrConfig rootKeyWriteConfig = {
        .keySlot = KEYSTORE_ROOT_KEY_SLOT,
        .keySize = KEYSTORE_KEY_SIZE,
        .key = buf
    };
    DL_KEYSTORECTL_writeKey(KEYSTORECTL, (DL_KEYSTORECTL_KeyWrConfig*) &rootKeyWriteConfig);

    // Validate status
    while (DL_KEYSTORECTL_getStatus(KEYSTORECTL) == DL_KEYSTORECTL_STATUS_BUSY_RX);
    if (DL_KEYSTORECTL_getStatus(KEYSTORECTL) != DL_KEYSTORECTL_STATUS_VALID) return HSM_KEYSTORE_ERR_BAD_RX;
    
    initialized = true;
    return HSM_KEYSTORE_OK;
}

HSM_KEYSTORE_STATUS HSM_KEYSTORE_transferRootKeyToAES(void) {
    
    // Validate status
    if (DL_KEYSTORECTL_getStatus(KEYSTORECTL) != DL_KEYSTORECTL_STATUS_VALID) return HSM_KEYSTORE_ERR_INVALID_CFG;

    // We need to verify that our root key slot is occupied
    if (DL_KEYSTORECTL_getValidKeySlots(KEYSTORECTL) == 0) return HSM_KEYSTORE_ERR_NO_KEY_FOUND;

    // Transfer key
    DL_KEYSTORECTL_transferKey(KEYSTORECTL, (DL_KEYSTORECTL_Config*) &rootKeyTransferConfig);

    // Validate status
    while (DL_KEYSTORECTL_getStatus(KEYSTORECTL) == DL_KEYSTORECTL_STATUS_BUSY_TX);
    if (DL_KEYSTORECTL_getStatus(KEYSTORECTL) != DL_KEYSTORECTL_STATUS_VALID) return HSM_KEYSTORE_ERR_BAD_TX;

    return HSM_KEYSTORE_OK;
}