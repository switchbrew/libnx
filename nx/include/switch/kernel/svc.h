/**
 * @file svc.h
 * @brief Wrappers for kernel syscalls.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Pseudo handle for the current process.
#define CUR_PROCESS_HANDLE 0xFFFF8001

/// Pseudo handle for the current thread.
#define CUR_THREAD_HANDLE 0xFFFF8000

/// Memory type enumeration (lower 8 bits of \ref MemoryState)
typedef enum {
    MemType_Unmapped=0x00,            ///< Unmapped memory.
    MemType_Io=0x01,                  ///< Mapped by kernel capability parsing in \ref svcCreateProcess.
    MemType_Normal=0x02,              ///< Mapped by kernel capability parsing in \ref svcCreateProcess.
    MemType_CodeStatic=0x03,          ///< Mapped during \ref svcCreateProcess.
    MemType_CodeMutable=0x04,         ///< Transition from MemType_CodeStatic performed by \ref svcSetProcessMemoryPermission.
    MemType_Heap=0x05,                ///< Mapped using \ref svcSetHeapSize.
    MemType_SharedMem=0x06,           ///< Mapped using \ref svcMapSharedMemory.
    MemType_WeirdMappedMem=0x07,      ///< Mapped using \ref svcMapMemory.
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

/// Secure monitor arguments.
typedef struct {
    u64 X[8]; ///< Values of X0 through X7.
} PACKED SecmonArgs;

/// JIT mapping operations
typedef enum {
    JitMapOperation_MapOwner=0,   ///< Map owner.
    JitMapOperation_MapSlave=1,   ///< Map slave.
    JitMapOperation_UnmapOwner=2, ///< Unmap owner.
    JitMapOperation_UnmapSlave=3, ///< Unmap slave.
} JitMapOperation;

///@name Memory management
///@{

/**
 * @brief Set the process heap to a given size. It can both extend and shrink the heap.
 * @param[out] out_addr Variable to which write the address of the heap (which is randomized and fixed by the kernel)
 * @param[in] size Size of the heap, must be a multiple of 0x2000000 and [2.0.0+] less than 0x18000000.
 * @return Result code.
 * @note Syscall number 0x00.
 */
Result svcSetHeapSize(void** out_addr, u64 size);

/**
 * @brief Set the memory permissions of a (page-aligned) range of memory.
 * @param[in] addr Start address of the range.
 * @param[in] size Size of the range, in bytes.
 * @param[in] perm Permissions (see \ref Permission).
 * @return Result code.
 * @remark Perm_X is not allowed. Setting write-only is not allowed either (Perm_W).
 *         This can be used to move back and forth between Perm_None, Perm_R and Perm_Rw.
 * @note Syscall number 0x01.
 */
Result svcSetMemoryPermission(void* addr, u64 size, u32 perm);

/**
 * @brief Set the memory attributes of a (page-aligned) range of memory.
 * @param[in] addr Start address of the range.
 * @param[in] size Size of the range, in bytes.
 * @param[in] val0 State0
 * @param[in] val1 State1
 * @return Result code.
 * @remark See <a href="http://switchbrew.org/index.php?title=SVC#svcSetMemoryAttribute">switchbrew.org Wiki</a> for more details.
 * @note Syscall number 0x02.
 */
Result svcSetMemoryAttribute(void* addr, u64 size, u32 val0, u32 val1);

/**
 * @brief Maps a memory range into a different range. Mainly used for adding guard pages around stack.
 * Source range gets reprotected to Perm_None (it can no longer be accessed), and \ref MemAttr_IsBorrowed is set in the source \ref MemoryAttribute.
 * @param[in] dst_addr Destination address.
 * @param[in] src_addr Source address.
 * @param[in] size Size of the range.
 * @return Result code.
 * @note Syscall number 0x04.
 */
Result svcMapMemory(void* dst_addr, void* src_addr, u64 size);

/**
 * @brief Unmaps a region that was previously mapped with \ref svcMapMemory.
 * @param[in] dst_addr Destination address.
 * @param[in] src_addr Source address.
 * @param[in] size Size of the range.
 * @return Result code.
 * @note Syscall number 0x05.
 */
Result svcUnmapMemory(void* dst_addr, void* src_addr, u64 size);

/**
 * @brief Query information about an address. Will always fetch the lowest page-aligned mapping that contains the provided address.
 * @param[out] meminfo_ptr \ref MemoryInfo structure which will be filled in.
 * @param[out] page_info Page information which will be filled in.
 * @param[in] addr Address to query.
 * @return Result code.
 * @note Syscall number 0x06.
 */
