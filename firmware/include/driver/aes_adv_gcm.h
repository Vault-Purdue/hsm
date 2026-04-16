/**
 * @file aes-adv_gcm.h
 * @author Vault Team - Purdue
 * @brief AES-256-GCM implementation using the AESADV hardware accelerator
 * @date 2026
 *
 */

#ifndef AESADV_GCM_H
#define AESADV_GCM_H

#ifdef ENABLE_AESADV_GCM

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize and power up AESADV in 256-bit key mode
 *
 * Resets peripheral, enables power, and configures key size
 */
void AESADV_init();

/**
 * @brief Perform AES-256-GCM authenticated encryption
 *
 * @param key         Pointer to 32-byte AES-256 key
 * @param iv          Pointer to 12-byte IV
 * @param aad         Pointer to additional authenticated data
 * @param aad_len     Length of AAD in bytes
 * @param plaintext   Pointer to plaintext buffer
 * @param len         Length of plaintext in bytes
 * @param ciphertext  Output buffer for ciphertext
 * @param tag         Output buffer for 16-byte authentication tag
 *
 * @return 0 on success
 *
 * Encrypts plaintext and generates an authentication tag
 * Caller must ensure IV uniqueness for a given key
 */
int AESADV_AESGCM256_encrypt(
    const uint8_t key[32],
    const uint8_t iv[12],
    const uint8_t *aad, size_t aad_len,
    const uint8_t *plaintext, size_t len,
    uint8_t *ciphertext,
    uint8_t tag[16]);

/**
 * @brief Perform AES-256-GCM authenticated decryption
 *
 * @param key         Pointer to 32-byte AES-256 key
 * @param iv          Pointer to 12-byte IV used during encryption
 * @param aad         Pointer to additional authenticated data
 * @param aad_len     Length of AAD in bytes
 * @param ciphertext  Pointer to ciphertext buffer
 * @param len         Length of ciphertext in bytes
 * @param tag         Pointer to expected 16-byte authentication tag
 * @param plaintext   Output buffer for decrypted plaintext
 *
 * @return 0 if authentication succeeds, -1 if tag verification fails
 *
 * Decrypts ciphertext and verifies authentication tag in constant time
 * Caller must discard plaintext if return value is non-zero
 */
int AESADV_AESGCM256_decrypt(
    const uint8_t key[32],
    const uint8_t iv[12],
    const uint8_t *aad, size_t aad_len,
    const uint8_t *ciphertext, size_t len,
    const uint8_t tag[16],
    uint8_t *plaintext);

#endif /* HAVE_AESADV_AESGCM */

#endif /* AESADV_GCM_H */