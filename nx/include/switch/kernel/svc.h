/**
 * @file svc.h
 * @brief Wrappers for kernel syscalls.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../arm/thread_context.h"

/// Pseudo handle for the current process.
#define CUR_PROCESS_HANDLE 0xFFFF8001

/// Pseudo handle for the current thread.
#define CUR_THREAD_HANDLE 0xFFFF8000

/// Maximum number of objects that can be waited on by \ref svcWaitSynchronization (Horizon kernel limitation).
#define MAX_WAIT_OBJECTS 0x40

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
    MemType_CodeReadOnly=0x14,        ///< Mapped in kernel during \ref svcControlCodeMemory.
    MemType_CodeWritable=0x15,        ///< Mapped in kernel during \ref svcControlCodeMemory.
    MemType_Coverage=0x16,            ///< Not available.
    MemType_Insecure=0x17,            ///< Mapped in kernel during \ref svcMapInsecurePhysicalMemory.
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
    MemState_CodeMemAllowed=BIT(25),                ///< Code memory allowed.
} MemoryState;

/// Memory attribute bitmasks.
typedef enum {
    MemAttr_IsBorrowed=BIT(0),         ///< Is borrowed memory.
    MemAttr_IsIpcMapped=BIT(1),        ///< Is IPC mapped (when IpcRefCount > 0).
    MemAttr_IsDeviceMapped=BIT(2),     ///< Is device mapped (when DeviceRefCount > 0).
    MemAttr_IsUncached=BIT(3),         ///< Is uncached.
    MemAttr_IsPermissionLocked=BIT(4), ///< Is permission locked.
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
    u32 ipc_refcount;    ///< IPC reference count.
    u32 device_refcount; ///< Device reference count.
    u32 padding;         ///< Padding.
} MemoryInfo;

/// Physical memory information structure.
typedef struct {
    u64 physical_address; ///< Physical address.
    u64 virtual_address;  ///< Virtual address.
    u64 size;             ///< Size.
} PhysicalMemoryInfo;

/// Secure monitor arguments.
typedef struct {
    u64 X[8]; ///< Values of X0 through X7.
} NX_PACKED SecmonArgs;

/// Break reasons
typedef enum {
    BreakReason_Panic         = 0,
    BreakReason_Assert        = 1,
    BreakReason_User          = 2,
    BreakReason_PreLoadDll    = 3,
    BreakReason_PostLoadDll   = 4,
    BreakReason_PreUnloadDll  = 5,
    BreakReason_PostUnloadDll = 6,
    BreakReason_CppException  = 7,

    BreakReason_NotificationOnlyFlag = 0x80000000,
} BreakReason;

/// Code memory mapping operations
typedef enum {
    CodeMapOperation_MapOwner=0,   ///< Map owner.
    CodeMapOperation_MapSlave=1,   ///< Map slave.
    CodeMapOperation_UnmapOwner=2, ///< Unmap owner.
    CodeMapOperation_UnmapSlave=3, ///< Unmap slave.
} CodeMapOperation;

/// Limitable Resources.
typedef enum {
    LimitableResource_Memory=0,           ///<How much memory can a process map.
    LimitableResource_Threads=1,          ///<How many threads can a process spawn.
    LimitableResource_Events=2,           ///<How many events can a process have.
    LimitableResource_TransferMemories=3, ///<How many transfer memories can a process make.
    LimitableResource_Sessions=4,         ///<How many sessions can a process own.
} LimitableResource;

/// Thread Activity.
typedef enum {
    ThreadActivity_Runnable = 0, ///< Thread can run.
    ThreadActivity_Paused   = 1, ///< Thread is paused.
} ThreadActivity;

/// Process Information.
typedef enum {
    ProcessInfoType_ProcessState=0,       ///<What state is a process in.
} ProcessInfoType;

/// Process States.
typedef enum {
    ProcessState_Created=0,             ///<Newly-created process, not yet started.
    ProcessState_CreatedAttached=1,     ///<Newly-created process, not yet started but attached to debugger.
    ProcessState_Running=2,             ///<Process that is running normally (and detached from any debugger).
    ProcessState_Crashed=3,             ///<Process that has just crashed.
    ProcessState_RunningAttached=4,     ///<Process that is running normally, attached to a debugger.
    ProcessState_Exiting=5,             ///<Process has begun exiting.
    ProcessState_Exited=6,              ///<Process has finished exiting.
    ProcessState_DebugSuspended=7,      ///<Process execution suspended by debugger.
} ProcessState;

/// Process Activity.
typedef enum {
    ProcessActivity_Runnable = 0, ///< Process can run.
    ProcessActivity_Paused   = 1, ///< Process is paused.
} ProcessActivity;

/// Debug Thread Parameters.
typedef enum {
    DebugThreadParam_ActualPriority=0,
    DebugThreadParam_State=1,
    DebugThreadParam_IdealCore=2,
    DebugThreadParam_CurrentCore=3,
    DebugThreadParam_CoreMask=4,
} DebugThreadParam;

/// GetInfo IDs.
typedef enum {
    InfoType_CoreMask                       = 0,  ///< Bitmask of allowed Core IDs.
    InfoType_PriorityMask                   = 1,  ///< Bitmask of allowed Thread Priorities.
    InfoType_AliasRegionAddress             = 2,  ///< Base of the Alias memory region.
    InfoType_AliasRegionSize                = 3,  ///< Size of the Alias memory region.
    InfoType_HeapRegionAddress              = 4,  ///< Base of the Heap memory region.
    InfoType_HeapRegionSize                 = 5,  ///< Size of the Heap memory region.
    InfoType_TotalMemorySize                = 6,  ///< Total amount of memory available for process.
    InfoType_UsedMemorySize                 = 7,  ///< Amount of memory currently used by process.
    InfoType_DebuggerAttached               = 8,  ///< Whether current process is being debugged.
    InfoType_ResourceLimit                  = 9,  ///< Current process's resource limit handle.
    InfoType_IdleTickCount                  = 10, ///< Number of idle ticks on CPU (only usable with current thread's core).
    InfoType_RandomEntropy                  = 11, ///< [2.0.0+] Random entropy for current process.
    InfoType_AslrRegionAddress              = 12, ///< [2.0.0+] Base of the process's address space.
    InfoType_AslrRegionSize                 = 13, ///< [2.0.0+] Size of the process's address space.
    InfoType_StackRegionAddress             = 14, ///< [2.0.0+] Base of the Stack memory region.
    InfoType_StackRegionSize                = 15, ///< [2.0.0+] Size of the Stack memory region.
    InfoType_SystemResourceSizeTotal        = 16, ///< [3.0.0+] Total memory allocated for process memory management.
    InfoType_SystemResourceSizeUsed         = 17, ///< [3.0.0+] Amount of memory currently used by process memory management.
    InfoType_ProgramId                      = 18, ///< [3.0.0+] Program ID for the process.
    InfoType_InitialProcessIdRange          = 19, ///< [4.0.0-4.1.0] Min/max initial process IDs.
    InfoType_UserExceptionContextAddress    = 20, ///< [5.0.0+] Address of the process's exception context (for break).
    InfoType_TotalNonSystemMemorySize       = 21, ///< [6.0.0+] Total amount of memory available for process, excluding that for process memory management.
    InfoType_UsedNonSystemMemorySize        = 22, ///< [6.0.0+] Amount of memory used by process, excluding that for process memory management.
    InfoType_IsApplication                  = 23, ///< [9.0.0+] Whether the specified process is an Application.
    InfoType_FreeThreadCount                = 24, ///< [11.0.0+] The number of free threads available to the process's resource limit.
    InfoType_ThreadTickCount                = 25, ///< [13.0.0+] Number of ticks spent on thread.
    InfoType_IsSvcPermitted                 = 26, ///< [14.0.0+] Does process have access to SVC (only usable with \ref svcSynchronizePreemptionState at present).
    InfoType_IoRegionHint                   = 27, ///< [16.0.0+] Low bits of the physical address for a KIoRegion.
    InfoType_AliasRegionExtraSize           = 28, ///< [18.0.0+] Extra size added to the reserved region.

    InfoType_TransferMemoryHint             = 34, ///< [19.0.0+] Low bits of the process address for a KTransferMemory.

    InfoType_ThreadTickCountDeprecated      = 0xF0000002, ///< [1.0.0-12.1.0] Number of ticks spent on thread.
} InfoType;

/// GetSystemInfo IDs.
typedef enum {
    SystemInfoType_TotalPhysicalMemorySize  = 0, ///< Total amount of DRAM available to system.
    SystemInfoType_UsedPhysicalMemorySize   = 1, ///< Current amount of DRAM used by system.
    SystemInfoType_InitialProcessIdRange    = 2, ///< Min/max initial process IDs.
} SystemInfoType;

/// GetInfo Idle/Thread Tick Count Sub IDs.
typedef enum {
    TickCountInfo_Core0 = 0,       ///< Tick count on core 0.
    TickCountInfo_Core1 = 1,       ///< Tick count on core 1.
    TickCountInfo_Core2 = 2,       ///< Tick count on core 2.
    TickCountInfo_Core3 = 3,       ///< Tick count on core 3.

    TickCountInfo_Total = UINT64_MAX, ///< ThreadTickCount: Tick count on all cores, IdleTickCount: Thread's current core.
} TickCountInfo;

/// GetInfo InitialProcessIdRange Sub IDs.
typedef enum {
    InitialProcessIdRangeInfo_Minimum = 0, ///< Lowest initial process ID.
    InitialProcessIdRangeInfo_Maximum = 1, ///< Highest initial process ID.
} InitialProcessIdRangeInfo;

/// GetSystemInfo PhysicalMemory Sub IDs.
typedef enum {
    PhysicalMemorySystemInfo_Application  = 0, ///< Memory allocated for application usage.
    PhysicalMemorySystemInfo_Applet       = 1, ///< Memory allocated for applet usage.
    PhysicalMemorySystemInfo_System       = 2, ///< Memory allocated for system usage.
    PhysicalMemorySystemInfo_SystemUnsafe = 3, ///< Memory allocated for unsafe system usage (accessible to devices).
} PhysicalMemorySystemInfo;

/// SleepThread yield types.
typedef enum {
    YieldType_WithoutCoreMigration = 0l,  ///< Yields to another thread on the same core.
    YieldType_WithCoreMigration    = -1l, ///< Yields to another thread (possibly on a different core).
    YieldType_ToAnyThread          = -2l, ///< Yields and performs forced load-balancing.
} YieldType;

/// SignalToAddress behaviors.
typedef enum {
    SignalType_Signal                                          = 0, ///< Signals the address.
    SignalType_SignalAndIncrementIfEqual                       = 1, ///< Signals the address and increments its value if equal to argument.
    SignalType_SignalAndModifyBasedOnWaitingThreadCountIfEqual = 2, ///< Signals the address and updates its value if equal to argument.
} SignalType;

/// WaitForAddress behaviors.
typedef enum {
    ArbitrationType_WaitIfLessThan             = 0, ///< Wait if the 32-bit value is less than argument.
    ArbitrationType_DecrementAndWaitIfLessThan = 1, ///< Decrement the 32-bit value and wait if it is less than argument.
    ArbitrationType_WaitIfEqual                = 2, ///< Wait if the 32-bit value is equal to argument.
    ArbitrationType_WaitIfEqual64              = 3, ///< [19.0.0+] Wait if the 64-bit value is equal to argument.
} ArbitrationType;

/// Context of a scheduled thread.
typedef struct {
    u64 fp; ///< Frame Pointer for the thread.
    u64 sp; ///< Stack Pointer for the thread.
    u64 lr; ///< Link Register for the thread.
    u64 pc; ///< Program Counter for the thread.
} LastThreadContext;

/// Memory mapping type.
typedef enum {
    MemoryMapping_IoRegister = 0, ///< Mapping IO registers.
    MemoryMapping_Uncached   = 1, ///< Mapping normal memory without cache.
    MemoryMapping_Memory     = 2, ///< Mapping normal memory.
} MemoryMapping;

/// Io Pools.
typedef enum {
    IoPoolType_PcieA2 = 0, ///< Physical address range 0x12000000-0x1FFFFFFF
} IoPoolType;

/// DebugEvent types
typedef enum {
    DebugEventType_CreateProcess = 0,
    DebugEventType_CreateThread  = 1,
    DebugEventType_ExitProcess   = 2,
    DebugEventType_ExitThread    = 3,
    DebugEventType_Exception     = 4,
} DebugEventType;

/// Process exit reasons
typedef enum {
    ProcessExitReason_ExitProcess      = 0,
    ProcessExitReason_TerminateProcess = 1,
    ProcessExitReason_Exception        = 2,
} ProcessExitReason;

/// Thread exit reasons
typedef enum {
    ThreadExitReason_ExitThread       = 0,
    ThreadExitReason_TerminateThread  = 1,
    ThreadExitReason_ExitProcess      = 2,
    ThreadExitReason_TerminateProcess = 3,
} ThreadExitReason;

/// Debug exception types
typedef enum {
    DebugException_UndefinedInstruction = 0,
    DebugException_InstructionAbort     = 1,
    DebugException_DataAbort            = 2,
    DebugException_AlignmentFault       = 3,
    DebugException_DebuggerAttached     = 4,
    DebugException_BreakPoint           = 5,
    DebugException_UserBreak            = 6,
    DebugException_DebuggerBreak        = 7,
    DebugException_UndefinedSystemCall  = 8,
    DebugException_MemorySystemError    = 9, ///< [2.0.0+]
} DebugException;

/// Break point types
typedef enum {
    BreakPointType_HardwareInstruction = 0,
    BreakPointType_HardwareData        = 1,
} BreakPointType;

/// DebugEvent flags
typedef enum {
    DebugEventFlag_Stopped = BIT(0),
} DebugEventFlag;

/// Address space types for CreateProcessFlags
typedef enum {
    CreateProcessFlagAddressSpace_32bit             = 0,
    CreateProcessFlagAddressSpace_64bitDeprecated   = 1, ///< 36-bit width
    CreateProcessFlagAddressSpace_32bitWithoutAlias = 2,
    CreateProcessFlagAddressSpace_64bit             = 3, ///< [2.0.0+] 39-bit width
} CreateProcessFlagAddressSpace;

/// Memory regions
typedef enum {
    MemoryRegion_Default = 0, ///< [1.0.0-4.1.0]
    MemoryRegion_Secure  = 1, ///< [2.0.0-4.1.0]
} MemoryRegion;

/// Flags for svcCreateProcess and CreateProcess event
typedef struct {
    u32 is_64bit: 1;
    u32 address_space: 3;                      ///< \ref CreateProcessFlagAddressSpace
    u32 enable_debug: 1;                       ///< [2.0.0+]
    u32 enable_aslr: 1;
    u32 is_application: 1;
    u32 pool_partition: 4;                     ///< [4.0.0-4.1.0] \ref MemoryRegion, [5.0.0+] \ref PhysicalMemorySystemInfo
    u32 optimize_memory_allocation: 1;         ///< [7.0.0+] Only allowed in combination with is_application
    u32 disable_device_address_space_merge: 1; ///< [11.0.0+]
    u32 enable_alias_region_extra_size: 1;     ///< [18.0.0+]
    u32 reserved: 18;
} CreateProcessFlags;

/// DebugEvent structure
typedef struct {
    u32 type;                                              ///< \ref DebugEventType
    u32 flags;                                             ///< \ref DebugEventFlag
    u64 thread_id;

    union {
        /// DebugEventType_CreateProcess
        struct {
            u64 program_id;
            u64 process_id;
            char name[0xC];
            u32 flags;                                     ///< \ref CreateProcessFlags
            void* user_exception_context_address;          ///< [5.0.0+]
        } create_process; 

        /// DebugEventType_CreateThread
        struct {
            u64 thread_id;
            void* tls_address;
            void* entrypoint;                              ///< [1.0.0-10.2.0]
        } create_thread;

        /// DebugEventType_ExitProcess
        struct {
            u32 reason;                                    ///< \ref ProcessExitReason
        } exit_process;

        /// DebugEventType_ExitThread
        struct {
            u32 reason;                                    ///< \ref ThreadExitReason
        } exit_thread;

        /// DebugEventType_Exception
        struct {
            u32 type;                                      ///< \ref DebugException
            void* address;
            union {
                /// DebugException_UndefinedInstruction
                struct {
                    u32 insn;
                } undefined_instruction;

                /// DebugException_DataAbort
                struct {
                    void* address;
                } data_abort;

                /// DebugException_AlignmentFault
                struct {
                    void* address;
                } alignment_fault;

                /// DebugException_BreakPoint
                struct {
                    u32 type;                              ///< \ref BreakPointType
                    void* address;
                } break_point;

                /// DebugException_UserBreak
                struct {
                    u32 break_reason;                      ///< \ref BreakReason
                    void* address;
                    size_t size;
                } user_break;

                /// DebugException_DebuggerBreak
                struct {
                    u64 active_thread_ids[4];
                } debugger_break;

                /// DebugException_UndefinedSystemCall
                struct {
                    u32 id;
                } undefined_system_call;

                u64 raw;
            } specific;
        } exception;
    } info;
} DebugEventInfo;

///@name Memory management
///@{

/**
 * @brief Set the process heap to a given size. It can both extend and shrink the heap.
 * @param[out] out_addr Variable to which write the address of the heap (which is randomized and fixed by the kernel)
 * @param[in] size Size of the heap, must be a multiple of 0x200000.
 * @return Result code.
 * @note Syscall number 0x01.
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
 * @note Syscall number 0x02.
 */
