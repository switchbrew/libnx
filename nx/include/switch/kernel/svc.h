/**
 * @file svc.h
 * @brief Syscall wrappers.
 */
#pragma once

#include "../types.h"

/// Pseudo handle for the current process
#define CUR_PROCESS_HANDLE 0xFFFF8001

/// Pseudo handle for the current thread
#define CUR_THREAD_HANDLE 0xFFFF8000

typedef struct {
    u64 addr;
    u64 size;
    u32 type;
    u32 attr;
    u32 perm;
    u32 device_refcount;
    u32 ipc_refcount;
    u32 padding;
} MemoryInfo;

typedef struct {
    u64 X[8];
} __attribute__((packed)) SecmonArgs;

Result svcSetHeapSize(void** out_addr, u64 size);
Result svcMapMemory(void* dst_addr, void* src_addr, u64 size);
Result svcUnmapMemory(void* dst_addr, void* src_addr, u64 size);
Result svcQueryMemory(MemoryInfo* meminfo_ptr, u32 *pageinfo, u64 addr);
void NORETURN svcExitProcess(void);
Result svcCreateThread(Handle* out, void* entry, void* arg, void* stack_top, int prio, int cpuid);
Result svcStartThread(Handle handle);
void NORETURN svcExitThread(void);
Result svcSleepThread(u64 nano);
Result svcClearEvent(Handle handle);
Result svcCloseHandle(Handle handle);
Result svcResetSignal(Handle handle);
Result svcMapSharedMemory(Handle handle, void* addr, size_t size, u32 perm);
Result svcUnmapSharedMemory(Handle handle, void* addr, size_t size);
Result svcCreateTransferMemory(Handle* out, void* addr, size_t size, u32 perm);
Result svcWaitSynchronization(s32* index, const Handle* handles, s32 handleCount, u64 timeout);
Result svcCancelSynchronization(Handle thread);
Result svcArbitrateLock(u32 wait_tag, u32* tag_location, u32 self_tag);
Result svcArbitrateUnlock(u32* tag_location);
Result svcConnectToNamedPort(Handle* session, const char* name);
u64    svcGetSystemTick(void);
Result svcSendSyncRequest(Handle session);
Result svcGetProcessId(u64 *processID, Handle handle);
Result svcBreak(u32 breakReason, u64 inval1, u64 inval2);
Result svcOutputDebugString(const char *str, u64 size);
Result svcGetInfo(u64* out, u64 id0, Handle handle, u64 id1);
Result svcSetThreadActivity(Handle thread, bool paused);
Result svcCreateSession(Handle *server_handle, Handle *client_handle, u32 unk0, u64 unk1);//unk* are normally 0?
Result svcAcceptSession(Handle *session_handle, Handle port_handle);
Result svcReplyAndReceive(s32* index, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);
Result svcCreateSharedMemory(Handle* out, size_t size, u32 local_perm, u32 other_perm);
Result svcMapTransferMemory(Handle tmem_handle, void* addr, size_t size, u32 perm);
Result svcUnmapTransferMemory(Handle tmem_handle, void* addr, size_t size);
Result svcQueryPhysicalAddress(u64 out[3], u64 virtaddr);
Result svcQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);
Result svcCreateDeviceAddressSpace(Handle *handle, u64 dev_addr, u64 dev_size);
Result svcAttachDeviceAddressSpace(u64 device, Handle handle);
Result svcDetachDeviceAddressSpace(u64 device, Handle handle);
Result svcMapDeviceAddressSpaceAligned(Handle handle, Handle proc_handle, u64 dev_addr, u64 dev_size, u64 map_addr, u64 perm);
Result svcUnmapDeviceAddressSpace(Handle handle, Handle proc_handle, u64 map_addr, u64 map_size, u64 perm);
Result svcDebugActiveProcess(Handle* debug, u64 processID);
Result svcBreakDebugProcess(Handle debug);
Result svcGetDebugEvent(u8* event_out, Handle* debug);
Result svcContinueDebugEvent(Handle debug, u32 flags, u64 unk);
Result svcGetDebugThreadContext(u8* out, Handle debug, u64 threadID, u32 flags);
Result svcGetProcessList(u32 *num_out, u64 *pids_out, u32 max_pids);
Result svcQueryDebugProcessMemory(MemoryInfo* meminfo_ptr, u32* pageinfo, Handle debug, u64 addr);
Result svcReadDebugProcessMemory(void* buffer, Handle debug, u64 addr, u64 size);
Result svcWriteDebugProcessMemory(Handle debug, void* buffer, u64 addr, u64 size);
Result svcManageNamedPort(Handle* portServer, const char* name, s32 maxSessions);
Result svcMapProcessMemory(void* dst, Handle proc, u64 src, u64 size);
Result svcCreateProcess(Handle* out, void* proc_info, u32* caps, u64 cap_num);
Result svcStartProcess(Handle proc, s32 main_prio, s32 default_cpu, u32 stack_size);
u64    svcCallSecureMonitor(SecmonArgs* regs);
