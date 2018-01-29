/**
 * @file svc.h
 * @brief Wrappers for kernel syscalls.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Pseudo handle for the current process
#define CUR_PROCESS_HANDLE 0xFFFF8001

/// Pseudo handle for the current thread
#define CUR_THREAD_HANDLE 0xFFFF8000

/// Memory type enumeration (lower 8 bits of MemoryState)
typedef enum {
    MemType_Unmapped=0x00,            ///< Unmapped memory.
    MemType_Io=0x01,                  ///< Mapped by kernel capability parsing in \ref svcCreateProcess.
    MemType_Normal=0x02,              ///< Mapped by kernel capability parsing in \ref svcCreateProcess.
    MemType_CodeStatic=0x03,          ///< Mapped during \ref svcCreateProcess.
    MemType_CodeMutable=0x04,         ///< Transition from MemType_CodeStatic performed by \ref svcSetProcessMemoryPermission.
    MemType_Heap=0x05,                ///< Mapped using \ref svcSetHeapSize.
    MemType_SharedMem=0x06,           ///< Mapped using \ref svcMapSharedMemory.
    MemType_WeirdSharedMem=0x07,      ///< Mapped using \ref svcMapMemory.
    MemType_ModuleCodeStatic=0x08,    ///< Mapped using \ref svcMapProcessCodeMemory.
    MemType_ModuleCodeMutable=0x09,   ///< Transition from \ref MemType_ModuleCodeStatic performed by \ref svcSetProcessMemoryPermission.
    MemType_IpcBuffer0=0x0A,          ///< IPC buffers with descriptor flags=0.
    MemType_MappedMemory=0x0B,        ///< Mapped using \ref svcMapMemory.
    MemType_ThreadLocal=0x0C,         ///< Mapped during \ref svcCreateThread.
    MemType_TransferMemIsolated=0x0D, ///< Mapped using \ref svcMapTransferMemory when the owning process has perm=0.
    MemType_TransferMem=0x0E,         ///< Mapped using \ref svcMapTransferMemory when the owning process has perm!=0.
    MemType_ProcessMem=0x0F,          ///< Mapped using \ref svcMapProcessMemory.
    MemType_Reserved=0x10,            ///< Reserved.
    MemType_IpcBuffer1=0x11,          ///< IPC buffers with descriptor flags=1.
    MemType_IpcBuffer3=0x12,          ///< IPC buffers with descriptor flags=3.
    MemType_KernelStack=0x13,         ///< Mapped in kernel during \ref svcCreateThread.
    MemType_JitReadOnly=0x14,         ///< Mapped in kernel during \ref svcMapJitMemory.
    MemType_JitWritable=0x15,         ///< Mapped in kernel during \ref svcMapJitMemory.
} MemoryType;

/// Memory state bitmasks.
typedef enum {
    MemState_Type=0xFF,                             ///< Type field (see \ref MemoryType).
    MemState_PermChangeAllowed=BIT(8),              ///< Permission change allowed.
    MemState_ForceRwByDebugSyscalls=BIT(9),         ///< Force read/writable by debug syscalls.
    MemState_IpcSendAllowed_Type0=BIT(10),          ///< IPC type 0 send allowed.
    MemState_IpcSendAllowed_Type3=BIT(11),          ///< IPC type 3 send allowed.
    MemState_IpcSendAllowed_Type1=BIT(12),          ///< IPC type 1 send allowed.
    MemState_ProcessPermChangeAllowed=BIT(14),      ///< Process permission change allowed.
    MemState_MapAllowed=BIT(15),                    ///< Map allowed.
    MemState_UnmapProcessCodeMemAllowed=BIT(16),    ///< Unmap process code memory allowed.
    MemState_TransferMemAllowed=BIT(17),            ///< Transfer memory allowed.
    MemState_QueryPAddrAllowed=BIT(18),             ///< Query physical address allowed.
    MemState_MapDeviceAllowed=BIT(19),              ///< Map device allowed (\ref svcMapDeviceAddressSpace and \ref svcMapDeviceAddressSpaceByForce).
    MemState_MapDeviceAlignedAllowed=BIT(20),       ///< Map device aligned allowed.
    MemState_IpcBufferAllowed=BIT(21),              ///< IPC buffer allowed.
    MemState_IsPoolAllocated=BIT(22),               ///< Is pool allocated.
    MemState_IsRefCounted=MemState_IsPoolAllocated, ///< Alias for \ref MemState_IsPoolAllocated.
    MemState_MapProcessAllowed=BIT(23),             ///< Map process allowed.
    MemState_AttrChangeAllowed=BIT(24),             ///< Attribute change allowed.
    MemState_JitMemAllowed=BIT(25),                 ///< JIT memory allowed.
} MemoryState;

/// Memory attribute bitmasks.
typedef enum {
    MemAttr_IsBorrowed=BIT(0),     ///< Is borrowed memory.
    MemAttr_IsIpcMapped=BIT(1),    ///< Is IPC mapped (when IpcRefCount > 0).
    MemAttr_IsDeviceMapped=BIT(2), ///< Is device mapped (when DeviceRefCount > 0).
    MemAttr_IsUncached=BIT(3),     ///< Is uncached.
} MemoryAttribute;

/// Memory permission bitmasks.
typedef enum {
    Perm_None     = 0,               ///< No permissions.
    Perm_R        = BIT(0),          ///< Read permission.
    Perm_W        = BIT(1),          ///< Write permission.
    Perm_X        = BIT(2),          ///< Execute permission.
    Perm_Rw       = Perm_R | Perm_W, ///< Read/write permissions.
    Perm_Rx       = Perm_R | Perm_X, ///< Read/execute permissions.
    Perm_DontCare = BIT(28),         ///< Don't care
} Permission;

/// Memory information structure.
typedef struct {
    u64 addr;            ///< Base address.
    u64 size;            ///< Size.
    u32 type;            ///< Memory type (see lower 8 bits of \ref MemoryState).
    u32 attr;            ///< Memory attributes (see \ref MemoryAttribute).
    u32 perm;            ///< Memory permissions (see \ref Permission).
    u32 device_refcount; ///< Device reference count.
    u32 ipc_refcount;    ///< IPC reference count.
    u32 padding;         ///< Padding.
} MemoryInfo;

typedef struct {
    u64 X[8];
} PACKED SecmonArgs;

typedef enum {
    JitMapOperation_MapOwner=0,
    JitMapOperation_MapSlave=1,
    JitMapOperation_UnmapOwner=2,
    JitMapOperation_UnmapSlave=3
} JitMapOperation;

Result svcSetHeapSize(void** out_addr, u64 size);
Result svcSetMemoryPermission(void* addr, u64 size, u32 perm);
Result svcSetMemoryAttribute(void* addr, u64 size, u32 val0, u32 val1);
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
Result svcWaitProcessWideKeyAtomic(u32* key, u32* tag_location, u32 self_tag, u64 timeout);
Result svcSignalProcessWideKey(u32* key, s32 num);
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
Result svcCreateJitMemory(Handle* jit_handle, void* src_addr, u64 size);
Result svcMapJitMemory(Handle jit_handle, JitMapOperation op, void* dst_addr, u64 size, u64 perm);
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
Result svcSetProcessMemoryPermission(Handle proc, u64 addr, u64 size, u32 perm);
Result svcMapProcessMemory(void* dst, Handle proc, u64 src, u64 size);
Result svcMapProcessCodeMemory(Handle proc, u64 dst, u64 src, u64 size);
Result svcUnmapProcessCodeMemory(Handle proc, u64 dst, u64 src, u64 size);
Result svcCreateProcess(Handle* out, void* proc_info, u32* caps, u64 cap_num);
Result svcStartProcess(Handle proc, s32 main_prio, s32 default_cpu, u32 stack_size);
u64    svcCallSecureMonitor(SecmonArgs* regs);

static inline Result svcWaitSynchronizationSingle(Handle handle, u64 timeout) {
    s32 tmp;
    return svcWaitSynchronization(&tmp, &handle, 1, timeout);
}
