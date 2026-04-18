/**
 * @file crypto_module.h
 * @author Vault Team - Purdue
 * @brief Crypto Module - AES-128-GCM authenticated encryption
 * @date 2026
 *
 * Thin wrapper over the AESADV and TRNG drivers. Provides per-chunk
 * AES-128-GCM encrypt/decrypt for the File Manager, with the chunk's
 * File ID and Chunk Index bound into the AAD to prevent reordering
 * attacks.
 *
 * Designed for the file chunk layout:
 *   File ID (2B) | Chunk Index (2B) | IV (12B) | Auth Tag (16B) |
 *   Payload Size (2B) | Reserved (2B) | Data Payload (92B)
 */

#ifndef __CRYPTO_MODULE__
#define __CRYPTO_MODULE__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


/* ============================================================== */
/* CONSTANTS */

#define CRYPTO_AES_KEY_SIZE  16 // AES-128
#define CRYPTO_GCM_IV_SIZE   12 // 96-bit IV
#define CRYPTO_GCM_TAG_SIZE  16 // 128-bit tag
#define CRYPTO_CHUNK_PAYLOAD 92 // Max bytes per file chunk

/* ============================================================== */
/* ERROR CODES */

typedef enum crypto_status {
    CRYPTO_OK = 0,
    CRYPTO_ERR_NULL_PARAM,
    CRYPTO_ERR_BAD_LENGTH,
    CRYPTO_ERR_HW_FAIL, // AESADV driver returned an error
    CRYPTO_ERR_TRNG_FAIL, // TRNG driver returned an error
    CRYPTO_ERR_AUTH_FAIL // GCM tag mismatch on decrypt
} crypto_status;

/* ============================================================== */
// FUNCTIONS

// Initialize the module. Call once at startup.
crypto_status crypto_init(void);

// Fill a buffer with random bytes from the TRNG.
crypto_status crypto_random(uint8_t *out, uint32_t len);

// Generate a fresh 12-byte GCM IV.
crypto_status crypto_generate_iv(uint8_t iv[CRYPTO_GCM_IV_SIZE]);


// AES-128-GCM encrypt. 
crypto_status crypto_gcm_encrypt(
    const uint8_t key[CRYPTO_AES_KEY_SIZE],
    const uint8_t iv[CRYPTO_GCM_IV_SIZE],
    const uint8_t *aad, uint32_t aad_len,
    const uint8_t *pt, uint32_t pt_len,
    uint8_t *ct,
    uint8_t tag[CRYPTO_GCM_TAG_SIZE]);

// AES-128-GCM decrypt
crypto_status crypto_gcm_decrypt(
    const uint8_t key[CRYPTO_AES_KEY_SIZE],
    const uint8_t iv[CRYPTO_GCM_IV_SIZE],
    const uint8_t *aad, uint32_t aad_len,
    const uint8_t *ct, uint32_t ct_len,
    const uint8_t tag[CRYPTO_GCM_TAG_SIZE],
    uint8_t *pt);

#endif
/* ============================================================== */