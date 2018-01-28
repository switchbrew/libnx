/**
 * @file cache.h
 * @brief AArch64 cache operations.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Performs a data cache flush on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @remarks Cache flush is defined as Clean + Invalidate.
 */
void armDCacheFlush(void* addr, size_t size);

/**
 * @brief Performs a data cache clean on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 */
void armDCacheClean(void* addr, size_t size);

/**
 * @brief Performs an instruction cache invalidation clean on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 */
void armICacheInvalidate(void* addr, size_t size);

/**
 * @brief Performs a data cache zeroing operation on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 */
void armDCacheZero(void* addr, size_t size);
