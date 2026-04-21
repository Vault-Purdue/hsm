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
#include "lib/curve25519.h"
#include "wolfssl/wolfcrypt/curve25519.h"
#include "wolfssl/wolfcrypt/hmac.h"
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
    HSM_CRYPTO_ERR_HW_FAIL, // AESADV driver returned an error
    HSM_CRYPTO_ERR_TRNG_FAIL, // TRNG driver returned an error
    HSM_CRYPTO_ERR_KEYSTORE_FAIL, // KEYSTORE driver returned an error
    HSM_CRYPTO_ERR_AUTH_FAIL // GCM tag mismatch on decrypt
} HSM_CRYPTO_STATUS;

/* ============================================================== */
/* FUNCTIONS */

/**
 * @brief Initializes the HSM crypto module.
 *
 * @retval 0: HSM crypto successfully initialized
 * @retval 4: HSM TRNG driver failed to initialize
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_init(void);

/**
 * @brief Generates the X25519 private session key.
 *
 * @param buf The buffer to store the key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 2: Invalid length for key was given.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_generatePrivateSessionKey(uint8_t *buf, size_t len);

/**
 * @brief Generates the X25519 public session key using the private key.
 *
 * @param key The buffer of the private key.
 * @param pub The buffer to store the public key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 2: Invalid length for key was given.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_generatePublicSessionKey(uint8_t *key, uint8_t *pub, size_t len);

/**
 * @brief Generates the shared key to use for encrypting / decrypting commands.
 *
 * @param key The buffer of the private key.
 * @param pub The buffer of the client public key.
 * @param shared The buffer to store the shared key.
 * @param len Size of the buffer in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 2: Invalid length for key was given.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_generateSharedKey(uint8_t *key, uint8_t *pub, uint8_t *shared, size_t len);

// TODO: still needs to be implemented
/**
 * @brief Encrypts the HSM response payload sent over UART using the shared key.
 *
 * @param sframe The buffer of the unencrypted UART response frame payload.
 * @param cframe The buffer to store the encrypted UART response frame payload.
 * @param len Size of the UART frame in bytes.
 * @param key The buffer of the shared key.
 * @param keylen Size of the key in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 2: Invalid length for key was given.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_encryptCommandPayload(uint8_t *sframe, uint8_t *cframe, size_t len, uint8_t *key, size_t keylen);

// TODO: still needs to be implemented
/**
 * @brief Decrypts the client request payload sent over UART using the shared key.
 *
 * @param cframe The buffer of the encrypted UART request frame payload.
 * @param sframe The buffer to store the unencrypted UART request frame payload.
 * @param len Size of the UART frame in bytes.
 * @param key The buffer of the shared key.
 * @param keylen Size of the key in bytes.
 * 
 * @retval 0: Key was successfully generated.
 * @retval 2: Invalid length for key was given.
 */
HSM_CRYPTO_STATUS HSM_CRYPTO_decryptCommandPayload(uint8_t *cframe, uint8_t *sframe, size_t len, uint8_t *key, size_t keylen);

/**
 * @brief Encrypts a file payload while generating a key, IV, and auth tag.
 *
 * @param key Pointer to the AES-256-GCM unencrypted key buffer in memory.
 * @param keylen Size of the AES-256-GCM unencrypted key buffer in bytes.
 * @param iv Pointer to the AES-256-GCM IV buffer in memory.
 * @param ivlen Size of the AES-256-GCM IV buffer in bytes.
 * @param at Pointer to the AES-256-GCM auth tag buffer in memory.
 * @param atlen Size of the AES-256-GCM auth tag buffer in bytes.
 * @param pt Pointer to the plaintext payload buffer in memory.
 * @param ct Pointer to the ciphertext payload buffer in memory.
 * @param ptlen Size of the plaintext payload buffer in bytes.
 *
 * @retval 0: HSM crypto successfully encrypted file buffer.
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
    size_t ptlen
);

/**
 * @brief Encrypts a file key while generating an IV and auth tag.
 *
 * @param pkey Pointer to the AES-256-GCM unencrypted key buffer in memory.
 * @param ckey Pointer to the AES-256-GCM encrypted key buffer in memory.
 * @param keylen Size of the AES-256-GCM key buffer in bytes.
 * @param iv Pointer to the AES-256-GCM IV buffer in memory.
 * @param ivlen Size of the AES-256-GCM IV buffer in bytes.
 * @param at Pointer to the AES-256-GCM auth tag buffer in memory.
 * @param atlen Size of the AES-256-GCM auth tag buffer in bytes.
 *
 * @retval 0: HSM crypto successfully encrypted file key.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 5: Root key failed to transfer to the AESADV engine.
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
 * @brief Decrypts a file payload while validating the auth tag.
 *
 * @param key Pointer to the AES-256-GCM unencrypted key buffer in memory.
 * @param keylen Size of the AES-256-GCM unencrypted key buffer in bytes.
 * @param iv Pointer to the AES-256-GCM IV buffer in memory.
 * @param ivlen Size of the AES-256-GCM IV buffer in bytes.
 * @param at Pointer to the AES-256-GCM auth tag buffer in memory.
 * @param atlen Size of the AES-256-GCM auth tag buffer in bytes.
 * @param ct Pointer to the ciphertext payload buffer in memory.
 * @param pt Pointer to the plaintext payload buffer in memory.
 * @param ptlen Size of the ciphertext payload buffer in bytes.
 *
 * @retval 0: HSM crypto successfully decrypted file buffer.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 6: Decrypted output could not be verified according to auth tag.
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
    size_t ctlen
);

/**
 * @brief Encrypts a file key while validating the auth tag.
 *
 * @param ckey Pointer to the AES-256-GCM encrypted key buffer in memory.
 * @param pkey Pointer to the AES-256-GCM unencrypted key buffer in memory.
 * @param keylen Size of the AES-256-GCM key buffer in bytes.
 * @param iv Pointer to the AES-256-GCM IV buffer in memory.
 * @param ivlen Size of the AES-256-GCM IV buffer in bytes.
 * @param at Pointer to the AES-256-GCM auth tag buffer in memory.
 * @param atlen Size of the AES-256-GCM auth tag buffer in bytes.
 *
 * @retval 0: HSM crypto successfully dncrypted file key.
 * @retval 2: Invalid length was given in the parameters.
 * @retval 5: Root key failed to transfer to the AESADV engine.
 * @retval 6: Decrypted output could not be verified according to auth tag.
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