Result svcQueryMemory(MemoryInfo* meminfo_ptr, u32 *pageinfo, u64 addr);

///@}

///@name Process and thread management
///@{

void NORETURN svcExitProcess(void);
Result svcCreateThread(Handle* out, void* entry, void* arg, void* stack_top, int prio, int cpuid);
Result svcStartThread(Handle handle);
void NORETURN svcExitThread(void);
Result svcSleepThread(u64 nano);

///@}

///@name Synchronization
///@{

Result svcClearEvent(Handle handle);

///@}

///@name Inter-process memory sharing
///@{

Result svcMapSharedMemory(Handle handle, void* addr, size_t size, u32 perm);
Result svcUnmapSharedMemory(Handle handle, void* addr, size_t size);
Result svcCreateTransferMemory(Handle* out, void* addr, size_t size, u32 perm);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Closes a handle, decrementing the reference count of the corresponding kernel object.
 * This might result in the kernel freeing the object.
 * @param handle Handle to close.
 * @return Result code.
 * @note Syscall number 0x16.
 */
Result svcCloseHandle(Handle handle);

///@}

///@name Synchronization
///@{

Result svcResetSignal(Handle handle);

///@}

///@name Synchronization
///@{

Result svcWaitSynchronization(s32* index, const Handle* handles, s32 handleCount, u64 timeout);

static inline Result svcWaitSynchronizationSingle(Handle handle, u64 timeout) {
    s32 tmp;
    return svcWaitSynchronization(&tmp, &handle, 1, timeout);
}

Result svcCancelSynchronization(Handle thread);
Result svcArbitrateLock(u32 wait_tag, u32* tag_location, u32 self_tag);
Result svcArbitrateUnlock(u32* tag_location);
Result svcWaitProcessWideKeyAtomic(u32* key, u32* tag_location, u32 self_tag, u64 timeout);
Result svcSignalProcessWideKey(u32* key, s32 num);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Gets the current system tick.
 * @return The current system tick.
 * @note Syscall number 0x1E.
 */
u64 svcGetSystemTick(void);

///@}

///@name Inter-process communication (IPC)
///@{

Result svcConnectToNamedPort(Handle* session, const char* name);
Result svcSendSyncRequest(Handle session);

///@}

///@name Process and thread management
///@{

Result svcGetProcessId(u64 *processID, Handle handle);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Breaks execution. Panic.
 * @param[in] breakReason Break reason.
 * @param[in] inval1 First break parameter.
 * @param[in] inval2 Second break parameter.
 * @return Result code.
 * @note Syscall number 0x26.
 */
Result svcBreak(u32 breakReason, u64 inval1, u64 inval2);

///@}

///@name Debugging
///@{

Result svcOutputDebugString(const char *str, u64 size);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Retrieves information about the system, or a certain kernel object.
 * @param[out] out Variable to which store the information.
 * @param[in] id0 First ID of the property to retrieve.
 * @param[in] handle Handle of the object to retrieve information from, or \ref INVALID_HANDLE to retrieve information about the system.
 * @param[in] id1 Second ID of the property to retrieve.
 * @return Result code.
 * @remark The full list of property IDs can be found on the <a href="http://switchbrew.org/index.php?title=SVC#svcGetInfo">switchbrew.org wiki</a>.
 * @note Syscall number 0x29.
 */
Result svcGetInfo(u64* out, u64 id0, Handle handle, u64 id1);

///@}

///@name Process and thread management
///@{

Result svcSetThreadActivity(Handle thread, bool paused);

///@}

///@name Inter-process communication (IPC)
///@{

Result svcCreateSession(Handle *server_handle, Handle *client_handle, u32 unk0, u64 unk1);//unk* are normally 0?
Result svcAcceptSession(Handle *session_handle, Handle port_handle);
Result svcReplyAndReceive(s32* index, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);

///@}

///@name Just-in-time (JIT) compilation support
///@{

Result svcCreateJitMemory(Handle* jit_handle, void* src_addr, u64 size);
Result svcMapJitMemory(Handle jit_handle, JitMapOperation op, void* dst_addr, u64 size, u64 perm);

///@}

///@name Inter-process memory sharing
///@{

Result svcCreateSharedMemory(Handle* out, size_t size, u32 local_perm, u32 other_perm);
Result svcMapTransferMemory(Handle tmem_handle, void* addr, size_t size, u32 perm);
Result svcUnmapTransferMemory(Handle tmem_handle, void* addr, size_t size);