Result svcSetMemoryPermission(void* addr, u64 size, u32 perm);

/**
 * @brief Set the memory attributes of a (page-aligned) range of memory.
 * @param[in] addr Start address of the range.
 * @param[in] size Size of the range, in bytes.
 * @param[in] val0 State0
 * @param[in] val1 State1
 * @return Result code.
 * @remark See <a href="https://switchbrew.org/wiki/SVC#svcSetMemoryAttribute">switchbrew.org Wiki</a> for more details.
 * @note Syscall number 0x03.
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
 * @param[out] pageinfo Page information which will be filled in.
 * @param[in] addr Address to query.
 * @return Result code.
 * @note Syscall number 0x06.
 */
Result svcQueryMemory(MemoryInfo* meminfo_ptr, u32 *pageinfo, u64 addr);

///@}

///@name Process and thread management
///@{

/**
 * @brief Exits the current process.
 * @note Syscall number 0x07.
 */

void NX_NORETURN svcExitProcess(void);

/**
 * @brief Creates a thread.
 * @return Result code.
 * @note Syscall number 0x08.
 */
Result svcCreateThread(Handle* out, void* entry, void* arg, void* stack_top, int prio, int cpuid);

/**
 * @brief Starts a freshly created thread.
 * @return Result code.
 * @note Syscall number 0x09.
 */
