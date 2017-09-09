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

static inline void* armGetTls(void) {
    void* ret;
    __asm__ ("mrs %x[data], tpidrro_el0" : [data] "=r" (ret));
    return ret;
}

Result svcCloseHandle(Handle handle);
Result svcWaitSynchronization(s32* index, const Handle* handles, s32 handleCount, u64 timeout);
Result svcConnectToNamedPort(Handle* session, const char* name);
Result svcBreak(u32 BreakReason, u64 inval1, u64 inval2);
Result svcAcceptSession(Handle *session_handle, Handle port_handle);
Result svcReplyAndReceive(s32* index, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);
Result svcQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);
Result svcManageNamedPort(Handle* portServer, const char* name, s32 maxSessions);

