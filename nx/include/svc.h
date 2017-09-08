/**
 * @file svc.h
 * @brief Syscall wrappers.
 */
#pragma once

#include "types.h"

/// Pseudo handle for the current process
#define CUR_PROCESS_HANDLE 0xFFFF8001

/// Pseudo handle for the current thread
#define CUR_THREAD_HANDLE 0xFFFF8000

/**
 * @brief Gets the thread local storage buffer.
 * @return The thread local storage buffer.
 */
static inline void* getThreadLocalStorage(void)
{
	void* ret;
	__asm__ ("mrs %[data], tpidrro_el0 : [data] "=x" (ret));
	return ret;
}

/**
 * @brief Gets the thread command buffer.
 * @return The thread command buffer.
 */
static inline u32* getThreadCommandBuffer(void)
{
	return (u32*)getThreadLocalStorage();
}

