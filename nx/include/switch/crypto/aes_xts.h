/**
 * @file aes_xts.h
 * @brief Hardware accelerated AES-XTS implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "aes.h"

/// Context for AES-128 XTS.
typedef struct {
    Aes128Context aes_ctx;
    Aes128Context tweak_ctx;
    u8 tweak[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
} Aes128XtsContext;

/// Context for AES-192 XTS.
typedef struct {
    Aes192Context aes_ctx;
    Aes192Context tweak_ctx;
    u8 tweak[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
} Aes192XtsContext;

/// Context for AES-256 XTS.
typedef struct {
    Aes256Context aes_ctx;
    Aes256Context tweak_ctx;
    u8 tweak[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
} Aes256XtsContext;

/// 128-bit XTS API.
void aes128XtsContextCreate(Aes128XtsContext *out, const void *key0, const void *key1, bool is_encryptor);
void aes128XtsContextResetTweak(Aes128XtsContext *ctx, const void *tweak);
void aes128XtsContextResetSector(Aes128XtsContext *ctx, uint64_t sector, bool is_nintendo);
size_t aes128XtsEncrypt(Aes128XtsContext *ctx, void *dst, const void *src, size_t size);
size_t aes128XtsDecrypt(Aes128XtsContext *ctx, void *dst, const void *src, size_t size);

/// 192-bit XTS API.
void aes192XtsContextCreate(Aes192XtsContext *out, const void *key0, const void *key1, bool is_encryptor);
void aes192XtsContextResetTweak(Aes192XtsContext *ctx, const void *tweak);
void aes192XtsContextResetSector(Aes192XtsContext *ctx, uint64_t sector, bool is_nintendo);
size_t aes192XtsEncrypt(Aes192XtsContext *ctx, void *dst, const void *src, size_t size);
size_t aes192XtsDecrypt(Aes192XtsContext *ctx, void *dst, const void *src, size_t size);

/// 256-bit XTS API.
void aes256XtsContextCreate(Aes256XtsContext *out, const void *key0, const void *key1, bool is_encryptor);
void aes256XtsContextResetTweak(Aes256XtsContext *ctx, const void *tweak);
void aes256XtsContextResetSector(Aes256XtsContext *ctx, uint64_t sector, bool is_nintendo);
size_t aes256XtsEncrypt(Aes256XtsContext *ctx, void *dst, const void *src, size_t size);
size_t aes256XtsDecrypt(Aes256XtsContext *ctx, void *dst, const void *src, size_t size);
