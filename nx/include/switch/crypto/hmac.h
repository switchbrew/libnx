/**
 * @file hmac.h
 * @brief Hardware accelerated HMAC-SHA(1, 256) implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "sha256.h"

/// Context for HMAC-SHA256 operations.
typedef struct {
    Sha256Context sha_ctx;
    u32 key[SHA256_BLOCK_SIZE / sizeof(u32)];
    u32 mac[SHA256_HASH_SIZE / sizeof(u32)];
    bool finalized;
} HmacSha256Context;

/// Initialize a HMAC-SHA256 context.
void hmacSha256ContextCreate(HmacSha256Context *out, const void *key, size_t key_size);
/// Updates HMAC-SHA256 context with data to hash
void hmacSha256ContextUpdate(HmacSha256Context *ctx, const void *src, size_t size);
/// Gets the context's output mac, finalizes the context.
void hmacSha256ContextGetMac(HmacSha256Context *ctx, void *dst);

/// Simple all-in-one HMAC-SHA256 calculator.
void hmacSha256CalculateMac(void *dst, const void *key, size_t key_size, const void *src, size_t size);
