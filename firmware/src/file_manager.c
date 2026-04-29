/**
 * @file file_manager.c
 * @author Vault Team - Purdue
 * @brief File manager: write, read, delete for files and keys
 * @date 2026
 */

#include "file_manager.h"
#include "flash.h"

#define MAX_SLOTS  8   /* one sector = 8 slots of 128B */
#define CRYPTO_AES_KEY_SIZE 128
//#define PIN_BUF_SIZE (PIN_LEN + 1)

/************************ INTERNAL HELPERS ***********************/

/** @brief Find Free File Slot
 *
 * Scans the file sector for a virgin slot (all 0xFF).
 *
 * @return slot index if found, FM_SLOT_NOT_FOUND otherwise
 */
static uint8_t fm_find_free_file_slot(void)
{
    fm_layout slot = {0};
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        flash_read(FLASH_BASE_FILE + (i * FLASH_BLOCK_SIZE), &slot, sizeof(slot));
        if (slot.status == 0xFFFFFFFF) {
            return i;
        }
    }
    return FM_SLOT_NOT_FOUND;
}

/** @brief Find File Slot by ID
 *
 * Scans the file sector for a slot matching the given file_id.
 *
 * @param file_id logical file identifier to search for
 *
 * @return slot index if found, FM_SLOT_NOT_FOUND otherwise
 */
static uint8_t fm_find_file_slot_by_id(uint8_t file_id)
{
    fm_layout slot = {0};
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        flash_read(FLASH_BASE_FILE + (i * FLASH_BLOCK_SIZE), &slot, sizeof(slot));
        if (slot.status == VALID_FILE && slot.file_id == file_id) {
            return i;
        }
    }
    return FM_SLOT_NOT_FOUND;
}

/** @brief Count Active File Slots
 *
 * Returns how many slots currently have status VALID_FILE.
 *
 * @return number of active files
 */
static uint8_t fm_count_active_files(void)
{
    fm_layout slot = {0};
    uint8_t count  = 0;
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        flash_read(FLASH_BASE_FILE + (i * FLASH_BLOCK_SIZE), &slot, sizeof(slot));
        if (slot.status == VALID_FILE) {
            count++;
        }
    }
    return count;
}

/** @brief Find free key slot
 *
 * Scans the key sector for a virgin slot (all 0xFF)
 *
 * @return slot index if found, FM_SLOT_NOT_FOUND otherwise
 */
static uint8_t fm_find_free_key_slot(void)
{
    fm_key_layout slot = {0};
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        flash_read(FLASH_BASE_KEY + (i * FLASH_BLOCK_SIZE), &slot, sizeof(slot));
        if (slot.status == 0xFFFFFFFF) {
            return i;
        }
    }
    return FM_SLOT_NOT_FOUND;
}

/** @brief Find Key Slot by ID
 *
 * Scans the key sector for a slot matching the given file_id
 *
 * @param file_id logical file identifier to search for
 *
 * @return slot index if found, FM_SLOT_NOT_FOUND otherwise
 */
static uint8_t fm_find_key_slot_by_id(uint8_t file_id)
{
    fm_key_layout slot = {0};
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        flash_read(FLASH_BASE_KEY + (i * FLASH_BLOCK_SIZE), &slot, sizeof(slot));
        if (slot.status == VALID_KEY && slot.file_id == file_id) {
            return i;
        }
    }
    return FM_SLOT_NOT_FOUND;
}

/** @brief Crypto Erase File Slot
 *
 * Overwrites target slot with TRNG noise, erases the entire sector,
 * then rewrites all remaining VALID_FILE slots
 *
 * @param slot physical slot index to destroy
 *
 * @return FM_OK on success, FM_ERROR_FLASH on failure
 */
static fm_status_t fm_crypto_erase_file_slot(uint8_t slot)
{
    /* read all slots to RAM */
    fm_layout buf[MAX_SLOTS];
    flash_read(FLASH_BASE_FILE, buf, sizeof(buf));

    /* overwrite target slot with TRNG noise */
    trngGenerateNumber((uint32_t *)&buf[slot], sizeof(fm_layout) / 4);

    /* erase sector */
    if (!flash_erase_page(FLASH_BASE_FILE)) {
        return FM_ERROR_FLASH;
    }

    /* rewrite only VALID slots, skip the erased one */
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        if (i == slot) continue;
        if (buf[i].status == VALID_FILE) {
            if (!flash_write(FLASH_BASE_FILE + (i * FLASH_BLOCK_SIZE),&buf[i], sizeof(fm_layout))) {
                return FM_ERROR_FLASH;
            }
        }
    }

    return FM_OK;
}

/** @brief Crypto Erase Key Slot
 *
 * Same as fm_crypto_erase_file_slot but for the key sector
 * Physically destroys the encrypted DEK
 *
 * @param slot physical slot index to destroy
 *
 * @return FM_OK on success, FM_ERROR_FLASH on failure
 */
