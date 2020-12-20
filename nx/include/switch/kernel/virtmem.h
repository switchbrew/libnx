/**
 * @file virtmem.h
 * @brief Virtual memory mapping utilities
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Address space reservation type (see \ref virtmemAddReservation)
typedef struct VirtmemReservation VirtmemReservation;

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

/**
 * @brief Finds a random slice of free code memory address space.
 * @param size Desired size of the slice (rounded up to page alignment).
 * @param guard_size Desired size of the unmapped guard areas surrounding the slice  (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 * @note The virtual memory manager mutex must be held during the find-and-map process (see \ref virtmemLock and \ref virtmemUnlock).
 */
void* virtmemFindCodeMemory(size_t size, size_t guard_size);

/**
 * @brief Reserves a range of memory address space.
 * @param mem Pointer to the address space slice.
 * @param size Size of the slice.
 * @return Pointer to a reservation object, or NULL on failure.
 * @remark This function is intended to be used in lieu of a memory map operation when the memory won't be mapped straight away.
 * @note The virtual memory manager mutex must be held during the find-and-reserve process (see \ref virtmemLock and \ref virtmemUnlock).
 */
VirtmemReservation* virtmemAddReservation(void* mem, size_t size);

/**
 * @brief Releases a memory address space reservation.
 * @param rv Reservation to release.
 * @note The virtual memory manager mutex must be held before calling this function (see \ref virtmemLock and \ref virtmemUnlock).
 */
void virtmemRemoveReservation(VirtmemReservation* rv);
