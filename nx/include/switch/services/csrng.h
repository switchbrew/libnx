/**
 * @file csrng.h
 * @brief Cryptographically-Secure Random Number Generation (csrng) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result csrngInitialize(void);
void csrngExit(void);

Result csrngGetRandomBytes(void *out, size_t out_size);
