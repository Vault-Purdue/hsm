#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

#define WOLFSSL_USER_SETTINGS
#define WOLFCRYPT_ONLY

#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_WRITEV

#define NO_RSA
#define NO_DH
#define NO_ECC
#define NO_AES
#define NO_DES3
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_ASN
#define NO_DSA

#define HAVE_CURVE25519
#define HAVE_SHA256
#define HAVE_HKDF

#define WOLFSSL_SHA256
#define WOLFSSL_HMAC

#define WOLFSSL_SP_MATH
#define WOLFSSL_SP_SMALL

#define CUSTOM_RAND_GENERATE_BLOCK HSM_CRYPTO_useCryptoRNG
#define NO_DEV_RANDOM
#define NO_ERROR_STRINGS
#define NO_INLINE

#endif