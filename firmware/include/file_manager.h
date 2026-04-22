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

#define FLASH_BLOCK_SIZE 128
#define FLASH_SECTOR_SIZE 1024

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

/* Key Layout, 128-byte aligned */
typedef struct __attribute__((packed, aligned(16))) {
      uint32_t key_id;
      uint8_t key[32];
      uint8_t  iv[12];
      uint8_t  auth_tag[16];
      uint8_t reserved[64];
 }fm_key_layout;