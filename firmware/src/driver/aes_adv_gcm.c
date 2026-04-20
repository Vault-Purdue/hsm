/**
 * @file aes_adv_gcm.c
 * @author Vault team - Purdue
 * @brief AES-256-GCM implementation using the AESADV hardware accelerator
 * @date 2026
 *
 */

#ifdef ENABLE_AESADV_GCM

#include "aes_adv_gcm.h"
#include <string.h>
#include <ti/driverlib/dl_aesadv.h>

/* 128-bit IV buffer (12-byte caller IV || 0x00000001 counter suffix)
 * Must remain 4-byte aligned for hardware access
 */
static __attribute__((aligned(4))) uint8_t iv128[16] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};

/* Aligned staging buffer for 256-bit AES key. */
static __attribute__((aligned(4))) uint8_t key_aligned[32];

/* 16-byte block staging buffer for AES input/output. */
static __attribute__((aligned(4))) uint8_t block[16];

static void force_memset(void *p, int c, size_t n) {
    volatile uint8_t *vp = (volatile uint8_t *)p;
    while (n--) *vp++ = (uint8_t)c;
}

/**
 * @brief Wait until AESADV input is ready
 *
 * @param None
 * @return None
 *
 * Blocks execution until the hardware is ready to accept input data
 */
static void wait_input_ready() {
    while (!DL_AESADV_isInputReady(AESADV));
}

/**
 * @brief Wait until AESADV output data is available
 *
 * @param None
 * @return None
 *
 * Blocks execution until processed output data can be read
 */
static void wait_output_ready() {
    while (!DL_AESADV_isOutputReady(AESADV));
}

/**
 * @brief Load a 256-bit AES key into the hardware engine
 *
 * @param key Pointer to 32-byte AES-256 key
 * @return None
 *
 * Copies the key into an aligned buffer before programming the AESADV
 * hardware to satisfy alignment requirements
 */
static inline void load_key(const uint8_t key[32]) {
    memcpy(key_aligned, key, 32);
    DL_AESADV_setKeyAligned(AESADV, (uint32_t*) key_aligned, DL_AESADV_KEY_SIZE_256_BIT);
}

/**
 * @brief Clear sensitive buffers and hardware key state
 *
 * @param None
 * @return None
 *
 * Wipes key material, IV buffer, and block staging buffer from memory,
 * and reloads a zero key into the AES engine
 */
static inline void clear_data() {
    force_memset(key_aligned, 0, 32);
    force_memset(block, 0, 16);
    force_memset(iv128, 0, 16);
    DL_AESADV_setKeyAligned(AESADV, (uint32_t*) key_aligned, DL_AESADV_KEY_SIZE_256_BIT);
}

/**
 * @brief Configure AESADV for GCM mode operation
 *
 * @param dir        Encryption or decryption direction
 * @param iv         Pointer to 12-byte IV
 * @param aad_len    Length of additional authenticated data in bytes
 * @param data_len   Length of plaintext/ciphertext in bytes
 * @return None
 *
 * Initializes AESADV in autonomous GCM mode with the provided parameters
 */
static void setup_gcm(
    DL_AESADV_DIR dir,
    const uint8_t iv[12],
    size_t aad_len,
    size_t data_len
) {
    memcpy(iv128, iv, 12);
    iv128[12] = 0x00;
    iv128[13] = 0x00;
    iv128[14] = 0x00;
    iv128[15] = 0x01;

    const DL_AESADV_Config cfg = {
        .mode = DL_AESADV_MODE_GCM_AUTONOMOUS,
        .direction = dir,
        .iv = iv128,
        .aadLength  = aad_len,
        .lowerCryptoLength = data_len,
        .upperCryptoLength = 0
    };

    DL_AESADV_initGCM(AESADV, &cfg);
    DL_AESADV_enableSavedOutputContext(AESADV);
}

/**
 * @brief Process data blocks through AESADV
 *
 * @param in   Pointer to input buffer (plaintext or ciphertext)
 * @param out  Pointer to output buffer
 * @param len  Number of bytes to process
 * @return None
 *
 * Processes data in 16-byte blocks, handling partial final blocks
 * according to GCM hardware requirements
 */
static void process_blocks(
    const uint8_t *in,
    uint8_t *out,
    size_t len
) {
    while (len) {
        size_t n = (len >= 16) ? 16 : len;

        for (int i = 0; i < 16; i++) {
            block[i] = (i < n) ? in[i] : 0;
        }

        wait_input_ready();
        DL_AESADV_loadInputDataAligned(AESADV, (uint32_t*) block);

        wait_output_ready();
        DL_AESADV_readOutputDataAligned(AESADV, (uint32_t*) block);

        for (size_t i = 0; i < n; i++)
            out[i] = block[i];

        in  += n;
        out += n;
        len -= n;
    }
}

