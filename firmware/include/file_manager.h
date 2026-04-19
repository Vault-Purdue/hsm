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

/* Blocks Layout, 128-bit aligned*/
typedef struct __attribute__((aligned(16))) {
      uint32_t status;       
      uint8_t file_id[1];
      uint8_t reserved_align;
      uint16_t payload_size;
      uint8_t iv[12];
      uint8_t auth_tag[16];
      uint32_t reserved;
      uint8_t payload[88];  
 }fm_layout;

/* Key Layout, 128-bit aligned */
typedef struct __attribute__((aligned(16))) {
      uint32_t key_id;
      uint8_t key[32];
      uint32_t reservedkey1;
      uint8_t reservedkey2[40];
 }fm_key_layout;