Result svcStartThread(Handle handle);

/**
 * @brief Exits the current thread.
 * @note Syscall number 0x0A.
 */
void NX_NORETURN svcExitThread(void);

/**
 * @brief Sleeps the current thread for the specified amount of time.
 * @param[in] nano Number of nanoseconds to sleep, or \ref YieldType for yield.
 * @note Syscall number 0x0B.
 */
void svcSleepThread(s64 nano);

/**
 * @brief Gets a thread's priority.
 * @return Result code.
 * @note Syscall number 0x0C.
 */
Result svcGetThreadPriority(s32* priority, Handle handle);

/**
 * @brief Sets a thread's priority.
 * @return Result code.
 * @note Syscall number 0x0D.
 */
Result svcSetThreadPriority(Handle handle, u32 priority);

/**
 * @brief Gets a thread's core mask.
 * @return Result code.
 * @note Syscall number 0x0E.
 */
Result svcGetThreadCoreMask(s32* preferred_core, u64* affinity_mask, Handle handle);

/**
 * @brief Sets a thread's core mask.
 * @return Result code.
 * @note Syscall number 0x0F.
 */
Result svcSetThreadCoreMask(Handle handle, s32 preferred_core, u32 affinity_mask);

/**
 * @brief Gets the current processor's number.
 * @return The current processor's number.
 * @note Syscall number 0x10.
 */
