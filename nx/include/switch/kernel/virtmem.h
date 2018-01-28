/**
 * @file virtmem.h
 * @brief Virtual memory mapping utilities
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Reserves a slice of general purpose address space.
 * @param size The size of the slice of address space that will be reserved (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 */
void* virtmemReserve(size_t size);

/**
 * @brief Relinquishes a slice of address space reserved with virtmemReserve (currently no-op).
 * @param addr Pointer to the slice.
 * @param size Size of the slice.
 */
void  virtmemFree(void* addr, size_t size);

/**
 * @brief Reserves a slice of address space inside the alias memory mapping region(s) (for use with svcMapMemory).
 * @param size The size of the slice of address space that will be reserved (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 */
void* virtmemReserveMap(size_t size);

/**
 * @brief Relinquishes a slice of address space reserved with virtmemReserveMap (currently no-op).
 * @param addr Pointer to the slice.
 * @param size Size of the slice.
 */
void  virtmemFreeMap(void* addr, size_t size);
