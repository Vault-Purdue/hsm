/**
 * @file crypto_module.c
 * @author Vault Team - Purdue
 * @brief Crypto module - AES-128-GCM encrypt/decrypt
 * @date 2026
 *
 * Calls into the AESADV and TRNG drivers.
 *
 * TODO: when the AESADV/TRNG headers are done, add includes below
 * and rename the following placeholder functions:
 *   aesadv_gcm_encrypt, aesadv_gcm_decrypt, trng_get_bytes
 */

#include "crypto_module.h"

// TODO: add real driver headers when they are complete */
// #include "aesadv.h"
// #include "trng.h"

/* ========================================================================= */
/* Placeholder driver prototypes - delete these once the real headers
 * are included above. Just here so the file compiles standalone. */
extern int aesadv_gcm_encrypt(
    const uint8_t *key, uint32_t key_len,
    const uint8_t *iv, uint32_t iv_len,
    const uint8_t *aad, uint32_t aad_len, // Additional Authenticated Data
                                          // Validated Data but not encrypted
    const uint8_t *pt, uint32_t pt_len,
    uint8_t *ct,
    uint8_t *tag, uint32_t tag_len);

extern int aesadv_gcm_decrypt(
    const uint8_t *key, uint32_t key_len,
    const uint8_t *iv, uint32_t iv_len,
    const uint8_t *aad, uint32_t aad_len, // Additional Authenticated Data
                                          // Validated Data but not encrypted
    const uint8_t *ct, uint32_t ct_len,
    const uint8_t *tag, uint32_t tag_len,
    uint8_t *pt);

extern int trng_get_bytes(uint8_t *out, uint32_t len);

/* ========================================================================= */

// Wipe the plaintext buffer if decrypt fails (aka if someone was tampering)
// so the file manager doesn't end up with some garbage bits 
static void wipe(void *p, uint32_t n)
{
    volatile uint8_t *v = (volatile uint8_t *)p;
    while (n--) {
        *v++ = 0;
    }
}

// This is an empty function for now since TRNG and AESADV will do 
// their own inits. But we can call them here if the team chooses 
crypto_status crypto_init(void)
{
    return CRYPTO_OK;
}

// Fills a buffer with the random bytes from the TRNG 
crypto_status crypto_random(uint8_t *out, uint32_t len)
{
    if (out == NULL) {
        return CRYPTO_ERR_NULL_PARAM;
    }
    if (len == 0) {
        return CRYPTO_OK;
    }

    if (trng_get_bytes(out, len) != 0) {
        return CRYPTO_ERR_TRNG_FAIL;
    }
    return CRYPTO_OK;
}

// This generates a new 12 byte Initialization Vector for the AES-GCM
crypto_status crypto_generate_iv(uint8_t iv[CRYPTO_GCM_IV_SIZE])
{
    return crypto_random(iv, CRYPTO_GCM_IV_SIZE);
}

crypto_status crypto_gcm_encrypt(
    const uint8_t key[CRYPTO_AES_KEY_SIZE],
    const uint8_t iv[CRYPTO_GCM_IV_SIZE],
    const uint8_t *aad, uint32_t aad_len,
    const uint8_t *pt, uint32_t pt_len,
    uint8_t *ct,
    uint8_t tag[CRYPTO_GCM_TAG_SIZE])
{
    if (key == NULL || iv == NULL || tag == NULL) {
        return CRYPTO_ERR_NULL_PARAM;
    }
    if (pt_len > 0 && (pt == NULL || ct == NULL)) {
        return CRYPTO_ERR_NULL_PARAM;
    }
    if (aad_len > 0 && aad == NULL) {
        return CRYPTO_ERR_NULL_PARAM;
    }

    int rc = aesadv_gcm_encrypt(
        key, CRYPTO_AES_KEY_SIZE,
        iv, CRYPTO_GCM_IV_SIZE,
        aad, aad_len,
        pt, pt_len,
        ct,
        tag, CRYPTO_GCM_TAG_SIZE);

    if (rc != 0) {
        return CRYPTO_ERR_HW_FAIL;
    }
    return CRYPTO_OK;
}

crypto_status crypto_gcm_decrypt(
    const uint8_t key[CRYPTO_AES_KEY_SIZE],
    const uint8_t iv[CRYPTO_GCM_IV_SIZE],
    const uint8_t *aad, uint32_t aad_len,
    const uint8_t *ct, uint32_t ct_len,
    const uint8_t tag[CRYPTO_GCM_TAG_SIZE],
    uint8_t *pt)
{
    if (key == NULL || iv == NULL || tag == NULL) {
        return CRYPTO_ERR_NULL_PARAM;
    }
    if (ct_len > 0 && (ct == NULL || pt == NULL)) {
        return CRYPTO_ERR_NULL_PARAM;
    }
    if (aad_len > 0 && aad == NULL) {
        return CRYPTO_ERR_NULL_PARAM;
    }
    
    int rc = aesadv_gcm_decrypt(
        key, CRYPTO_AES_KEY_SIZE,
        iv, CRYPTO_GCM_IV_SIZE,
        aad, aad_len,
        ct, ct_len,
        tag, CRYPTO_GCM_TAG_SIZE,
        pt);

    if (rc != 0) {
        // Could be a tag mismatch or a hardware error. Either way,
        // wipe the plaintext buffer so the caller can't accidentally
        // trust unauthenticated data.
        if (pt != NULL && ct_len > 0) {
            wipe(pt, ct_len);
        }
        return CRYPTO_ERR_AUTH_FAIL;
    }
    return CRYPTO_OK;
}
/* ========================================================================= */