///@}

///@name Device memory-mapped I/O (MMIO)
///@{

Result svcQueryPhysicalAddress(u64 out[3], u64 virtaddr);
Result svcQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);

///@}

///@name I/O memory management unit (IOMMU)
///@{

Result svcCreateDeviceAddressSpace(Handle *handle, u64 dev_addr, u64 dev_size);
Result svcAttachDeviceAddressSpace(u64 device, Handle handle);
Result svcDetachDeviceAddressSpace(u64 device, Handle handle);
Result svcMapDeviceAddressSpaceAligned(Handle handle, Handle proc_handle, u64 dev_addr, u64 dev_size, u64 map_addr, u64 perm);
Result svcUnmapDeviceAddressSpace(Handle handle, Handle proc_handle, u64 map_addr, u64 map_size, u64 perm);

///@}

///@name Debugging
///@{

Result svcDebugActiveProcess(Handle* debug, u64 processID);
Result svcBreakDebugProcess(Handle debug);
Result svcGetDebugEvent(u8* event_out, Handle* debug);
Result svcContinueDebugEvent(Handle debug, u32 flags, u64 unk);
Result svcGetDebugThreadContext(u8* out, Handle debug, u64 threadID, u32 flags);

///@}

///@name Process and thread management
///@{

Result svcGetProcessList(u32 *num_out, u64 *pids_out, u32 max_pids);

///@}

///@name Debugging
///@{

Result svcQueryDebugProcessMemory(MemoryInfo* meminfo_ptr, u32* pageinfo, Handle debug, u64 addr);
Result svcReadDebugProcessMemory(void* buffer, Handle debug, u64 addr, u64 size);
Result svcWriteDebugProcessMemory(Handle debug, void* buffer, u64 addr, u64 size);

///@}

///@name Inter-process communication (IPC)
///@{

Result svcManageNamedPort(Handle* portServer, const char* name, s32 maxSessions);

///@}

///@name Memory management
///@{

/**
 * @brief Sets the memory permissions for the specified memory with the supplied process handle.
 * @param[in] proc Process handle.
 * @param[in] addr Address of the memory.
 * @param[in] size Size of the memory.
 * @param[in] perm Permissions (see \ref Permission).
 * @return Result code.
 * @remark This returns an error (0xD801) when \p perm is >0x5, hence -WX and RWX are not allowed.
 * @note Syscall number 0x73.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetProcessMemoryPermission(Handle proc, u64 addr, u64 size, u32 perm);

/**
 * @brief Maps the src address from the supplied process handle into the current process.
 * @param[in] dst Address to which map the memory in the current process.
 * @param[in] proc Process handle.
 * @param[in] src Address of the memory in the process.
 * @param[in] size Size of the memory.
 * @return Result code.
 * @remark This allows mapping code and rodata with RW- permission.
 * @note Syscall number 0x74.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapProcessMemory(void* dst, Handle proc, u64 src, u64 size);

/**
 * @brief Maps normal heap in a certain process as executable code (used when loading NROs).
 * @param[in] proc Process handle (cannot be \ref CUR_PROCESS_HANDLE).
 * @param[in] dst Destination mapping address.
 * @param[in] src Source mapping address.
 * @param[in] size Size of the mapping.
 * @return Result code.
 * @note Syscall number 0x77.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapProcessCodeMemory(Handle proc, u64 dst, u64 src, u64 size);

/**
 * @brief Undoes the effects of \ref svcMapProcessCodeMemory.
 * @param[in] proc Process handle (cannot be \ref CUR_PROCESS_HANDLE).
 * @param[in] dst Destination mapping address.
 * @param[in] src Source mapping address.
 * @param[in] size Size of the mapping.
 * @return Result code.
 * @note Syscall number 0x78.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcUnmapProcessCodeMemory(Handle proc, u64 dst, u64 src, u64 size);

///@}

///@name Process and thread management
///@{

Result svcCreateProcess(Handle* out, void* proc_info, u32* caps, u64 cap_num);
Result svcStartProcess(Handle proc, s32 main_prio, s32 default_cpu, u32 stack_size);

///@}

///@name ( ͡° ͜ʖ ͡°)
///@{

/**
 * @brief Calls a secure monitor function (TrustZone, EL3).
 * @param regs Arguments to pass to the secure monitor.
 * @return Return value from the secure monitor.
 * @note Syscall number 0x7F.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
u64 svcCallSecureMonitor(SecmonArgs* regs);

///@}
