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

typedef struct {
	u64 base_addr;
	u64 size;
	u32 memorytype;
	u32 memoryattribute;
	u32 perm;
	u32 devicerefcount;
	u32 ipcrefcount;
	u32 padding;
} MemInfo;

typedef struct _regs_t
{
	u64 X0;
	u64 X1;
	u64 X2;
	u64 X3;
	u64 X4;
	u64 X5;
	u64 X6;
	u64 X7;
} __attribute__((packed)) regs_t;

Result svcSetHeapSize(void** out_addr, u64 size);
Result svcQueryMemory(MemInfo* meminfo_ptr, u32 *pageinfo, u64 addr);
void svcExitProcess() __attribute__((noreturn));
Result svcSleepThread(u64 nano);
Result svcCloseHandle(Handle handle);
Result svcCreateTransferMemory(Handle* out, void* addr, size_t size, u32 perm);
Result svcWaitSynchronization(s32* index, const Handle* handles, s32 handleCount, u64 timeout);
Result svcArbitrateLock(u32 wait_tag, u32* tag_location, u32 self_tag);
Result svcArbitrateUnlock(u32* tag_location);
Result svcConnectToNamedPort(Handle* session, const char* name);
Result svcSendSyncRequest(Handle session);
Result svcBreak(u32 breakReason, u64 inval1, u64 inval2);
Result svcGetInfo(u64* out, u64 id0, Handle handle, u64 id1);
Result svcAcceptSession(Handle *session_handle, Handle port_handle);
Result svcReplyAndReceive(s32* index, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);
Result svcQueryPhysicalAddress(u64 out[3], u64 virtaddr);
Result svcQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);
Result svcCreateDeviceAddressSpace(Handle *handle, u64 dev_addr, u64 dev_size);
Result svcAttachDeviceAddressSpace(u64 device, Handle handle);
Result svcDetachDeviceAddressSpace(u64 device, Handle handle);
Result svcMapDeviceAddressSpaceAligned(Handle handle, Handle proc_handle, u64 dev_addr, u64 dev_size, u64 map_addr, u64 perm);
Result svcUnmapDeviceAddressSpace(Handle handle, Handle proc_handle, u64 map_addr, u64 map_size, u64 perm);
Result svcDebugActiveProcess(Handle* debug, u32 processID);
Result svcContinueDebugEvent(Handle debug, u32 flags, u64 unk);
Result svcQueryDebugProcessMemory(MemInfo* meminfo_ptr, u32* pageinfo, Handle debug, u64 addr);
Result svcReadDebugProcessMemory(void* buffer, Handle debug, u64 addr, u64 size);
Result svcManageNamedPort(Handle* portServer, const char* name, s32 maxSessions);
u64 svcCallSecureMonitor(regs_t *regs);
