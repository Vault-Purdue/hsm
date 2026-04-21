/**
 * @file crypto_module.c
 * @author Vault Team - Purdue
 * @brief HSM Crypto Module
 * @date 2026
 *
 * Handles the encryption of file entries amd file keys. Handles the
 * decryption of file entries, file keys, and client commands.
 * Generates keys, IVs, and auth tags for AES-256-GCM encryption.
 */

#include "crypto_module.h"

HSM_CRYPTO_STATUS HSM_CRYPTO_init(void) {
    
    // Initialize the TRNG driver
    if (HSM_TRNG_init() != HSM_TRNG_OK) return HSM_CRYPTO_ERR_TRNG_FAIL;

    // Initialize the AESADV driver
    AESADV_init();

    return HSM_CRYPTO_OK;
}

/**
 * @brief Generates a 12-byte IV for AES-256-GCM encryption.
 * 
 * @param buf Pointer to the IV buffer in memory.
 * @param len Size of the IV buffer in bytes.
 * 
 * @retval 0: IV was successfully generated.
 * @retval 2: Invalid length for IV was given.
 */
static HSM_CRYPTO_STATUS HSM_CRYPTO_generateIV(uint8_t *buf, size_t len) {

    // Size check
    if (len != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Get random number from TRNG
    HSM_TRNG_generateNumber((uint32_t *) buf, CRYPTO_GCM_IV_SIZE / sizeof(uint32_t));

    return HSM_CRYPTO_OK;
}

/**
 * @brief Generates a 256-bit key for AES-256-GCM encryption.
 *
 * @param buf Pointer to the key buffer in memory.
 * @param len Size of the key buffer in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 2: Invalid length for key was given.
 */
static HSM_CRYPTO_STATUS HSM_CRYPTO_generateKey(uint8_t *buf, size_t len) {

    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Get random number from TRNG
    HSM_TRNG_generate256BitNumber((uint32_t *) buf, CRYPTO_AES_KEY_SIZE / sizeof(uint32_t));

    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_generatePrivateSessionKey(uint8_t *buf, size_t len) {
    
    // We can just use the same logic used in generateKey for this
    if (HSM_CRYPTO_generateKey(buf, len) != HSM_CRYPTO_OK) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Perform the necessary clamping procedure
    buf[0] &= 248;
    buf[31] &= 127;
    buf[31] |= 64;

    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_generatePublicSessionKey(uint8_t *key, uint8_t *pub, size_t len) {
    
    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    
    // Generate public key using basepoint = 9
    cf_curve25519_mul_base(pub, key);
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_generateSharedKey(uint8_t *key, uint8_t *pub, uint8_t *shared, size_t len) {

    // Null check
    if (!key || !pub || !shared) return HSM_CRYPTO_ERR_NULL_PARAM;

    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    
    // Generate shared key using basepoint = client's pub
    cf_curve25519_mul(shared, key, pub);
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_encryptCommandPayload(
    uint8_t *sframe,
    uint8_t *cframe,
    size_t len,
    uint8_t *key,
    size_t keylen
) {

    // Null check
    if (!sframe || !cframe || !key) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size check
    if (len > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Clear buffer for client just in case
    memset(cframe, 0, len);
    
} 

HSM_CRYPTO_STATUS HSM_CRYPTO_encryptFile(
    uint8_t *key,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen,
    uint8_t *pt,
    uint8_t *ct,
    size_t ptlen
) {
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ptlen > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Clear output buffers just in case
    memset(key, 0, keylen);
    memset(iv, 0, ivlen);
    memset(at, 0, atlen);
    memset(ct, 0, ptlen);

    // Generate key, IV
    (void) HSM_CRYPTO_generateKey(key, keylen);
    (void) HSM_CRYPTO_generateIV(iv, ivlen);

    // Encrypt the file
    (void) AESADV_AESGCM256_encrypt(
        key,
        iv,
        aad,
        aadlen,
        pt,
        ptlen,
        ct,
        at
    );
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_encryptFileKey(
    uint8_t *pkey,
    uint8_t *ckey,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen
) {
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Clear output buffers just in case
    memset(ckey, 0, keylen);
    memset(iv, 0, ivlen);
    memset(at, 0, atlen);

    // Transfer key from KEYSTORE to AESADV
    if (HSM_KEYSTORE_transferRootKeyToAES() != HSM_KEYSTORE_OK) return HSM_CRYPTO_ERR_KEYSTORE_FAIL;
    
    // Generate IV
    (void) HSM_CRYPTO_generateIV(iv, ivlen);

    // Encrypt the key
    (void) AESADV_AESGCM256_encryptKey(
        pkey,
        ckey,
        keylen,
        iv,
        aad,
        aad_len,
        at
    );
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_decryptFile(
    uint8_t *key,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen,
    uint8_t *ct,
    uint8_t *pt,
    size_t ctlen
) {
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ctlen > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Clear output buffers just in case
    memset(pt, 0, ctlen);

    // Decrypt the file
    int result = AESADV_AESGCM256_decrypt(
        key,
        iv,
        aad,
        aadlen,
        ct,
        ctlen,
        at,
        pt
    );
    if (result != 0) return HSM_CRYPTO_ERR_AUTH_FAIL;
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_decryptFileKey(
    uint8_t *ckey,
    uint8_t *pkey,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen
) {
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Clear output buffers just in case
    memset(pkey, 0, keylen);

    // Transfer key from KEYSTORE to AESADV
    if (HSM_KEYSTORE_transferRootKeyToAES() != HSM_KEYSTORE_OK) return HSM_CRYPTO_ERR_KEYSTORE_FAIL;

    // Encrypt the key
    int result = AESADV_AESGCM256_decryptKey(
        pkey,
        ckey,
        keylen,
        iv,
        aad,
        aad_len,
        at
    );
    if (result != 0) return HSM_CRYPTO_ERR_AUTH_FAIL;
    
    return HSM_CRYPTO_OK;
}
