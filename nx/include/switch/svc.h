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
	__asm__ ("mrs %x[data], tpidrro_el0" : [data] "=r" (ret));
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

/**
 * @brief Replies to and receives a new request.
 * @param index Pointer to the index of the request.
 * @param handles Session handles to receive requests from.
 * @param handleCount Number of handles.
 * @param replyTarget Handle of the session to reply to, 0 = don't reply.
 */
Result svcReplyAndReceive(s32* index, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);

/**
 * @brief Creates a named port.
 * @param[out] portServer Pointer to output the port handle to.
 * @param name Name of the port.
 * @param maxSessions Maximum number of sessions that can connect to the port.
 */
Result svcManageNamedPort(Handle* portServer, const char* name, s32 maxSessions);

/**
 * @brief Gets the virtaddr which the input physaddr is mapped to.
 * @param[out] virtaddr Pointer to output the virtaddr to.
 * @param physaddr Input physaddr.
 * @param size Size.
 */
Result svcQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);