/**
 * @brief Process Additional Authenticated Data (AAD)
 *
 * @param aad  Pointer to AAD buffer
 * @param len  Length of AAD in bytes
 * @return None
 *
 * Feeds AAD into the GCM engine for authentication without producing output
 */
static void process_aad(const uint8_t *aad, size_t len) {
    if (!aad || !len) return;

    while (len) {
        size_t n = (len >= 16) ? 16 : len;

        for (int i = 0; i < 16; i++)
            block[i] = (i < n) ? aad[i] : 0;

        wait_input_ready();
        DL_AESADV_loadInputDataAligned(AESADV, (uint32_t*) block);

        // AAD does not produce output, no read required

        aad += n;
        len -= n;
    }
}

/**
 * @brief Read the 128-bit GCM authentication tag from hardware
 *
 * @param tag Output buffer for 16-byte authentication tag
 * @return None
 *
 * Blocks until the authentication tag is ready and copies it to the caller
 */
static void read_tag(uint8_t tag[16]) {
    __attribute__((aligned(4))) uint8_t tag_words[16];

    while (!DL_AESADV_isSavedOutputContextReady(AESADV));
    DL_AESADV_readTAGAligned(AESADV, (uint32_t*) tag_words);

    memcpy(tag, tag_words, 16);
}

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
    uint8_t tag[16]
) {
    load_key(key);
    setup_gcm(DL_AESADV_DIR_ENCRYPT, iv, aad_len, len);

    process_aad(aad, aad_len);
    process_blocks(plaintext, ciphertext, len);

    DL_AESADV_haltOperationAndGenerateDigest(AESADV);
    read_tag(tag);

    clear_data();

    return 0;
}

int AESADV_AESGCM256_encryptKey(
    const uint8_t *pkey,
    const uint8_t *ckey,
    size_t keylen,
    const uint8_t iv[12],
    const uint8_t *aad, size_t aad_len,
    uint8_t tag[16]
) {
    setup_gcm(DL_AESADV_DIR_ENCRYPT, iv, aad_len, keylen);

    process_aad(aad, aad_len);
    process_blocks(pkey, ckey, keylen);

    DL_AESADV_haltOperationAndGenerateDigest(AESADV);
    read_tag(tag);

    clear_data();

    return 0;
}

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
    uint8_t *plaintext
) {
    load_key(key);
    setup_gcm(DL_AESADV_DIR_DECRYPT, iv, aad_len, len);

    process_aad(aad, aad_len);
    process_blocks(ciphertext, plaintext, len);

    DL_AESADV_haltOperationAndGenerateDigest(AESADV);

    uint8_t calc_tag[16];
    read_tag(calc_tag);

    // constant-time compare
    uint32_t diff = 0;
    for (int i = 0; i < 16; i++)
        diff |= calc_tag[i] ^ tag[i];

    clear_data();

    return diff == 0 ? 0 : -1;
}

int AESADV_AESGCM256_decryptKey(
    const uint8_t *ckey,
    const uint8_t *pkey,
    size_t keylen,
    const uint8_t iv[12],
    const uint8_t *aad, size_t aad_len,
    uint8_t tag[16]
) {
    uint8_t temp[keylen];
    setup_gcm(DL_AESADV_DIR_DECRYPT, iv, aad_len, len);

    process_aad(aad, aad_len);
    process_blocks(ckey, temp, keylen);

    DL_AESADV_haltOperationAndGenerateDigest(AESADV);

    uint8_t calc_tag[16];
    read_tag(calc_tag);

    // constant-time compare
    uint32_t diff = 0;
    for (int i = 0; i < 16; i++)
        diff |= calc_tag[i] ^ tag[i];

    clear_data();

    if (diff == 0) {
        memcpy(pkey, temp, keylen);
        memset(temp, 0, keylen);
        return 0;
    }

    return -1;
}

/**
 * @brief Initialize and configure the AESADV peripheral
 *
 * @param None
 * @return None
 *
 * Resets the AES module, enables power, and sets key size to 256 bits
 * Must be called before any encryption or decryption operations
 */
void AESADV_init() {
    DL_AESADV_reset(AESADV);
    while (!DL_AESADV_isReset(AESADV));

    DL_AESADV_enablePower(AESADV);
    while (!DL_AESADV_isPowerEnabled(AESADV));

    while (!DL_AESADV_isUserKeyWriteEnabled(AESADV));

    DL_AESADV_setKeySize(AESADV, DL_AESADV_KEY_SIZE_256_BIT);
}

#endif /* ENABLE_AESADV_GCM */