u32 svcGetCurrentProcessorNumber(void);

///@}

///@name Synchronization
///@{

/**
 * @brief Sets an event's signalled status.
 * @return Result code.
 * @note Syscall number 0x11.
 */
Result svcSignalEvent(Handle handle);

/**
 * @brief Clears an event's signalled status.
 * @return Result code.
 * @note Syscall number 0x12.
 */
Result svcClearEvent(Handle handle);

///@}

///@name Inter-process memory sharing
///@{

/**
 * @brief Maps a block of shared memory.
 * @return Result code.
 * @note Syscall number 0x13.
 */
Result svcMapSharedMemory(Handle handle, void* addr, size_t size, u32 perm);

/**
 * @brief Unmaps a block of shared memory.
 * @return Result code.
 * @note Syscall number 0x14.
 */
Result svcUnmapSharedMemory(Handle handle, void* addr, size_t size);

/**
 * @brief Creates a block of transfer memory.
 * @return Result code.
 * @note Syscall number 0x15.
 */
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

/**
 * @brief Resets a signal.
 * @return Result code.
 * @note Syscall number 0x17.
 */
Result svcResetSignal(Handle handle);

///@}

///@name Synchronization
///@{

/**
 * @brief Waits on one or more synchronization objects, optionally with a timeout.
 * @return Result code.
 * @note Syscall number 0x18.
 * @note \p handleCount must not be greater than \ref MAX_WAIT_OBJECTS. This is a Horizon kernel limitation.
 * @note This is the raw syscall, which can be cancelled by \ref svcCancelSynchronization or other means. \ref waitHandles or \ref waitMultiHandle should normally be used instead.
 */
Result svcWaitSynchronization(s32* index, const Handle* handles, s32 handleCount, u64 timeout);

/**
 * @brief Waits on a single synchronization object, optionally with a timeout.
 * @return Result code.
 * @note Wrapper for \ref svcWaitSynchronization.
 * @note This is the raw syscall, which can be cancelled by \ref svcCancelSynchronization or other means. \ref waitSingleHandle should normally be used instead.
 */
static inline Result svcWaitSynchronizationSingle(Handle handle, u64 timeout) {
    s32 tmp;
    return svcWaitSynchronization(&tmp, &handle, 1, timeout);
}

/**
 * @brief Waits a \ref svcWaitSynchronization operation being done on a synchronization object in another thread.
 * @return Result code.
 * @note Syscall number 0x19.
 */
Result svcCancelSynchronization(Handle thread);

/**
 * @brief Arbitrates a mutex lock operation in userspace.
 * @return Result code.
 * @note Syscall number 0x1A.
 */
Result svcArbitrateLock(u32 wait_tag, u32* tag_location, u32 self_tag);

/**
 * @brief Arbitrates a mutex unlock operation in userspace.
 * @return Result code.
 * @note Syscall number 0x1B.
 */
Result svcArbitrateUnlock(u32* tag_location);

/**
 * @brief Performs a condition variable wait operation in userspace.
 * @return Result code.
 * @note Syscall number 0x1C.
 */
Result svcWaitProcessWideKeyAtomic(u32* key, u32* tag_location, u32 self_tag, u64 timeout);

/**
 * @brief Performs a condition variable wake-up operation in userspace.
 * @note Syscall number 0x1D.
 */
void svcSignalProcessWideKey(u32* key, s32 num);

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

/**
 * @brief Connects to a registered named port.
 * @return Result code.
 * @note Syscall number 0x1F.
 */
Result svcConnectToNamedPort(Handle* session, const char* name);

/**
 * @brief Sends a light IPC synchronization request to a session.
 * @return Result code.
 * @note Syscall number 0x20.
 */
Result svcSendSyncRequestLight(Handle session);

/**
 * @brief Sends an IPC synchronization request to a session.
 * @return Result code.
 * @note Syscall number 0x21.
 */
Result svcSendSyncRequest(Handle session);

/**
 * @brief Sends an IPC synchronization request to a session from an user allocated buffer.
 * @return Result code.
 * @remark size must be allocated to 0x1000 bytes.
 * @note Syscall number 0x22.
 */
Result svcSendSyncRequestWithUserBuffer(void* usrBuffer, u64 size, Handle session);

/**
 * @brief Sends an IPC synchronization request to a session from an user allocated buffer (asynchronous version).
 * @return Result code.
 * @remark size must be allocated to 0x1000 bytes.
 * @note Syscall number 0x23.
 */
Result svcSendAsyncRequestWithUserBuffer(Handle* handle, void* usrBuffer, u64 size, Handle session);

///@}

///@name Process and thread management
///@{

/**
 * @brief Gets the PID associated with a process.
 * @return Result code.
 * @note Syscall number 0x24.
 */
Result svcGetProcessId(u64 *processID, Handle handle);

/**
 * @brief Gets the TID associated with a process.
 * @return Result code.
 * @note Syscall number 0x25.
 */
