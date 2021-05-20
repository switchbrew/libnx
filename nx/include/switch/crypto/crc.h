/**
 * @file crc.h
 * @brief Hardware accelerated CRC32 implementation.
 * @copyright libnx Authors
 */
#pragma once
#include <arm_acle.h>
#include "../types.h"

#define _CRC_ALIGN(sz, insn) \
do { \
    if (((uintptr_t)src_u8 & sizeof(sz)) && (u64)len >= sizeof(sz)) { \
        crc = __crc32##insn(crc, *((const sz *)src_u8)); \
        src_u8 += sizeof(sz); \
        len -= sizeof(sz); \
    } \
} while (0)

#define _CRC_REMAINDER(sz, insn) \
do { \
    if (len & sizeof(sz)) { \
        crc = __crc32##insn(crc, *((const sz *)src_u8)); \
        src_u8 += sizeof(sz); \
    } \
} while (0)

/// Calculate a CRC32 over data using a seed.
/// Can be used to calculate a CRC32 in chunks using an initial seed of zero for the first chunk.
static inline u32 crc32CalculateWithSeed(u32 seed, const void *src, size_t size) {
    const u8 *src_u8 = (const u8 *)src;

    u32 crc = ~seed;
    s64 len = size;

    _CRC_ALIGN(u8,  b);
    _CRC_ALIGN(u16, h);
    _CRC_ALIGN(u32, w);

    while ((len -= sizeof(u64)) >= 0) {
        crc = __crc32d(crc, *((const u64 *)src_u8));
        src_u8 += sizeof(u64);
    }

    _CRC_REMAINDER(u32, w);
    _CRC_REMAINDER(u16, h);
    _CRC_REMAINDER(u8,  b);

    return ~crc;
}

/// Calculate a CRC32 over data.
static inline u32 crc32Calculate(const void *src, size_t size) {
    return crc32CalculateWithSeed(0, src, size);
}

/// Calculate a CRC32C over data using a seed.
/// Can be used to calculate a CRC32C in chunks using an initial seed of zero for the first chunk.
static inline u32 crc32cCalculateWithSeed(u32 seed, const void *src, size_t size) {
    const u8 *src_u8 = (const u8 *)src;

    u32 crc = ~seed;
    s64 len = size;

    _CRC_ALIGN(u8,  cb);
    _CRC_ALIGN(u16, ch);
    _CRC_ALIGN(u32, cw);

    while ((len -= sizeof(u64)) >= 0) {
        crc = __crc32cd(crc, *((const u64 *)src_u8));
        src_u8 += sizeof(u64);
    }

    _CRC_REMAINDER(u32, cw);
    _CRC_REMAINDER(u16, ch);
    _CRC_REMAINDER(u8,  cb);

    return ~crc;
}

/// Calculate a CRC32C over data.
static inline u32 crc32cCalculate(const void *src, size_t size) {
    return crc32cCalculateWithSeed(0, src, size);
}

#undef _CRC_REMAINDER
#undef _CRC_ALIGN
