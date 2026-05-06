/**
 * @file crypto_module.c
 * @author Vault Team - Purdue
 * @brief HSM Crypto Module
 * @date 2026
 *
 * Handles the encryption / decryption of file entries, file keys, and message payloads.
 * Generates keys, IVs, and auth tags for AES-256-GCM encryption.
 */

#include "crypto_module.h"

static curve25519_key priv25519;
static curve25519_key client25519;
static WC_RNG rng;

/**
 * @brief Defines the WolfSSL RNG function. Needed for hashing and Curve25519, apparently.
 *
 * @param output Pointer to the buffer of the byte output.
 * @param sz Size of the buffer in bytes.
 */
int HSM_CRYPTO_useCryptoRNG(unsigned char* output, unsigned int sz) {

    // Due to weird behavior, we can't use the HSM TRNG driver here, so we
    // need to repeat the functionality for WolfSSL types
    for (unsigned int i = 0; i < sz; i += 4) {
        uint32_t r = 0;

        // Get capture
        while (!DL_TRNG_isCaptureReady(TRNG));
        r = DL_TRNG_getCapture(TRNG);
        DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);
        DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CAPTURE_RDY_EVENT);

        // Horrible hack that aligns the bytes in case we accidentally floor a non-divisible by 4
        unsigned int chunk = (sz - i > 4) ? 4 : (sz - i);
        memcpy(&output[i], &r, chunk);
    }

    return 0;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_init(void) {
    
    // Initialize the TRNG driver
    if (HSM_TRNG_init() != HSM_TRNG_OK) return HSM_CRYPTO_ERR_TRNG_FAIL;

    // Initialize the AESADV driver
    AESADV_init();

    // Initialize RNG because apparently it is needed
    // even when you are not generating a private key
    wc_InitRng(&rng);

    // Initialize the X25519 key structs for ECDH
    if (wc_curve25519_init(&priv25519) != 0) return HSM_CRYPTO_ERR_SESSION_FAIL;
    if (wc_curve25519_init(&client25519) != 0) return HSM_CRYPTO_ERR_SESSION_FAIL;
    if (wc_curve25519_set_rng(&priv25519, &rng) != 0) return HSM_CRYPTO_ERR_SESSION_FAIL;
    if (wc_curve25519_set_rng(&client25519, &rng) != 0) return HSM_CRYPTO_ERR_SESSION_FAIL;

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

    // Null check
    if (!buf) return HSM_CRYPTO_ERR_NULL_PARAM;

    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    
    // We can just use the same logic used in generateKey for this
    if (HSM_CRYPTO_generateKey(buf, len) != HSM_CRYPTO_OK) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Perform the necessary clamping procedure
    buf[0] &= 248;
    buf[31] &= 127;
    buf[31] |= 64;

    // Import private key into X25519 key struct
    if (
        wc_curve25519_import_private_ex(
            (byte *) buf,
            (word32)(len / sizeof(byte)),
            &priv25519,
            EC25519_LITTLE_ENDIAN
        ) != 0
    ) return HSM_CRYPTO_ERR_SESSION_FAIL;

    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_loadPrivateSessionKey(uint8_t *buf, size_t len) {

    // Null check
    if (!buf) return HSM_CRYPTO_ERR_NULL_PARAM;

    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Perform the necessary clamping procedure
    buf[0] &= 248;
    buf[31] &= 127;
    buf[31] |= 64;

    // Import private key into X25519 key struct
    if (
        wc_curve25519_import_private_ex(
            (byte *) buf,
            (word32)(len / sizeof(byte)),
            &priv25519,
            EC25519_LITTLE_ENDIAN
        ) != 0
    ) return HSM_CRYPTO_ERR_SESSION_FAIL;

    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_generatePublicSessionKey(uint8_t *key, uint8_t *pub, size_t len) {
    
    // Null check
    if (!key || !pub) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Generate public key using basepoint = 9
    int ret = wc_curve25519_make_pub((int)len, (byte *)pub, (int)len, (byte *)key);
    if (ret != 0) return HSM_CRYPTO_ERR_SESSION_FAIL;
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_loadClientPublicKey(uint8_t *pub, size_t len) {
    
    // Null check
    if (!pub) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Import public key into X25519 key struct
    int ret = wc_curve25519_import_public_ex((byte *)pub, (word32)(len / sizeof(byte)), &client25519, EC25519_LITTLE_ENDIAN);
    if (ret != 0) return HSM_CRYPTO_ERR_SESSION_FAIL;
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_generateSharedSecret(uint8_t *secret, size_t len) {

    // Null check
    if (!secret) return HSM_CRYPTO_ERR_NULL_PARAM;

    // Size check
    if (len != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // We need to pass in a word32 pointer because it returns the size
    // We already know what the size will be, so these are garbage values
    word32 garbage = 256;
    word32 *garbageptr = &garbage;
    
    // Generate shared key using basepoint = client's pub
    if (
        wc_curve25519_shared_secret_ex(
            &priv25519,
            &client25519,
            (byte *)secret,
            garbageptr,
            EC25519_LITTLE_ENDIAN
        ) != 0
    ) return HSM_CRYPTO_ERR_SESSION_FAIL;
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_deriveKeyAndIV(uint8_t *secret, uint8_t *key, size_t len, uint8_t *iv, size_t ivlen) {

    // Null check
    if (!secret || !key || !iv) return HSM_CRYPTO_ERR_NULL_PARAM;

    // Size check
    if (len != CRYPTO_AES_KEY_SIZE || ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Clear output buffers just in case
    memset(key, 0, len);
    memset(iv, 0, ivlen);
    
    // Get HKDF extract using HMAC-SHA256
    byte extract[CRYPTO_AES_KEY_SIZE];
    memset(extract, 0, CRYPTO_AES_KEY_SIZE);
    if (
        wc_HKDF_Extract(
            WC_SHA256,
            NULL,
            0,
            (byte *)secret,
            (word32)(len / sizeof(byte)),
            extract
        ) != 0
    ) return HSM_CRYPTO_ERR_SESSION_FAIL;

    const byte *aesKeyInfo = (byte *)"aes-key";
    const byte *aesIVInfo = (byte *)"iv";
    
    // Expand into key, IV
    if (
        wc_HKDF_Expand(
            WC_SHA256,
            extract,
            (word32)(CRYPTO_AES_KEY_SIZE / sizeof(byte)),
            aesKeyInfo,
            7,
            (byte *)key,
            (word32)(CRYPTO_AES_KEY_SIZE / sizeof(byte))
        ) != 0
    ) return HSM_CRYPTO_ERR_SESSION_FAIL;
    if (
        wc_HKDF_Expand(
            WC_SHA256,
            extract,
            (word32)(CRYPTO_AES_KEY_SIZE / sizeof(byte)),
            aesIVInfo,
            2,
            (byte *)iv,
            (word32)(CRYPTO_GCM_IV_SIZE / sizeof(byte))
        ) != 0
    ) return HSM_CRYPTO_ERR_SESSION_FAIL;
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_encryptCommandPayload(
    uint8_t *key,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen,
    uint8_t *pt,
    uint8_t *ct,
    size_t len
) {

    // Null check
    if (!key || !iv || !at || !pt || !ct) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size check
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (len > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Clear output buffers for client just in case
    memset(ct, 0, len);
    memset(at, 0, atlen);

    // Encrypt the payload
    (void) AESADV_AESGCM256_encrypt(
        key,
        iv,
        NULL,
        0,
        pt,
        len,
        ct,
        at
    );
    
    return HSM_CRYPTO_OK;
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
    size_t len
) {

    // Null check
    if (!key || !iv || !at || !pt || !ct) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (len > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Clear output buffers just in case
    memset(key, 0, keylen);
    memset(iv, 0, ivlen);
    memset(at, 0, atlen);
    memset(ct, 0, len);

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
        len,
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

    // Null check
    if (!pkey || !ckey || !iv || !at) return HSM_CRYPTO_ERR_NULL_PARAM;
    
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
    // TODO: uncomment this out when CSC is implemented, remember to remove the hardcoded master key in the AES driver
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
        aadlen,
        at
    );
    
    return HSM_CRYPTO_OK;
}

HSM_CRYPTO_STATUS HSM_CRYPTO_decryptCommandPayload(
    uint8_t *key,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen,
    uint8_t *ct,
    uint8_t *pt,
    size_t len
) {

    // Null check
    if (!key || !iv || !at || !pt || !ct) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (len > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;
    
    // Create a temp buffer that will handle the plaintext for now
    // If success we copy to pt buf
    uint8_t temp[len];
    memset(temp, 0, len);

    // Decrypt the file
    int result = AESADV_AESGCM256_decrypt(
        key,
        iv,
        NULL,
        0,
        ct,
        len,
        at,
        temp
    );
    if (result != 0) return HSM_CRYPTO_ERR_AUTH_FAIL;

    // Success means wwe can copy temp into pt
    // Clear output buffers just in case
    memset(pt, 0, len);
    memcpy(pt, temp, len);
    
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
    size_t len
) {

    // Null check
    if (!key || !iv || !at || !pt || !ct) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (len > CRYPTO_CHUNK_PAYLOAD) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Create a temp buffer that will handle the plaintext for now
    // If success we copy to pt buf
    uint8_t temp[len];
    memset(temp, 0, len);

    // Decrypt the file
    int result = AESADV_AESGCM256_decrypt(
        key,
        iv,
        aad,
        aadlen,
        ct,
        len,
        at,
        temp
    );
    if (result != 0) return HSM_CRYPTO_ERR_AUTH_FAIL;

    // Success means wwe can copy temp into pt
    // Clear output buffers just in case
    memset(pt, 0, len);
    memcpy(pt, temp, len);
    
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

    // Null check
    if (!pkey || !ckey || !iv || !at) return HSM_CRYPTO_ERR_NULL_PARAM;
    
    // Size checks
    if (keylen != CRYPTO_AES_KEY_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (ivlen != CRYPTO_GCM_IV_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;
    if (atlen != CRYPTO_GCM_TAG_SIZE) return HSM_CRYPTO_ERR_BAD_LENGTH;

    // Initialize dummy AAD
    // TODO: add AAD?
    uint8_t *aad = NULL;
    size_t aadlen = 0;

    // Create a temp buffer that will handle the plaintext for now
    // If success we copy to pt buf
    uint8_t temp[keylen];
    memset(temp, 0, keylen);

    // Transfer key from KEYSTORE to AESADV
    // TODO: uncomment this out when CSC is implemented, remember to remove the hardcoded master key in the AES driver
    if (HSM_KEYSTORE_transferRootKeyToAES() != HSM_KEYSTORE_OK) return HSM_CRYPTO_ERR_KEYSTORE_FAIL;

    // Encrypt the key
    int result = AESADV_AESGCM256_decryptKey(
        ckey,
        temp,
        keylen,
        iv,
        aad,
        aadlen,
        at
    );
    if (result != 0) return HSM_CRYPTO_ERR_AUTH_FAIL;

    // Success means wwe can copy temp into pt
    // Clear output buffers just in case
    memset(pkey, 0, keylen);
    memcpy(pkey, temp, keylen);
    
    return HSM_CRYPTO_OK;
}

