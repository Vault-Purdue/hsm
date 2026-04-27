/**
 * @file file_manager.c
 * @author Vault Team - Purdue
 * @brief File manager: write, read, delete for files and keys
 * @date 2026
 */

#include "file_manager.h"
#include "flash.h"
#include "crypto_module.h"
#include "uart_cmd_router.h"
#include <stdbool.h>

#define MAX_SLOTS  8   /* one sector = 8 slots of 128B */
#define CRYPTO_AES_KEY_SIZE 128
//#define PIN_BUF_SIZE (PIN_LEN + 1)

static struct {
    bool active;
    uint8_t direction;
    uint8_t file_id;
} fm_pending = {0};

/************************ INTERNAL HELPERS ***********************/

/** @brief Initialize File Manager
 *
 * Scans flash sectors and reports active file_ids over UART for debug
 * No state is built in RAM — lookups remain lazy. This is purely
 * informational at boot time.
 *
 * @return FM_OK always (informational only)
 */
fm_status_t init_fm(void)
{
    fm_layout slot = {0};
    uint8_t active = 0;

    uart_send_debug_msg("FM: Scanning file slots...");

    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        flash_read(FLASH_BASE_FILE + (i * FLASH_BLOCK_SIZE),
                   &slot, sizeof(slot));

        if (slot.status == VALID_FILE) {
            uart_send_debug_msg_with_error_code("FM: Found file_id", slot.file_id);
            active++;
        }
    }

    uart_send_debug_msg_with_error_code("FM: Active files", active);
    return FM_OK;
}

/** @brief Find Free File Slot
 *
 * Scans the file sector for a virgin slot (all 0xFF)
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
 * Scans the file sector for a slot matching the given file_id
 *
 * @param file_id logical file identifier to search for
 *
 * @return slot index if found, FM_SLOT_NOT_FOUND otherwise
 */
