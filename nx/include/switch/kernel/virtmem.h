/**
 * @file virtmem.h
 * @brief Virtual memory mapping utilities
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Reserves a slice of general purpose address space sequentially.
 * @param size Desired size of the slice (rounded up to page alignment).
 * @return Pointer to the slice of address space.
 */
void* virtmemReserve(size_t size);

/**
 * @brief Relinquishes a slice of address space reserved with virtmemReserve (currently no-op).
 * @param addr Pointer to the slice.
 * @param size Size of the slice.
 */
void  virtmemFree(void* addr, size_t size);

/// Locks the virtual memory manager mutex.
void virtmemLock(void);

/// Unlocks the virtual memory manager mutex.
void virtmemUnlock(void);

/**
 * @brief Finds a random slice of free general purpose address space.
 * @param size Desired size of the slice (rounded up to page alignment).
 * @param guard_size Desired size of the unmapped guard areas surrounding the slice  (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 * @note The virtual memory manager mutex must be held during the find-and-map process (see \ref virtmemLock and \ref virtmemUnlock).
 */
void* virtmemFindAslr(size_t size, size_t guard_size);

/**
 * @brief Finds a random slice of free stack address space.
 * @param size Desired size of the slice (rounded up to page alignment).
 * @param guard_size Desired size of the unmapped guard areas surrounding the slice  (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 * @note The virtual memory manager mutex must be held during the find-and-map process (see \ref virtmemLock and \ref virtmemUnlock).
 */
void* virtmemFindStack(size_t size, size_t guard_size);
