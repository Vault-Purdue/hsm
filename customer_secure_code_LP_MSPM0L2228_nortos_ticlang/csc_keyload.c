/**
 * @file csc_keyload.c
 * @author Vault Team - Purdue
 * @brief Vault CSC root key load implementation
 * @date 2026
 */

#include "csc_keyload.h"

/* 256-bit (32-byte) vault root key.
 *
 * Placed in the .secret section (mapped to flash 0x4000-0x43FF by the
 * linker script). The CSC reads this once during boot to provision
 * KEYSTORE, then sets up a read-execute-protect firewall over the
 * .secret region before issuing INITDONE. After INITDONE, hardware
 * blocks all reads and instruction fetches to this address range.
 */
 
__attribute__((section(".secret"), aligned(4), used))
static const uint8_t vault_root_key[32] = {
    0x26, 0x8e, 0xd1, 0xb5, 0xd7, 0xc9, 0xc7, 0x30,
    0x4f, 0x9c, 0xae, 0x5f, 0xc4, 0x37, 0xb4, 0xcd,
    0x3a, 0xeb, 0xe2, 0xec, 0x65, 0xf0, 0xd8, 0x5c,
    0x39, 0x18, 0xd3, 0xd3, 0xb5, 0xbb, 0xa8, 0x9b
};

HSM_KEYSTORE_STATUS CSC_loadVaultRootKey(void) {
    return HSM_KEYSTORE_initRootKeyStorage(
        (uint32_t *)(uintptr_t)vault_root_key);
}
