#include "crypto_module_test.h"

// Curve25519 test from RFC-7748
static uint8_t alice_private[32] = {
    0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d,
    0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45,
    0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a,
    0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a
};
static uint8_t alice_public[32] = {
    0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54,
    0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a,
    0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4,
    0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a
};
static uint8_t bob_public[32] = {
    0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4,
    0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37,
    0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d,
    0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f
};
static uint8_t shared_alice[32] = {
    0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1,
    0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25,
    0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33,
    0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42
};
static uint8_t aes_key[32] = {
    0x8c, 0x7f, 0x8f, 0x5f, 0x20, 0xf7, 0xf4, 0xef,
    0x58, 0xb7, 0x50, 0xa0, 0xf1, 0xf1, 0x62, 0x9f,
    0xfe, 0xdb, 0xa4, 0x6b, 0x7b, 0xe8, 0xef, 0x39,
    0x0a, 0xdf, 0xa1, 0xe6, 0xff, 0xeb, 0xdd, 0xab
};
static uint8_t aes_iv[12] = {
    0x2f, 0x4e, 0xeb, 0xe9,
    0xb7, 0xd4, 0x3c, 0x6d,
    0x76, 0x41, 0x44, 0xfb
};
static const size_t payload_size = 20;
static uint8_t ct_payload[payload_size] = {
    0xbb, 0xb5, 0xe3, 0x9e, 0xc1,
    0xb1, 0x9f, 0x8a, 0x14, 0x1c,
    0x70, 0x10, 0x47, 0x7e, 0x7d,
    0x41, 0xe0, 0x2a, 0x4a, 0x2b
};
static uint8_t aes_auth_tag[16] = {
    0x35, 0xec, 0x23, 0xad,
    0x64, 0x54, 0xf9, 0xae,
    0x16, 0x89, 0x82, 0x44,
    0xe2, 0x6a, 0xad, 0x72
};

// NIST AES-256-GCM test vectors for file key encryption
static uint8_t pt_aes_key[32] = {
    0xfe, 0x29, 0xa4, 0x0d, 0x8e, 0xbf, 0x57, 0x26,
    0x2b, 0xdb, 0x87, 0x19, 0x1d, 0x01, 0x84, 0x3f,
    0x4c, 0xa4, 0xb2, 0xde, 0x97, 0xd8, 0x82, 0x73,
    0x15, 0x4a, 0x0b, 0x7d, 0x9e, 0x2f, 0xdb, 0x80
};

bool HSM_CRYPTOTEST_sessionTest(void) {

    // Initialize buffers
    uint8_t temp_alice_public[CRYPTO_AES_KEY_SIZE] = {0};
    uint8_t temp_shared_alice[CRYPTO_AES_KEY_SIZE] = {0};
    uint8_t temp_aes_key[CRYPTO_AES_KEY_SIZE] = {0};
    uint8_t temp_aes_iv[CRYPTO_GCM_IV_SIZE] = {0};
    
    // Load Alice private key
    HSM_CRYPTO_STATUS status = HSM_CRYPTO_loadPrivateSessionKey(alice_private, 32);
    if (status != HSM_CRYPTO_OK) return false;
    
    // Test that the public key is correctly generated
    status = HSM_CRYPTO_generatePublicSessionKey(alice_private, temp_alice_public, CRYPTO_AES_KEY_SIZE);
    if (status != HSM_CRYPTO_OK) return false;
    if (memcmp(temp_alice_public, alice_public, CRYPTO_AES_KEY_SIZE) != 0) return false;

    // Test that the shared value is correctly generated
    status = HSM_CRYPTO_loadClientPublicKey(bob_public, 32);
    if (status != HSM_CRYPTO_OK) return false;
    status = HSM_CRYPTO_generateSharedSecret(temp_shared_alice, CRYPTO_AES_KEY_SIZE);
    if (status != HSM_CRYPTO_OK) return false;
    if (memcmp(temp_shared_alice, shared_alice, CRYPTO_AES_KEY_SIZE) != 0) return false;

    // Test that the derived key and iv are correct given info "aes-key" and "iv" respectively
    status = HSM_CRYPTO_deriveKeyAndIV(shared_alice, temp_aes_key, CRYPTO_AES_KEY_SIZE, temp_aes_iv, CRYPTO_GCM_IV_SIZE);
    if (status != HSM_CRYPTO_OK)return false;
    if (memcmp(temp_aes_key, aes_key, CRYPTO_AES_KEY_SIZE) != 0) return false;
    if (memcmp(temp_aes_iv, aes_iv, CRYPTO_GCM_IV_SIZE) != 0) return false;

    return true;
}

