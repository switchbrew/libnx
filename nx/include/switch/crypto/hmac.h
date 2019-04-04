/**
 * @file hmac.h
 * @brief Hardware accelerated HMAC-SHA(1, 256) implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "sha1.h"
#include "sha256.h"

/// Context for HMAC-SHA1 operations.
typedef struct {
    Sha1Context sha_ctx;
    u32 key[SHA1_BLOCK_SIZE / sizeof(u32)];
    u32 mac[SHA1_HASH_SIZE / sizeof(u32)];
    bool finalized;
} HmacSha1Context;

/// Context for HMAC-SHA256 operations.
typedef struct {
    Sha256Context sha_ctx;
    u32 key[SHA256_BLOCK_SIZE / sizeof(u32)];
    u32 mac[SHA256_HASH_SIZE / sizeof(u32)];
    bool finalized;
} HmacSha256Context;

#ifndef HMAC_SHA1_KEY_MAX
#define HMAC_SHA1_KEY_MAX   (sizeof(((HmacSha1Context *)NULL)->key))
#endif
#ifndef HMAC_SHA256_KEY_MAX
#define HMAC_SHA256_KEY_MAX (sizeof(((HmacSha256Context *)NULL)->key))
#endif

/// Initialize a HMAC-SHA256 context.
void hmacSha256ContextCreate(HmacSha256Context *out, const void *key, size_t key_size);
/// Updates HMAC-SHA256 context with data to hash
void hmacSha256ContextUpdate(HmacSha256Context *ctx, const void *src, size_t size);
/// Gets the context's output mac, finalizes the context.
void hmacSha256ContextGetMac(HmacSha256Context *ctx, void *dst);

/// Simple all-in-one HMAC-SHA256 calculator.
void hmacSha256CalculateMac(void *dst, const void *key, size_t key_size, const void *src, size_t size);

/// Initialize a HMAC-SHA1 context.
void hmacSha1ContextCreate(HmacSha1Context *out, const void *key, size_t key_size);
/// Updates HMAC-SHA1 context with data to hash
void hmacSha1ContextUpdate(HmacSha1Context *ctx, const void *src, size_t size);
/// Gets the context's output mac, finalizes the context.
void hmacSha1ContextGetMac(HmacSha1Context *ctx, void *dst);

/// Simple all-in-one HMAC-SHA1 calculator.
void hmacSha1CalculateMac(void *dst, const void *key, size_t key_size, const void *src, size_t size);