Result svcGetThreadId(u64 *threadID, Handle handle);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Breaks execution.
 * @param[in] breakReason Break reason (see \ref BreakReason).
 * @param[in] address Address of the buffer to pass to the debugger.
 * @param[in] size Size of the buffer to pass to the debugger.
 * @return Result code.
 * @note Syscall number 0x26.
 */
Result svcBreak(u32 breakReason, uintptr_t address, uintptr_t size);

///@}

///@name Debugging
///@{

/**
 * @brief Outputs debug text, if used during debugging.
 * @param[in] str Text to output.
 * @param[in] size Size of the text in bytes.
 * @return Result code.
 * @note Syscall number 0x27.
 */
Result svcOutputDebugString(const char *str, u64 size);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Returns from an exception.
 * @param[in] res Result code.
 * @note Syscall number 0x28.
 */
void NX_NORETURN svcReturnFromException(Result res);

/**
 * @brief Retrieves information about the system, or a certain kernel object.
 * @param[out] out Variable to which store the information.
 * @param[in] id0 First ID of the property to retrieve.
 * @param[in] handle Handle of the object to retrieve information from, or \ref INVALID_HANDLE to retrieve information about the system.
 * @param[in] id1 Second ID of the property to retrieve.
 * @return Result code.
 * @remark The full list of property IDs can be found on the <a href="https://switchbrew.org/wiki/SVC#svcGetInfo">switchbrew.org wiki</a>.
 * @note Syscall number 0x29.
 */
Result svcGetInfo(u64* out, u32 id0, Handle handle, u64 id1);

///@}

///@name Cache Management
///@{

/**
 * @brief Flushes the entire data cache (by set/way).
 * @note Syscall number 0x2A.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning This syscall is dangerous, and should not be used.
 */
void svcFlushEntireDataCache(void);

/**
 * @brief Flushes data cache for a virtual address range.
 * @param[in] address Address of region to flush.
 * @param[in] size Size of region to flush.
 * @remark armDCacheFlush should be used instead of this syscall whenever possible.
 * @note Syscall number 0x2B.
 */
Result svcFlushDataCache(void *address, size_t size);

///@}

///@name Memory management
///@{

/**
 * @brief Maps new heap memory at the desired address. [3.0.0+]
 * @return Result code.
 * @note Syscall number 0x2C.
 */
Result svcMapPhysicalMemory(void *address, u64 size);

/**
 * @brief Undoes the effects of \ref svcMapPhysicalMemory. [3.0.0+]
 * @return Result code.
 * @note Syscall number 0x2D.
 */
Result svcUnmapPhysicalMemory(void *address, u64 size);

///@}

///@name Process and thread management
///@{

/**
 * @brief Gets information about a thread that will be scheduled in the future. [5.0.0+]
 * @param[out] out_context Output \ref LastThreadContext for the thread that will be scheduled.
 * @param[out] out_thread_id Output thread id for the thread that will be scheduled.
 * @param[in] debug Debug handle.
 * @param[in] ns Nanoseconds in the future to get scheduled thread at.
 * @return Result code.
 * @note Syscall number 0x2E.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetDebugFutureThreadInfo(LastThreadContext *out_context, u64 *out_thread_id, Handle debug, s64 ns);

/**
 * @brief Gets information about the previously-scheduled thread.
 * @param[out] out_context Output \ref LastThreadContext for the previously scheduled thread.
 * @param[out] out_tls_address Output tls address for the previously scheduled thread.
 * @param[out] out_flags Output flags for the previously scheduled thread.
 * @return Result code.
 * @note Syscall number 0x2F.
 */
Result svcGetLastThreadInfo(LastThreadContext *out_context, u64 *out_tls_address, u32 *out_flags);

///@}

///@name Resource Limit Management
///@{

/**
 * @brief Gets the maximum value a LimitableResource can have, for a Resource Limit handle.
 * @return Result code.
 * @note Syscall number 0x30.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetResourceLimitLimitValue(s64 *out, Handle reslimit_h, LimitableResource which);

/**
 * @brief Gets the maximum value a LimitableResource can have, for a Resource Limit handle.
 * @return Result code.
 * @note Syscall number 0x31.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetResourceLimitCurrentValue(s64 *out, Handle reslimit_h, LimitableResource which);

///@}

///@name Process and thread management
///@{

/**
 * @brief Configures the pause/unpause status of a thread.
 * @return Result code.
 * @note Syscall number 0x32.
 */
Result svcSetThreadActivity(Handle thread, ThreadActivity paused);

/**
 * @brief Dumps the registers of a thread paused by @ref svcSetThreadActivity (register groups: all).
 * @param[out] ctx Output thread context (register dump).
 * @param[in] thread Thread handle.
 * @return Result code.
 * @note Syscall number 0x33.
 * @warning Official kernel will not dump x0..x18 if the thread is currently executing a system call, and prior to 6.0.0 doesn't dump TPIDR_EL0.
 */
Result svcGetThreadContext3(ThreadContext* ctx, Handle thread);

///@}

///@name Synchronization
///@{

/**
 * @brief Arbitrates an address depending on type and value. [4.0.0+]
 * @param[in] address Address to arbitrate.
 * @param[in] arb_type \ref ArbitrationType to use.
 * @param[in] value Value to arbitrate on.
 * @param[in] timeout Maximum time in nanoseconds to wait.
 * @return Result code.
 * @note Syscall number 0x34.
 */
Result svcWaitForAddress(void *address, u32 arb_type, s64 value, s64 timeout);

/**
 * @brief Signals (and updates) an address depending on type and value. [4.0.0+]
 * @param[in] address Address to arbitrate.
 * @param[in] signal_type \ref SignalType to use.
 * @param[in] value Value to arbitrate on.
 * @param[in] count Number of waiting threads to signal.
 * @return Result code.
 * @note Syscall number 0x35.
 */
Result svcSignalToAddress(void *address, u32 signal_type, s32 value, s32 count);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Sets thread preemption state (used during abort/panic). [8.0.0+]
 * @note Syscall number 0x36.
 */
void svcSynchronizePreemptionState(void);

///@}

///@name Resource Limit Management
///@{

