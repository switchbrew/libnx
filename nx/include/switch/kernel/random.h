/**
 * @file random.h
 * @brief OS-seeded pseudo-random number generation support (ChaCha algorithm).
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Fills a buffer with random data.
 * @param buf Pointer to the buffer.
 * @param len Size of the buffer in bytes.
 */
void randomGet(void* buf, size_t len);

/**
 * @brief Returns a random 64-bit value.
 * @return Random value.
 */
u64  randomGet64(void);
