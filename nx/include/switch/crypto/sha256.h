/**
 * @file sha256.h
 * @brief Hardware accelerated SHA256 implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"


#ifndef SHA256_HASH_SIZE
#define SHA256_HASH_SIZE 0x20
#endif

#ifndef SHA256_BLOCK_SIZE
#define SHA256_BLOCK_SIZE 0x40
#endif

/// Context for SHA256 operations.
typedef struct {
    u32 intermediate_hash[SHA256_HASH_SIZE / sizeof(u32)];
    u8  buffer[SHA256_BLOCK_SIZE];
    u64 bits_consumed;
    size_t num_buffered;
    bool finalized;
} Sha256Context;

/// Initialize a SHA256 context.
void sha256ContextCreate(Sha256Context *out);
/// Updates SHA256 context with data to hash
void sha256ContextUpdate(Sha256Context *ctx, const void *src, size_t size);
/// Gets the context's output hash, finalizes the context.
void sha256ContextGetHash(Sha256Context *ctx, void *dst);

/// Simple all-in-one SHA256 calculator.
void sha256CalculateHash(void *dst, const void *src, size_t size);