/**
 * @brief Gets the peak value a LimitableResource has had, for a Resource Limit handle. [11.0.0+]
 * @return Result code.
 * @note Syscall number 0x37.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetResourceLimitPeakValue(s64 *out, Handle reslimit_h, LimitableResource which);

///@}

///@name Memory Management
///@{

/**
 * @brief Creates an IO Pool. [13.0.0+]
 * @return Result code.
 * @note Syscall number 0x39.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateIoPool(Handle *out_handle, u32 pool_type);

/**
 * @brief Creates an IO Region. [13.0.0+]
 * @return Result code.
 * @note Syscall number 0x3A.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateIoRegion(Handle *out_handle, Handle io_pool_h, u64 physical_address, u64 size, u32 memory_mapping, u32 perm);

///@}

///@name Debugging
///@{
/**
 * @brief Causes the kernel to dump debug information. [1.0.0-3.0.2]
 * @param[in] dump_info_type Type of information to dump.
 * @param[in] arg0 Argument to the debugging operation.
 * @note Syscall number 0x3C.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
void svcDumpInfo(u32 dump_info_type, u64 arg0);

/**
 * @brief Performs a debugging operation on the kernel. [4.0.0+]
 * @param[in] kern_debug_type Type of debugging operation to perform.
 * @param[in] arg0 First argument to the debugging operation.
 * @param[in] arg1 Second argument to the debugging operation.
 * @param[in] arg2 Third argument to the debugging operation.
 * @note Syscall number 0x3C.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
void svcKernelDebug(u32 kern_debug_type, u64 arg0, u64 arg1, u64 arg2);

/**
 * @brief Performs a debugging operation on the kernel. [4.0.0+]
 * @param[in] kern_trace_state Type of tracing the kernel should perform.
 * @note Syscall number 0x3D.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
void svcChangeKernelTraceState(u32 kern_trace_state);

///@}
                                                                                                                                                                                                                      \
///@name Inter-process communication (IPC)
///@{

/**
 * @brief Creates an IPC session.
 * @return Result code.
 * @note Syscall number 0x40.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateSession(Handle *server_handle, Handle *client_handle, u32 unk0, u64 unk1);//unk* are normally 0?

/**
 * @brief Accepts an IPC session.
 * @return Result code.
 * @note Syscall number 0x41.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcAcceptSession(Handle *session_handle, Handle port_handle);

/**
 * @brief Performs light IPC input/output.
 * @return Result code.
 * @param[in] handle Server or port handle to act on.
 * @note Syscall number 0x42.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcReplyAndReceiveLight(Handle handle);

/**
 * @brief Performs IPC input/output.
 * @return Result code.
 * @note Syscall number 0x43.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcReplyAndReceive(s32* index, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);

/**
 * @brief Performs IPC input/output from an user allocated buffer.
 * @return Result code.
 * @note Syscall number 0x44.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcReplyAndReceiveWithUserBuffer(s32* index, void* usrBuffer, u64 size, const Handle* handles, s32 handleCount, Handle replyTarget, u64 timeout);

///@}

///@name Synchronization
///@{

/**
 * @brief Creates a system event.
 * @return Result code.
 * @note Syscall number 0x45.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateEvent(Handle* server_handle, Handle* client_handle);

///@}

///@name Memory management
///@{

/**
 * @brief Maps an IO Region. [13.0.0+]
 * @return Result code.
 * @note Syscall number 0x46.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapIoRegion(Handle io_region_h, void *address, u64 size, u32 perm);

/**
 * @brief Undoes the effects of \ref svcMapIoRegion. [13.0.0+]
 * @return Result code.
 * @note Syscall number 0x47.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcUnmapIoRegion(Handle io_region_h, void *address, u64 size);

/**
 * @brief Maps unsafe memory (usable for GPU DMA) for a system module at the desired address. [5.0.0+]
 * @return Result code.
 * @note Syscall number 0x48.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapPhysicalMemoryUnsafe(void *address, u64 size);

/**
 * @brief Undoes the effects of \ref svcMapPhysicalMemoryUnsafe. [5.0.0+]
 * @return Result code.
 * @note Syscall number 0x49.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcUnmapPhysicalMemoryUnsafe(void *address, u64 size);

/**
 * @brief Sets the system-wide limit for unsafe memory mappable using \ref svcMapPhysicalMemoryUnsafe. [5.0.0+]
 * @return Result code.
 * @note Syscall number 0x4A.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetUnsafeLimit(u64 size);

///@}


///@name Code memory / Just-in-time (JIT) compilation support
///@{

/**
 * @brief Creates code memory in the caller's address space [4.0.0+].
 * @return Result code.
 * @note Syscall number 0x4B.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateCodeMemory(Handle* code_handle, void* src_addr, u64 size);

/**
 * @brief Maps code memory in the caller's address space [4.0.0+].
 * @return Result code.
 * @note Syscall number 0x4C.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcControlCodeMemory(Handle code_handle, CodeMapOperation op, void* dst_addr, u64 size, u64 perm);

///@}

///@name Power Management
///@{

/**
 * @brief Causes the system to enter deep sleep.
 * @note Syscall number 0x4D.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
void svcSleepSystem(void);

///@}

///@name Device memory-mapped I/O (MMIO)
///@{

/**
 * @brief Reads/writes a protected MMIO register.
 * @return Result code.
 * @note Syscall number 0x4E.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcReadWriteRegister(u32* outVal, u64 regAddr, u32 rwMask, u32 inVal);

///@}

///@name Process and thread management
///@{

/**
 * @brief Configures the pause/unpause status of a process.
 * @return Result code.
 * @note Syscall number 0x4F.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetProcessActivity(Handle process, ProcessActivity paused);

///@}

///@name Inter-process memory sharing
///@{

/**
 * @brief Creates a block of shared memory.
 * @return Result code.
 * @note Syscall number 0x50.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateSharedMemory(Handle* out, size_t size, u32 local_perm, u32 other_perm);

/**
 * @brief Maps a block of transfer memory.
 * @return Result code.
 * @note Syscall number 0x51.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapTransferMemory(Handle tmem_handle, void* addr, size_t size, u32 perm);

/**
 * @brief Unmaps a block of transfer memory.
 * @return Result code.
 * @note Syscall number 0x52.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcUnmapTransferMemory(Handle tmem_handle, void* addr, size_t size);

///@}

///@name Device memory-mapped I/O (MMIO)
///@{

/**
 * @brief Creates an event and binds it to a specific hardware interrupt.
 * @return Result code.
 * @note Syscall number 0x53.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateInterruptEvent(Handle* handle, u64 irqNum, u32 flag);

/**
 * @brief Queries information about a certain virtual address, including its physical address.
 * @return Result code.
 * @note Syscall number 0x54.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcQueryPhysicalAddress(PhysicalMemoryInfo *out, u64 virtaddr);

/**
 * @brief Returns a virtual address mapped to a given IO range.
 * @return Result code.
 * @note Syscall number 0x55.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning Only exists on [10.0.0+]. For older versions use \ref svcLegacyQueryIoMapping.
 */