static fm_status_t fm_crypto_erase_key_slot(uint8_t slot)
{
    fm_key_layout buf[MAX_SLOTS];
    flash_read(FLASH_BASE_KEY, buf, sizeof(buf));

    trngGenerateNumber((uint32_t *)&buf[slot], sizeof(fm_key_layout) / 4);

    if (!flash_erase_page(FLASH_BASE_KEY)) {
        return FM_ERROR_FLASH;
    }

    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        if (i == slot) continue;
        if (buf[i].status == VALID_KEY) {
            if (!flash_write(FLASH_BASE_KEY + (i * FLASH_BLOCK_SIZE),
                             &buf[i], sizeof(fm_key_layout))) {
                return FM_ERROR_FLASH;
            }
        }
    }

    return FM_OK;
}

/************************ FILES ***********************/

/** @brief Write File
 *
 * Encrypts and writes a file payload to the next available slot.
 * The corresponding DEK must already exist in the key sector before
 * calling this function. Rejects if file_id already exists or if
 * the sector is at maximum capacity (8 active files).
 *
 * @param file_id logical file identifier
 * @param payload pointer to plaintext data to encrypt and store
 * @param size number of bytes in payload, must be 1-88
 *
 * @return FM_OK on success, error code otherwise
 */
fm_status_t fm_write_file(uint8_t file_id, const uint8_t *payload, uint16_t size)
{
    if (payload == NULL) {
        return FM_ERROR_NULL;
    }
    if (size == 0 || size > 88) {
        return FM_ERROR_SIZE;
    }
    if (fm_find_file_slot_by_id(file_id) != FM_SLOT_NOT_FOUND) {
        return FM_ERROR_EXISTS;
    }
    if (fm_count_active_files() >= MAX_SLOTS) {
        return FM_ERROR_FULL;
    }

    uint8_t slot = fm_find_free_file_slot();
    if (slot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_FULL;
    }

    fm_layout file    = {0};
    uint32_t  iv_words[3] = {0};

    file.file_id      = file_id;
    file.payload_size = size;

    trngGenerateNumber(iv_words, 3);
    memcpy(file.iv, iv_words, sizeof(file.iv));

#ifdef CRYPTO_ENABLE
    uint8_t dek[CRYPTO_AES_KEY_SIZE] = {0};
    if (fm_read_key(file_id, dek) != FM_OK) {
        return FM_ERROR_CRYPTO;
    }

    crypto_status_t cstatus = crypto_gcm_encrypt(
        dek,
        file.iv,
        &file_id,
        sizeof(file_id),
        payload,
        size,
        file.payload,
        file.auth_tag
    );

    memset(dek, 0, sizeof(dek));

    if (cstatus != CRYPTO_OK) {
        return FM_ERROR_CRYPTO;
    }
#else
    memcpy(file.payload, payload, size);
#endif

    file.status = VALID_FILE;

    uint32_t addr = FLASH_BASE_FILE + (slot * FLASH_BLOCK_SIZE);
    return flash_write(addr, &file, sizeof(fm_layout)) ? FM_OK : FM_ERROR_FLASH;
}

/** @brief Read File
 *
 * Reads and decrypts a file payload into the provided output buffer
 * Caller must provide a buffer of at least 88 bytes
 *
 * @param file_id logical file identifier to read
 * @param out_buf pointer to output buffer (minimum 88 bytes)
 *
 * @return FM_OK on success, error code otherwise
 */
fm_status_t fm_read_file(uint8_t file_id, uint8_t *out_buf)
{
    if (out_buf == NULL) {
        return FM_ERROR_NULL;
    }

    uint8_t slot = fm_find_file_slot_by_id(file_id);
    if (slot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_NOT_FOUND;
    }

    fm_layout file = {0};
    flash_read(FLASH_BASE_FILE + (slot * FLASH_BLOCK_SIZE), &file, sizeof(file));

#ifdef CRYPTO_ENABLE
    uint8_t dek[CRYPTO_AES_KEY_SIZE] = {0};
    if (fm_read_key(file_id, dek) != FM_OK) {
        return FM_ERROR_CRYPTO;
    }

    crypto_status_t cstatus = crypto_gcm_decrypt(
        dek,
        file.iv,
        &file_id,
        sizeof(file_id),
        file.payload,
        sizeof(file.payload),
        out_buf,
        file.auth_tag
    );

    memset(dek, 0, sizeof(dek));

    if (cstatus != CRYPTO_OK) {
        return FM_ERROR_CRYPTO;
    }
#else
    memcpy(out_buf, file.payload, sizeof(file.payload));
#endif

    return FM_OK;
}

/** @brief Delete File
 *
 * Physically destroys the file and its associated key using crypto erase
 * Both slots are overwritten with TRNG noise before the sector erase so
 * the ciphertext and DEK are unrecoverable.
 *
 * @param file_id logical file identifier to delete
 *
 * @return FM_OK on success, error code otherwise
 */
fm_status_t fm_delete_file(uint8_t file_id)
{
    uint8_t file_slot = fm_find_file_slot_by_id(file_id);
    if (file_slot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_NOT_FOUND;
    }

    /* destroy file first */
    fm_status_t status = fm_crypto_erase_file_slot(file_slot);
    if (status != FM_OK) {
        return status;
    }

    /* destroy key — if missing, file is already gone so still return OK */
    uint8_t key_slot = fm_find_key_slot_by_id(file_id);
    if (key_slot != FM_SLOT_NOT_FOUND) {
        status = fm_crypto_erase_key_slot(key_slot);
    }

    return status;
}