bool HSM_CRYPTOTEST_fileKeyEncryptionTest(void) {

    // Initialize buffers
    uint8_t temp_ct_aes_key[CRYPTO_AES_KEY_SIZE] = {0};
    uint8_t temp_pt_aes_key[CRYPTO_AES_KEY_SIZE] = {0};
    uint8_t temp_aes_iv[CRYPTO_GCM_IV_SIZE] = {0};
    uint8_t temp_aes_at[CRYPTO_GCM_TAG_SIZE] = {0};

    // Encrypt key
    HSM_CRYPTO_STATUS status = HSM_CRYPTO_encryptFileKey(
        pt_aes_key,
        temp_ct_aes_key,
        CRYPTO_AES_KEY_SIZE,
        temp_aes_iv,
        CRYPTO_GCM_IV_SIZE,
        temp_aes_at,
        CRYPTO_GCM_TAG_SIZE
    );
    if (status != HSM_CRYPTO_OK) return false;

    // Decrypt key
    status = HSM_CRYPTO_decryptFileKey(
        temp_ct_aes_key,
        temp_pt_aes_key,
        CRYPTO_AES_KEY_SIZE,
        temp_aes_iv,
        CRYPTO_GCM_IV_SIZE,
        temp_aes_at,
        CRYPTO_GCM_TAG_SIZE
    );
    if (status != HSM_CRYPTO_OK) return false;

    // Compare the temp plaintext key and our original key
    if (memcmp(temp_pt_aes_key, pt_aes_key, CRYPTO_AES_KEY_SIZE) != 0) return false;

    return true;
}

bool HSM_CRYPTOTEST_messagePayloadEncryptionTest(void) {
    
    // Initialize buffers
    const char *plaintext = "This is a test file.";
    uint8_t temp_ct_payload[payload_size] = {0};
    uint8_t temp_aes_at[CRYPTO_GCM_TAG_SIZE] = {0};
    uint8_t temp_pt_payload[payload_size];
    memcpy(temp_pt_payload, (const uint8_t *)plaintext, payload_size);

    // Encrypt the message payload
    HSM_CRYPTO_STATUS status = HSM_CRYPTO_encryptCommandPayload(
        aes_key,
        CRYPTO_AES_KEY_SIZE,
        aes_iv,
        CRYPTO_GCM_IV_SIZE,
        temp_aes_at,
        CRYPTO_GCM_TAG_SIZE,
        temp_pt_payload,
        temp_ct_payload,
        payload_size
    );
    if (status != HSM_CRYPTO_OK) return false;
    //if (memcmp(temp_ct_payload, ct_payload, payload_size) != 0) return false;
    //if (memcmp(temp_aes_at, aes_auth_tag, CRYPTO_GCM_TAG_SIZE) != 0) return false;

    // Clear the plaintext buffer
    memset(temp_pt_payload, 0, payload_size);

    // Decrypt the message payload
    status = HSM_CRYPTO_decryptCommandPayload(
        aes_key,
        CRYPTO_AES_KEY_SIZE,
        aes_iv,
        CRYPTO_GCM_IV_SIZE,
        temp_aes_at,
        CRYPTO_GCM_TAG_SIZE,
        temp_ct_payload,
        temp_pt_payload,
        payload_size
    );
    if (status != HSM_CRYPTO_OK) return false;
    if (strncmp((const char *)temp_pt_payload, plaintext, payload_size) != 0) return false;

    return true;
}