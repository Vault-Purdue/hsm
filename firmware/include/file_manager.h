/**
 * @file file_manager.h
 * @author Vault Team - Purdue
 * @brief file manager implementation
 * @date 2026
 *
 * Wrapper functions for the file manager.
 */
#include <stdlib.h>
#include <stdint.h>

#define FLASH_BLOCK_SIZE  128
#define FLASH_SECTOR_SIZE 1024
#define VALID_KEY  0xDEADAAAA
#define VALID_FILE 0xDEADFFFF

// TODO: Define valid memory addresses
#define FLASH_BASE_KEY    0x00010000 
#define FLASH_BASE_FILE   0x00010400


#define FM_MAX_PAYLOAD_SIZE 88

/* Blocks Layout, 128-byte aligned*/
typedef struct __attribute__((packed, aligned(16))) {
    uint32_t status;
    uint8_t  file_id;
    uint8_t  reserved_0[3];
    uint8_t  iv[12];
    uint8_t  auth_tag[16];
    uint16_t payload_size;
    uint8_t  reserved_1[2];
    uint8_t  payload[88];
} fm_layout;

typedef struct __attribute__((packed, aligned(16))) {
    uint32_t status;
    uint8_t  file_id;
    uint8_t  reserved1[3];
    uint8_t  encrypted_dek[32];
    uint8_t  iv[12];
    uint8_t  auth_tag[16];
    uint8_t  reserved2[60];
} fm_key_layout;

 typedef enum {
      FM_WR_SUCESS,
      FM_RD_SUCESS,
      FM_WR_ERROR,
      FM_RD_ERROR,
      FM_ERROR_NULL,
      FM_ERROR_SIZE
 } fm_status_t;

//TODO: Add descriptions
fm_status_t fm_write_file(uint8_t file_id, const uint8_t* payload, uint16_t size);
fm_status_t fm_read_file(uint8_t file_id, uint8_t *out_buf);
fm_status_t fm_delete_file(uint8_t file_id);

fm_status_t fm_write_key(uint8_t file_id, const uint8_t *dek, uint16_t size);
fm_status_t fm_read_key(uint8_t file_id, uint8_t *out_dek);