static uint8_t fm_find_file_slot_by_id(uint8_t file_id) {
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
 * Returns how many slots currently have status VALID_FILE
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
 * When CRYPTO_ENABLE is defined, generates a DEK internally, encrypts
 * the file, and stores the encrypted DEK in the key sector automatically
 * Rejects if file_id already exists or if the sector is at maximum capacity
 *
 * @param file_id logical file identifier
 * @param payload pointer to plaintext data to encrypt and store
 * @param size number of bytes in payload, must be 1-88
 *
 * @return FM_OK on success, error code otherwise
 */
fm_status_t fm_write_file(uint8_t file_id, const uint8_t *payload, uint16_t size) {
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

    fm_layout file = {0};

    file.file_id      = file_id;
    file.payload_size = size;

#ifdef CRYPTO_ENABLE
    uint8_t dek[CRYPTO_AES_KEY_SIZE]= {0};
    uint8_t enc_dek[CRYPTO_AES_KEY_SIZE] = {0};
    uint8_t key_iv[CRYPTO_GCM_IV_SIZE] = {0};
    uint8_t key_at[CRYPTO_GCM_TAG_SIZE] = {0};

    /* Generates DEK and IV internally, encrypts file payload */
    HSM_CRYPTO_STATUS cstatus = HSM_CRYPTO_encryptFile(
        dek,           CRYPTO_AES_KEY_SIZE,
        file.iv,       CRYPTO_GCM_IV_SIZE,
        file.auth_tag, CRYPTO_GCM_TAG_SIZE,
        payload,
        file.payload,
        size
    );
    if (cstatus != HSM_CRYPTO_OK) {
        memset(dek, 0, sizeof(dek));
        return FM_ERROR_CRYPTO;
    }

    /* Encrypt the DEK with the root key (handled internally by crypto module) */
    cstatus = HSM_CRYPTO_encryptFileKey(
        dek,    enc_dek, CRYPTO_AES_KEY_SIZE,
        key_iv, CRYPTO_GCM_IV_SIZE,
        key_at, CRYPTO_GCM_TAG_SIZE
    );
    memset(dek, 0, sizeof(dek));   /* clear DEK from RAM immediately */
    if (cstatus != HSM_CRYPTO_OK) {
        return FM_ERROR_CRYPTO;
    }

    /* Store the encrypted DEK in the key sector */
    uint8_t kslot = fm_find_free_key_slot();
    if (kslot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_FULL;
    }

    fm_key_layout key_entry = {0};
    key_entry.file_id = file_id;
    key_entry.status  = VALID_KEY;
    memcpy(key_entry.encrypted_dek, enc_dek, CRYPTO_AES_KEY_SIZE);
    memcpy(key_entry.iv,            key_iv,  CRYPTO_GCM_IV_SIZE);
    memcpy(key_entry.auth_tag,      key_at,  CRYPTO_GCM_TAG_SIZE);

    if (!flash_write(FLASH_BASE_KEY + (kslot * FLASH_BLOCK_SIZE),
                     &key_entry, sizeof(fm_key_layout))) {
        return FM_ERROR_FLASH;
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
 * When CRYPTO_ENABLE is defined, retrieves and decrypts the DEK from
 * the key sector, then decrypts the file payload
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
    /* Load the key slot for this file */
    uint8_t kslot = fm_find_key_slot_by_id(file_id);
    if (kslot == FM_SLOT_NOT_FOUND) {
        return FM_ERROR_NOT_FOUND;
    }

    fm_key_layout key_entry = {0};
    flash_read(FLASH_BASE_KEY + (kslot * FLASH_BLOCK_SIZE), &key_entry, sizeof(key_entry));

    /* Decrypt the DEK using the root key (handled internally by crypto module) */
    uint8_t dek[CRYPTO_AES_KEY_SIZE] = {0};
    HSM_CRYPTO_STATUS cstatus = HSM_CRYPTO_decryptFileKey(
        key_entry.encrypted_dek, dek, CRYPTO_AES_KEY_SIZE,
        key_entry.iv,            CRYPTO_GCM_IV_SIZE,
        key_entry.auth_tag,      CRYPTO_GCM_TAG_SIZE
    );
    if (cstatus != HSM_CRYPTO_OK) {
        memset(dek, 0, sizeof(dek));
        return FM_ERROR_CRYPTO;
    }

    /* Decrypt the file payload using the DEK */
    cstatus = HSM_CRYPTO_decryptFile(
        dek,           CRYPTO_AES_KEY_SIZE,
        file.iv,       CRYPTO_GCM_IV_SIZE,
        file.auth_tag, CRYPTO_GCM_TAG_SIZE,
        file.payload,
        out_buf,
        sizeof(file.payload)
    );
    memset(dek, 0, sizeof(dek));   /* clear DEK from RAM immediately */
    if (cstatus != HSM_CRYPTO_OK) {
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

    /* destroy key if missing, file is already gone so still return OK */
    uint8_t key_slot = fm_find_key_slot_by_id(file_id);
    if (key_slot != FM_SLOT_NOT_FOUND) {
        status = fm_crypto_erase_key_slot(key_slot);
    }

    return status;
}

/************************ KEYS ***********************/

/** @brief Write Key
 *
 * Encrypts a DEK with the root key and stores it in the next available key slot.
 * Note: in normal write flow this is handled internally by fm_write_file.
 * This function is exposed for manual provisioning use cases.
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

    fm_key_layout key = {0};

    key.file_id = file_id;

#ifdef CRYPTO_ENABLE
    /* KEK is handled internally by crypto module */
    HSM_CRYPTO_STATUS cstatus = HSM_CRYPTO_encryptFileKey(
        dek,
        key.encrypted_dek, 
        CRYPTO_AES_KEY_SIZE,
        key.iv,            
        CRYPTO_GCM_IV_SIZE,
        key.auth_tag,      
        CRYPTO_GCM_TAG_SIZE
    );

    if (cstatus != HSM_CRYPTO_OK) {
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
    /* KEK is handled internally by crypto module */
    HSM_CRYPTO_STATUS cstatus = HSM_CRYPTO_decryptFileKey(
        key.encrypted_dek, out_dek, CRYPTO_AES_KEY_SIZE,
        key.iv,            CRYPTO_GCM_IV_SIZE,
        key.auth_tag,      CRYPTO_GCM_TAG_SIZE
    );

    if (cstatus != HSM_CRYPTO_OK) {
        return FM_ERROR_CRYPTO;
    }
#else
    memcpy(out_dek, key.encrypted_dek, CRYPTO_AES_KEY_SIZE);
#endif

    return FM_OK;
}

fm_status_t fm_read_pin(unsigned char *pin_buffer, size_t buffer_size) {
    if (pin_buffer == NULL || buffer_size != PIN_LEN) {
        return FM_ERROR;
    }

    memcpy(pin_buffer, "123456", PIN_LEN);
    return FM_OK;
}

fm_status_t fm_write_pin(const char *pin) {
    (void)pin;
    return FM_OK;
}

router_status_t fm_file_transfer_request(uint8_t direction, uint8_t file_id) {
    uint8_t ack_payload;
    bool failed = false;

    if (direction == FM_DIR_READ) {
        if (fm_find_file_slot_by_id(file_id) == FM_SLOT_NOT_FOUND) failed = true;
    } else if (direction == FM_DIR_WRITE) {
        if (fm_find_file_slot_by_id(file_id) != FM_SLOT_NOT_FOUND) failed = true;
        else if (fm_count_active_files() >= MAX_SLOTS) failed = true;
    } else {
        failed = true;
    }

    if (failed) {
        ack_payload = 0x01;
        uart_send_frame(MSG_FILE_REQUEST_ACK, &ack_payload, 1);
        fm_pending.active = false;
        return RT_FAIL;
    }

    fm_pending.active    = true;
    fm_pending.direction = direction;
    fm_pending.file_id   = (uint8_t)file_id;

    ack_payload = 0x00;
    uart_send_frame(MSG_FILE_REQUEST_ACK, &ack_payload, 1);

    if (direction == FM_DIR_READ) {
        uint8_t buf[FM_MAX_PAYLOAD_SIZE] = {0};
        if (fm_read_file(file_id, buf) == FM_OK) {
            uart_send_frame(MSG_FILE_CONTENTS, buf, FM_MAX_PAYLOAD_SIZE);
        }
        fm_pending.active = false;
    }

    return RT_OK;
}

router_status_t fm_handle_file_contents(const uint8_t *payload, uint8_t len) {
    uint8_t ack;

    if (!fm_pending.active || fm_pending.direction != FM_DIR_WRITE) {
        ack = 0x01;
        uart_send_frame(MSG_FILE_TRANSFER_COMPLETE_ACK, &ack, 1);
        return RT_FAIL;
    }

    fm_status_t status = fm_write_file(fm_pending.file_id, payload, len);
    fm_pending.active = false;

    ack = (status == FM_OK) ? 0x00 : 0x01;
    uart_send_frame(MSG_FILE_TRANSFER_COMPLETE_ACK, &ack, 1);
    return (status == FM_OK) ? RT_OK : RT_FAIL;
}