Result svcQueryMemoryMapping(u64* virtaddr, u64* out_size, u64 physaddr, u64 size);

/**
 * @brief Returns a virtual address mapped to a given IO range.
 * @return Result code.
 * @note Syscall number 0x55.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning Only exists on [1.0.0-9.2.0]. For newer versions use \ref svcQueryMemoryMapping.
 */
Result svcLegacyQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);

///@}

///@name I/O memory management unit (IOMMU)
///@{

/**
 * @brief Creates a virtual address space for binding device address spaces.
 * @return Result code.
 * @note Syscall number 0x56.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateDeviceAddressSpace(Handle *handle, u64 dev_addr, u64 dev_size);

/**
 * @brief Attaches a device address space to a device.
 * @return Result code.
 * @note Syscall number 0x57.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcAttachDeviceAddressSpace(u64 device, Handle handle);

/**
 * @brief Detaches a device address space from a device.
 * @return Result code.
 * @note Syscall number 0x58.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcDetachDeviceAddressSpace(u64 device, Handle handle);

/**
 * @brief Maps an attached device address space to an userspace address.
 * @return Result code.
 * @remark The userspace destination address must have the \ref MemState_MapDeviceAllowed bit set.
 * @note Syscall number 0x59.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapDeviceAddressSpaceByForce(Handle handle, Handle proc_handle, u64 map_addr, u64 dev_size, u64 dev_addr, u32 option);

/**
 * @brief Maps an attached device address space to an userspace address.
 * @return Result code.
 * @remark The userspace destination address must have the \ref MemState_MapDeviceAlignedAllowed bit set.
 * @note Syscall number 0x5A.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapDeviceAddressSpaceAligned(Handle handle, Handle proc_handle, u64 map_addr, u64 dev_size, u64 dev_addr, u32 option);

/**
 * @brief Maps an attached device address space to an userspace address. [1.0.0-12.1.0]
 * @return Result code.
 * @remark The userspace destination address must have the \ref MemState_MapDeviceAlignedAllowed bit set.
 * @note Syscall number 0x5B.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapDeviceAddressSpace(u64 *out_mapped_size, Handle handle, Handle proc_handle, u64 map_addr, u64 dev_size, u64 dev_addr, u32 perm);

/**
 * @brief Unmaps an attached device address space from an userspace address.
 * @return Result code.
 * @note Syscall number 0x5C.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcUnmapDeviceAddressSpace(Handle handle, Handle proc_handle, u64 map_addr, u64 map_size, u64 dev_addr);

///@}

///@name Cache Management
///@{

/**
 * @brief Invalidates data cache for a virtual address range within a process.
 * @param[in] address Address of region to invalidate.
 * @param[in] size Size of region to invalidate.
 * @note Syscall number 0x5D.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcInvalidateProcessDataCache(Handle process, uintptr_t address, size_t size);

/**
 * @brief Stores data cache for a virtual address range within a process.
 * @param[in] address Address of region to store.
 * @param[in] size Size of region to store.
 * @note Syscall number 0x5E.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcStoreProcessDataCache(Handle process, uintptr_t address, size_t size);

/**
 * @brief Flushes data cache for a virtual address range within a process.
 * @param[in] address Address of region to flush.
 * @param[in] size Size of region to flush.
 * @note Syscall number 0x5F.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcFlushProcessDataCache(Handle process, uintptr_t address, size_t size);

///@}

///@name Debugging
///@{

/**
 * @brief Debugs an active process.
 * @return Result code.
 * @note Syscall number 0x60.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcDebugActiveProcess(Handle* debug, u64 processID);

/**
 * @brief Breaks an active debugging session.
 * @return Result code.
 * @note Syscall number 0x61.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcBreakDebugProcess(Handle debug);

/**
 * @brief Terminates the process of an active debugging session.
 * @return Result code.
 * @note Syscall number 0x62.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcTerminateDebugProcess(Handle debug);

/**
 * @brief Gets an incoming debug event from a debugging session.
 * @return Result code.
 * @note Syscall number 0x63.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetDebugEvent(DebugEventInfo* event_out, Handle debug);

/**
 * @brief Continues a debugging session.
 * @return Result code.
 * @note Syscall number 0x64.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning Only exists on [3.0.0+]. For older versions use \ref svcLegacyContinueDebugEvent.
 */
Result svcContinueDebugEvent(Handle debug, u32 flags, u64* tid_list, u32 num_tids);

/**
 * @brief Continues a debugging session.
 * @return Result code.
 * @note Syscall number 0x64.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning Only exists on [1.0.0-2.3.0]. For newer versions use \ref svcContinueDebugEvent.
 */
Result svcLegacyContinueDebugEvent(Handle debug, u32 flags, u64 threadID);

