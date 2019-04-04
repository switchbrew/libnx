/**
 * @file aes.h
 * @brief Hardware accelerated AES-ECB implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

#ifndef AES_BLOCK_SIZE
#define AES_BLOCK_SIZE 0x10
#endif
#ifndef AES_128_KEY_SIZE
#define AES_128_KEY_SIZE   0x10
#endif
#ifndef AES_128_U32_PER_KEY
#define AES_128_U32_PER_KEY (AES_128_KEY_SIZE / sizeof(u32))
#endif
#ifndef AES_128_NUM_ROUNDS
#define AES_128_NUM_ROUNDS 10
#endif
#ifndef AES_192_KEY_SIZE
#define AES_192_KEY_SIZE   0x18
#endif
#ifndef AES_192_U32_PER_KEY
#define AES_192_U32_PER_KEY (AES_192_KEY_SIZE / sizeof(u32))
#endif
#ifndef AES_192_NUM_ROUNDS
#define AES_192_NUM_ROUNDS 12
#endif
#ifndef AES_256_KEY_SIZE
#define AES_256_KEY_SIZE   0x20
#endif
#ifndef AES_256_U32_PER_KEY
#define AES_256_U32_PER_KEY (AES_256_KEY_SIZE / sizeof(u32))
#endif
#ifndef AES_256_NUM_ROUNDS
#define AES_256_NUM_ROUNDS 14
#endif

/// Context for AES-128 operations.
typedef struct {
    u8 round_keys[AES_128_NUM_ROUNDS+1][AES_BLOCK_SIZE];
} Aes128Context;

/// Context for AES-192 operations.
typedef struct {
    u8 round_keys[AES_192_NUM_ROUNDS+1][AES_BLOCK_SIZE];
} Aes192Context;

/// Context for AES-256 operations.
typedef struct {
    u8 round_keys[AES_256_NUM_ROUNDS+1][AES_BLOCK_SIZE];
} Aes256Context;

/// Initialize a 128-bit AES context.
void aes128ContextCreate(Aes128Context *out, const void *key, bool is_encryptor);
/// Encrypt using an AES context (Requires is_encryptor when initializing)
void aes128EncryptBlock(const Aes128Context *ctx, void *dst, const void *src);
/// Decrypt using an AES context (Requires !is_encryptor when initializing)
void aes128DecryptBlock(const Aes128Context *ctx, void *dst, const void *src);

/// Initialize a 192-bit AES context.
void aes192ContextCreate(Aes192Context *out, const void *key, bool is_encryptor);
/// Encrypt using an AES context (Requires is_encryptor when initializing)
void aes192EncryptBlock(const Aes192Context *ctx, void *dst, const void *src);
/// Decrypt using an AES context (Requires !is_encryptor when initializing)
void aes192DecryptBlock(const Aes192Context *ctx, void *dst, const void *src);

/// Initialize a 256-bit AES context.
void aes256ContextCreate(Aes256Context *out, const void *key, bool is_encryptor);
/// Encrypt using an AES context (Requires is_encryptor when initializing)
void aes256EncryptBlock(const Aes256Context *ctx, void *dst, const void *src);
/// Decrypt using an AES context (Requires !is_encryptor when initializing)
void aes256DecryptBlock(const Aes256Context *ctx, void *dst, const void *src);