/************************ KEYS ***********************/

/** @brief Write Key
 *
 * Encrypts a DEK with the KEK and stores it in the next available key slot.
 * Must be called before fm_write_file for the same file_id.
 *
 * @param file_id logical file identifier this key belongs to
 * @param dek pointer to plaintext DEK (must be CRYPTO_AES_KEY_SIZE bytes)
 * @param size size of DEK in bytes, must equal CRYPTO_AES_KEY_SIZE
 *
 * @return FM_OK on success, error code otherwise
 */
fm_status_t fm_write_key(uint8_t file_id, const uint8_t *dek, uint16_t size)
{
    if (dek == NULL) {
        return FM_ERROR_NULL;
    }
    if (size != CRYPTO_AES_KEY_SIZE) {
        return FM_ERROR_SIZE;
    }
    if (fm_find_key_slot_by_id(file_id) != FM_SLOT_NOT_FOUND) {
        return FM_ERROR_EXISTS;
    }

    uint8_t slot = fm_find_free_key_slot();
    if (slot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_FULL;
    }

    fm_key_layout key     = {0};
    uint8_t       kek[CRYPTO_AES_KEY_SIZE] = {0};
    uint32_t      iv_words[3] = {0};

    key.file_id = file_id;
    trngGenerateNumber(iv_words, 3);
    memcpy(key.iv, iv_words, sizeof(key.iv));

#ifdef CRYPTO_ENABLE
    if (km_get_kek(kek) != KM_OK) {
        memset(kek, 0, sizeof(kek));
        return FM_ERROR_CRYPTO;
    }

    crypto_status_t cstatus = crypto_gcm_encrypt(
        kek,
        key.iv,
        &file_id,
        sizeof(file_id),
        dek,
        size,
        key.encrypted_dek,
        key.auth_tag
    );

    memset(kek, 0, sizeof(kek));

    if (cstatus != CRYPTO_OK) {
        return FM_ERROR_CRYPTO;
    }
#else
    memcpy(key.encrypted_dek, dek, size);
#endif

    key.status = VALID_KEY;

    uint32_t addr = FLASH_BASE_KEY + (slot * FLASH_BLOCK_SIZE);
    return flash_write(addr, &key, sizeof(fm_key_layout)) ? FM_OK : FM_ERROR_FLASH;
}

/** @brief Read Key
 *
 * Reads and decrypts a DEK from the key sector into the output buffer.
 * Typically called internally by fm_read_file and fm_write_file.
 *
 * @param file_id logical file identifier whose key to read
 * @param out_dek pointer to output buffer (must be CRYPTO_AES_KEY_SIZE bytes)
 *
 * @return FM_OK on success, error code otherwise
 */
fm_status_t fm_read_key(uint8_t file_id, uint8_t *out_dek)
{
    if (out_dek == NULL) {
        return FM_ERROR_NULL;
    }

    uint8_t slot = fm_find_key_slot_by_id(file_id);
    if (slot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_NOT_FOUND;
    }

    fm_key_layout key = {0};
    flash_read(FLASH_BASE_KEY + (slot * FLASH_BLOCK_SIZE), &key, sizeof(key));

#ifdef CRYPTO_ENABLE
    uint8_t kek[CRYPTO_AES_KEY_SIZE] = {0};
    if (km_get_kek(kek) != KM_OK) {
        memset(kek, 0, sizeof(kek));
        return FM_ERROR_CRYPTO;
    }

    crypto_status_t cstatus = crypto_gcm_decrypt(
        kek,
        key.iv,
        &file_id,
        sizeof(file_id),
        key.encrypted_dek,
        CRYPTO_AES_KEY_SIZE,
        out_dek,
        key.auth_tag
    );

    memset(kek, 0, sizeof(kek));

    if (cstatus != CRYPTO_OK) {
        return FM_ERROR_CRYPTO;
    }
#else
    memcpy(out_dek, key.encrypted_dek, CRYPTO_AES_KEY_SIZE);
#endif

    return FM_OK;
}

fm_status_t fm_read_pin(unsigned char *pin_buffer, size_t buffer_size) {
    const unsigned char expected_pin[32] = {
        0x3D, 0xC1, 0x71, 0x7E, 0x8E, 0xD8, 0x20, 0x92, 
        0x1E, 0xE4, 0x79, 0x42, 0x32, 0xEB, 0x11, 0xD6, 
        0x1C, 0x2E, 0x5C, 0xE7, 0xDE, 0xCC, 0xF7, 0xC5, 
        0xD6, 0x99, 0xC8, 0x2B, 0xE2, 0x4F, 0x8B, 0x79
    };

    if (pin_buffer == NULL || buffer_size != 32) {
        return FM_ERROR;
    }

    memcpy(pin_buffer, expected_pin, 32);
    return FM_OK;
}

fm_status_t fm_write_pin(const char *pin) {
    (void)pin;
    return FM_OK;
}