/**
 * @file csrng.h
 * @brief Cryptographically-Secure Random Number Generation (csrng) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize csrng.
Result csrngInitialize(void);

/// Exit csrng.
void csrngExit(void);

/// Gets the Service object for the actual csrng service session.
Service* csrngGetServiceSession(void);

Result csrngGetRandomBytes(void *out, size_t out_size);