/**
 * @brief Gets the context (dump the registers) of a thread in a debugging session.
 * @return Result code.
 * @param[out] ctx Output thread context (register dump).
 * @param[in] debug Debug handle.
 * @param[in] threadID ID of the thread to dump the context of.
 * @param[in] flags Register groups to select, combination of @ref RegisterGroup flags.
 * @note Syscall number 0x67.
 * @warning Official kernel will not dump any CPU GPR if the thread is currently executing a system call (except @ref svcBreak and @ref svcReturnFromException).
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetDebugThreadContext(ThreadContext* ctx, Handle debug, u64 threadID, u32 flags);

/**
 * @brief Gets the context (dump the registers) of a thread in a debugging session.
 * @return Result code.
 * @param[in] debug Debug handle.
 * @param[in] threadID ID of the thread to set the context of.
 * @param[in] ctx Input thread context (register dump).
 * @param[in] flags Register groups to select, combination of @ref RegisterGroup flags.
 * @note Syscall number 0x68.
 * @warning Official kernel will return an error if the thread is currently executing a system call (except @ref svcBreak and @ref svcReturnFromException).
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetDebugThreadContext(Handle debug, u64 threadID, const ThreadContext* ctx, u32 flags);

///@}

///@name Process and thread management
///@{

/**
 * @brief Retrieves a list of all running processes.
 * @return Result code.
 * @note Syscall number 0x65.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetProcessList(s32 *num_out, u64 *pids_out, u32 max_pids);

/**
 * @brief Retrieves a list of all threads for a debug handle (or zero).
 * @return Result code.
 * @note Syscall number 0x66.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetThreadList(s32 *num_out, u64 *tids_out, u32 max_tids, Handle debug);

///@}

///@name Debugging
///@{

/**
 * @brief Queries memory information from a process that is being debugged.
 * @return Result code.
 * @note Syscall number 0x69.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcQueryDebugProcessMemory(MemoryInfo* meminfo_ptr, u32* pageinfo, Handle debug, u64 addr);

/**
 * @brief Reads memory from a process that is being debugged.
 * @return Result code.
 * @note Syscall number 0x6A.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcReadDebugProcessMemory(void* buffer, Handle debug, u64 addr, u64 size);

/**
 * @brief Writes to memory in a process that is being debugged.
 * @return Result code.
 * @note Syscall number 0x6B.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcWriteDebugProcessMemory(Handle debug, const void* buffer, u64 addr, u64 size);

/**
 * @brief Sets one of the hardware breakpoints.
 * @return Result code.
 * @note Syscall number 0x6C.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetHardwareBreakPoint(u32 which, u64 flags, u64 value);

/**
 * @brief Gets parameters from a thread in a debugging session.
 * @return Result code.
 * @note Syscall number 0x6D.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetDebugThreadParam(u64* out_64, u32* out_32, Handle debug, u64 threadID, DebugThreadParam param);

///@}

///@name Miscellaneous
///@{

/**
 * @brief Retrieves privileged information about the system, or a certain kernel object.
 * @param[out] out Variable to which store the information.
 * @param[in] id0 First ID of the property to retrieve.
 * @param[in] handle Handle of the object to retrieve information from, or \ref INVALID_HANDLE to retrieve information about the system.
 * @param[in] id1 Second ID of the property to retrieve.
 * @return Result code.
 * @remark The full list of property IDs can be found on the <a href="https://switchbrew.org/wiki/SVC#svcGetSystemInfo">switchbrew.org wiki</a>.
 * @note Syscall number 0x6F.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetSystemInfo(u64* out, u64 id0, Handle handle, u64 id1);

///@}

///@name Inter-process communication (IPC)
///@{

/**
 * @brief Creates a port.
 * @return Result code.
 * @note Syscall number 0x70.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreatePort(Handle* portServer, Handle *portClient, s32 max_sessions, bool is_light, const char* name);

/**
 * @brief Manages a named port.
 * @return Result code.
 * @note Syscall number 0x71.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcManageNamedPort(Handle* portServer, const char* name, s32 maxSessions);

/**
 * @brief Manages a named port.
 * @return Result code.
 * @note Syscall number 0x72.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcConnectToPort(Handle* session, Handle port);

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
 * @param[in] src Source mapping address.
 * @param[in] size Size of the memory.
 * @return Result code.
 * @remark This allows mapping code and rodata with RW- permission.
 * @note Syscall number 0x74.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcMapProcessMemory(void* dst, Handle proc, u64 src, u64 size);

/**
 * @brief Undoes the effects of \ref svcMapProcessMemory.
 * @param[in] dst Destination mapping address
 * @param[in] proc Process handle.
 * @param[in] src Address of the memory in the process.
 * @param[in] size Size of the memory.
 * @return Result code.
 * @remark This allows mapping code and rodata with RW- permission.
 * @note Syscall number 0x75.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcUnmapProcessMemory(void* dst, Handle proc, u64 src, u64 size);

/**
 * @brief Equivalent to \ref svcQueryMemory, for another process.
 * @param[out] meminfo_ptr \ref MemoryInfo structure which will be filled in.
 * @param[out] pageinfo Page information which will be filled in.
 * @param[in] proc Process handle.
 * @param[in] addr Address to query.
 * @return Result code.
 * @note Syscall number 0x76.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcQueryProcessMemory(MemoryInfo* meminfo_ptr, u32 *pageinfo, Handle proc, u64 addr);

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

/**
 * @brief Creates a new process.
 * @return Result code.
 * @note Syscall number 0x79.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateProcess(Handle* out, const void* proc_info, const u32* caps, u64 cap_num);

/**
 * @brief Starts executing a freshly created process.
 * @return Result code.
 * @note Syscall number 0x7A.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcStartProcess(Handle proc, s32 main_prio, s32 default_cpu, u32 stack_size);

/**
 * @brief Terminates a running process.
 * @return Result code.
 * @note Syscall number 0x7B.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcTerminateProcess(Handle proc);

/**
 * @brief Gets a \ref ProcessInfoType for a process.
 * @return Result code.
 * @note Syscall number 0x7C.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcGetProcessInfo(s64 *out, Handle proc, ProcessInfoType which);

///@}

///@name Resource Limit Management
///@{

/**
 * @brief Creates a new Resource Limit handle.
 * @return Result code.
 * @note Syscall number 0x7D.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcCreateResourceLimit(Handle* out);

/**
 * @brief Sets the value for a \ref LimitableResource for a Resource Limit handle.
 * @return Result code.
 * @note Syscall number 0x7E.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetResourceLimitLimitValue(Handle reslimit, LimitableResource which, u64 value);

///@}

///@name ( ͡° ͜ʖ ͡°)
///@{

/**
 * @brief Calls a secure monitor function (TrustZone, EL3).
 * @param regs Arguments to pass to the secure monitor.
 * @note Syscall number 0x7F.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
void svcCallSecureMonitor(SecmonArgs* regs);

///@}

///@name Memory management
///@{

/**
 * @brief Maps new insecure memory at the desired address. [15.0.0+]
 * @return Result code.
 * @note Syscall number 0x90.
 */
Result svcMapInsecurePhysicalMemory(void *address, u64 size);

/**
 * @brief Undoes the effects of \ref svcMapInsecureMemory. [15.0.0+]
 * @return Result code.
 * @note Syscall number 0x91.
 */
Result svcUnmapInsecurePhysicalMemory(void *address, u64 size);

///@}
