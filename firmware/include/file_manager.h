/**
 * @file file_manager.h
 * @author Vault Team - Purdue
 * @brief file manager implementation
 * @date 2026
 *
 * Wrapper functions for the file manager.
 */
#include <stdalign.h>
#include "file_manager.c"

/* Blocks Layout, 128-bit aligned*/
 typedef struct alignas(16) {
      uint8_t status[4];       
      uint8_t file_id[1];
      uint8_t payload_size[1];
      uint8_t reservedblock1[2];
      uint8_t iv[12];
      uint8_t auth_tag[16];
      uint8_t reservedblock2[4];
      uint8_t payload[88];  
 }fm_layout;

/* Key Layout, 128-bit aligned */
 typedef struct alignas(16) {
      uint8_t key_id[4];
      uint8_t key[32];
      uint8_t reservedkey1[4];
      uint8_t reservedkey2[40];
 }fm_key_layout;