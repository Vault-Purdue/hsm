/**
 * @file crypto_module.h
 * @author Vault Team - Purdue
 * @brief HSM Crypto Module Header File
 * @date 2026
 *
 * Constants/function stubs for the HSM crypto module.
 */

#ifndef _HSM_CRYPTO_MODULE_H_
#define _HSM_CRYPTO_MODULE_H_

#include <stdint.h>
#include <string.h>
#include "driver/aes_adv_gcm.h"
#include "driver/keystore.h"
#include "driver/trng.h"
#include "wolfssl/wolfcrypt/curve25519.h"
#include "wolfssl/wolfcrypt/hmac.h"
#include "wolfssl/wolfcrypt/kdf.h"
#include "wolfssl/wolfcrypt/sha256.h"
#include "ti_drivers_config.h"

/* ============================================================== */
/* CONSTANTS */

#define CRYPTO_AES_KEY_SIZE  32  // AES-256
#define CRYPTO_GCM_IV_SIZE   12  // 96-bit IV
#define CRYPTO_GCM_TAG_SIZE  16  // 128-bit tag
#define CRYPTO_CHUNK_PAYLOAD 88  // Max bytes per file chunk
#define CRYPTO_CHUNK_SIZE    128 // File chunk size

/* ============================================================== */
/* ERROR CODES */

typedef enum {
    HSM_CRYPTO_OK,
    HSM_CRYPTO_ERR_NULL_PARAM,
    HSM_CRYPTO_ERR_BAD_LENGTH,
    HSM_CRYPTO_ERR_TRNG_FAIL, // TRNG driver returned an error
    HSM_CRYPTO_ERR_KEYSTORE_FAIL, // KEYSTORE driver returned an error
    HSM_CRYPTO_ERR_AUTH_FAIL, // GCM tag mismatch on decrypt
    HSM_CRYPTO_ERR_SESSION_FAIL // Errors related to session creation
} HSM_CRYPTO_STATUS;

/* ============================================================== */
/* FUNCTIONS */

/**
 * @brief Initializes the HSM crypto module.
 *
 * @retval 0: HSM crypto successfully initialized.
 * @retval 3: HSM TRNG driver failed to initialize.
 * @retval 6: Curve25519 key structs failed to initialize.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_init(void);

#ifndef ENABLE_HSM_CRYPTO_SESSION_TEST
/**
 * @brief Generates the X25519 private session key.
 *
 * @param buf Pointer to the buffer that will store the key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 6: Failed to import the buffer into the private key struct.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_generatePrivateSessionKey(uint8_t *buf, size_t len);
#endif

#ifdef ENABLE_HSM_CRYPTO_SESSION_TEST
/**
 * @brief Loads the X25519 private session key into the module.
 *
 * NOTE: for testing only.
 *
 * @param buf Pointer to the buffer of the key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully loaded.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 6: Failed to import the buffer into the private key struct.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_loadPrivateSessionKey(uint8_t *buf, size_t len);
#endif

/**
 * @brief Generates the X25519 public session key using the private key.
 *
 * @param key Pointer to the buffer of the private key.
 * @param pub Pointer to the buffer that will store the public key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 6: Failed to create the public key and store it in the buffer.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_generatePublicSessionKey(uint8_t *key, uint8_t *pub, size_t len);

/**
 * @brief Loads the client's X25519 public session key into the module.
 *
 * @param pub Pointer to the buffer of the public key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully loaded.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 6: Failed to import the buffer into the public key struct.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_loadClientPublicKey(uint8_t *pub, size_t len);

/**
 * @brief Generates the shared secret to use for encrypting / decrypting commands.
 *
 * @param secret Pointer to the buffer that will store the secret.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Secret was successfully generated.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length for buffer was given.
 * @retval 6: Key structs are null, or failed to store the secret in the buffer.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_generateSharedSecret(uint8_t *secret, size_t len);

/**
 * @brief Generates a derived AES-256-GCM key and IV using the shared ECDH secret.
 *
 * @param secret Pointer to the buffer of the secret.
 * @param key Pointer to the buffer that will store the key.
 * @param len Size of the secret and key buffers in bytes.
 * @param iv Pointer to the buffer that will store the IV.
 * @param ivlen Size of the IV buffer in bytes.
 * 
 * @retval 0: Key and IV were successfully generated.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length for buffer was given.
 * @retval 6: Failed to extract a PRK, or failed to derive a key/IV and store it in their respective buffers.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_deriveKeyAndIV(uint8_t *secret, uint8_t *key, size_t len, uint8_t *iv, size_t ivlen);

/**
 * @brief Encrypts the HSM response payload sent over UART using the derived key/IV.
 *
 * @param key Pointer to the buffer of the derived key.
 * @param keylen Size of the key buffer in bytes.
 * @param iv Pointer to the buffer of the derived IV.
 * @param ivlen Size of the IV buffer in bytes.
 * @param at Pointer to the buffer that will store the GCM auth tag.
 * @param atlen Size of the GCM auth tag buffer in bytes.
 * @param pt Pointer to the buffer of the plaintext payload.
 * @param ct Pointer to the buffer that will store the ciphertext payload.
 * @param len Size of the payload buffers in bytes.
 * 
 * @retval 0: Payload was successfully encrypted.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 */
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
);

