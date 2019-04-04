/**
 * @file sha1.h
 * @brief Hardware accelerated SHA1 implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

#ifndef SHA1_HASH_SIZE
#define SHA1_HASH_SIZE 0x14
#endif

#ifndef SHA1_BLOCK_SIZE
#define SHA1_BLOCK_SIZE 0x40
#endif

/// Context for SHA1 operations.
typedef struct {
    u32 intermediate_hash[SHA1_HASH_SIZE / sizeof(u32)];
    u8  buffer[SHA1_BLOCK_SIZE];
    u64 bits_consumed;
    size_t num_buffered;
    bool finalized;
} Sha1Context;

/// Initialize a SHA1 context.
void sha1ContextCreate(Sha1Context *out);
/// Updates SHA1 context with data to hash
void sha1ContextUpdate(Sha1Context *ctx, const void *src, size_t size);
/// Gets the context's output hash, finalizes the context.
void sha1ContextGetHash(Sha1Context *ctx, void *dst);

/// Simple all-in-one SHA1 calculator.
void sha1CalculateHash(void *dst, const void *src, size_t size);