/**
 * @brief Encrypts a file payload via AES-256-GCM while generating a key, IV, and auth tag.
 *
 * @param key Pointer to the buffer that will store the unencrypted AES key.
 * @param keylen Size of the key buffer in bytes.
 * @param iv Pointer to the buffer that will store the IV.
 * @param ivlen Size of the IV buffer in bytes.
 * @param at Pointer to the buffer that will store the GCM auth tag.
 * @param atlen Size of the GCM auth tag buffer in bytes.
 * @param pt Pointer to the buffer of the plaintext payload.
 * @param ct Pointer to the buffer that will store the ciphertext payload.
 * @param len Size of the payload buffers in bytes.
 *
 * @retval 0: File payload was successfully encrypted.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 */
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
);

/**
 * @brief Encrypts an AES-256-GCM file key while generating an IV and auth tag using the master key.
 *
 * NOTE: Master key needs to be loaded via the KEYSTORE driver before executing this.
 *
 * @param pkey Pointer to the buffer of the plaintext key.
 * @param ckey Pointer to the buffer that will store the ciphertext key.
 * @param keylen Size of the key buffers in bytes.
 * @param iv Pointer to the buffer that will store the IV.
 * @param ivlen Size of the IV buffer in bytes.
 * @param at Pointer to the buffer that will store the GCM auth tag.
 * @param atlen Size of the GCM auth tag buffer in bytes.
 *
 * @retval 0: File key was successfully encrypted.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 4: Root key failed to transfer to the AESADV engine.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_encryptFileKey(
    uint8_t *pkey,
    uint8_t *ckey,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen
);

/**
 * @brief Decrypts the client request payload sent over UART using the derived key/IV.
 *
 * @param key Pointer to the buffer of the derived key.
 * @param keylen Size of the key buffer in bytes.
 * @param iv Pointer to the buffer of the derived IV.
 * @param ivlen Size of the IV buffer in bytes.
 * @param at Pointer to the buffer of the GCM auth tag.
 * @param atlen Size of the GCM auth tag buffer in bytes.
 * @param ct Pointer to the buffer of the ciphertext payload.
 * @param pt Pointer to the buffer that will store the plaintext payload.
 * @param len Size of the payload buffers in bytes.
 * 
 * @retval 0: Payload was successfully encrypted.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 5: Auth tag indicated integrity failure.
 */
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
);

/**
 * @brief Decrypts a file payload via AES-256-GCM while validating the auth tag.
 *
 * @param key Pointer to the buffer of the unencrypted AES key.
 * @param keylen Size of the key buffer in bytes.
 * @param iv Pointer to the buffer of the IV.
 * @param ivlen Size of the IV buffer in bytes.
 * @param at Pointer to the buffer of the GCM auth tag.
 * @param atlen Size of the GCM auth tag buffer in bytes.
 * @param ct Pointer to the buffer of the ciphertext payload.
 * @param pt Pointer to the buffer that will store the plaintext payload.
 * @param len Size of the payload buffers in bytes.
 *
 * @retval 0: File payload was successfully decrypted.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 5: Auth tag indicated integrity failure.
 */
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
);

/**
 * @brief Decrypts an AES-256-GCM file key while validating the auth tag using the master key.
 *
 * NOTE: Master key needs to be loaded via the KEYSTORE driver before executing this.
 *
 * @param ckey Pointer to the buffer of the ciphertext key.
 * @param pkey Pointer to the buffer that will store the plaintext key.
 * @param keylen Size of the key buffers in bytes.
 * @param iv Pointer to the buffer of the IV.
 * @param ivlen Size of the IV buffer in bytes.
 * @param at Pointer to the buffer of the GCM auth tag.
 * @param atlen Size of the GCM auth tag buffer in bytes.
 *
 * @retval 0: File key was successfully decrypted.
 * @retval 1: Null parameter was given in the parameters.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 4: Root key failed to transfer to the AESADV engine.
 * @retval 5: Auth tag indicated integrity failure.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_decryptFileKey(
    uint8_t *ckey,
    uint8_t *pkey,
    size_t keylen,
    uint8_t *iv,
    size_t ivlen,
    uint8_t *at,
    size_t atlen
);

#endif