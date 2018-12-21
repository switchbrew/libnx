/**
 * @file result.h
 * @brief Switch result code tools.
 * @copyright libnx Authors
 */
#pragma once
#include "types.h"

/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res)   ((res)==0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)      ((res)!=0)
/// Returns the module ID of a result code.
#define R_MODULE(res)      ((res)&0x1FF)
/// Returns the description of a result code.
#define R_DESCRIPTION(res) (((res)>>9)&0x1FFF)
/// Masks out unused bits in a result code, retrieving the actual value for use in comparisons.
#define R_VALUE(res)       ((res)&0x3FFFFF)

/// Builds a result code from its constituent components.
#define MAKERESULT(module,description) \
    ((((module)&0x1FF)) | ((description)&0x1FFF)<<9)

/// Builds a kernel error result code.
#define KERNELRESULT(description) \
    MAKERESULT(Module_Kernel, KernelError_##description)

/// Module values
enum {
    Module_Kernel=1,
    Module_Libnx=345,
    Module_HomebrewAbi=346,
    Module_HomebrewLoader=347,
    Module_LibnxNvidia=348,
    Module_LibnxBinder=349,
};

/// Kernel error codes
enum {
    KernelError_OutOfSessions=7,
    KernelError_InvalidCapabilityDescriptor=14,
    KernelError_NotImplemented=33,
    KernelError_ThreadTerminating=59,
    KernelError_OutOfDebugEvents=70,
    KernelError_InvalidSize=101,
    KernelError_InvalidAddress=102,
    KernelError_ResourceExhausted=103,
    KernelError_OutOfMemory=104,
    KernelError_OutOfHandles=105,
    KernelError_InvalidMemoryState=106,
    KernelError_InvalidMemoryPermissions=108,
    KernelError_InvalidMemoryRange=110,
    KernelError_InvalidPriority=112,
    KernelError_InvalidCoreId=113,
    KernelError_InvalidHandle=114,
    KernelError_InvalidUserBuffer=115,
    KernelError_InvalidCombination=116,
    KernelError_TimedOut=117,
    KernelError_Cancelled=118,
    KernelError_OutOfRange=119,
    KernelError_InvalidEnumValue=120,
    KernelError_NotFound=121,
    KernelError_AlreadyExists=122,
    KernelError_ConnectionClosed=123,
    KernelError_UnhandledUserInterrupt=124,
    KernelError_InvalidState=125,
    KernelError_ReservedValue=126,
    KernelError_InvalidHwBreakpoint=127,
    KernelError_FatalUserException=128,
    KernelError_OwnedByAnotherProcess=129,
    KernelError_ConnectionRefused=131,
    KernelError_OutOfResource=132,
    KernelError_IpcMapFailed=259,
    KernelError_IpcCmdbufTooSmall=260,
    KernelError_NotDebugged=520,
};

/// libnx error codes
enum {
    LibnxError_BadReloc=1,
    LibnxError_OutOfMemory,
    LibnxError_AlreadyMapped,
    LibnxError_BadGetInfo_Stack,
    LibnxError_BadGetInfo_Heap,
    LibnxError_BadQueryMemory,
    LibnxError_AlreadyInitialized,
    LibnxError_NotInitialized,
    LibnxError_NotFound,
    LibnxError_IoError,
    LibnxError_BadInput,
    LibnxError_BadReent,
    LibnxError_BufferProducerError,
    LibnxError_HandleTooEarly,
    LibnxError_HeapAllocFailed,
    LibnxError_TooManyOverrides,
    LibnxError_ParcelError,
    LibnxError_BadGfxInit,
    LibnxError_BadGfxEventWait,
    LibnxError_BadGfxQueueBuffer,
    LibnxError_BadGfxDequeueBuffer,
    LibnxError_AppletCmdidNotFound,
    LibnxError_BadAppletReceiveMessage,
    LibnxError_BadAppletNotifyRunning,
    LibnxError_BadAppletGetCurrentFocusState,
    LibnxError_BadAppletGetOperationMode,
    LibnxError_BadAppletGetPerformanceMode,
    LibnxError_BadUsbCommsRead,
    LibnxError_BadUsbCommsWrite,
    LibnxError_InitFail_SM,
    LibnxError_InitFail_AM,
    LibnxError_InitFail_HID,
    LibnxError_InitFail_FS,
    LibnxError_BadGetInfo_Rng,
    LibnxError_JitUnavailable,
    LibnxError_WeirdKernel,
    LibnxError_IncompatSysVer,
    LibnxError_InitFail_Time,
    LibnxError_TooManyDevOpTabs,
    LibnxError_DomainMessageUnknownType,
    LibnxError_DomainMessageTooManyObjectIds,
    LibnxError_AppletFailedToInitialize,
    LibnxError_ApmFailedToInitialize,
    LibnxError_NvinfoFailedToInitialize,
    LibnxError_NvbufFailedToInitialize,
    LibnxError_LibAppletBadExit,
};

/// libnx binder error codes
enum {
    LibnxBinderError_Unknown=1,
    LibnxBinderError_NoMemory,
    LibnxBinderError_InvalidOperation,
    LibnxBinderError_BadValue,
    LibnxBinderError_BadType,
    LibnxBinderError_NameNotFound,
    LibnxBinderError_PermissionDenied,
    LibnxBinderError_NoInit,
    LibnxBinderError_AlreadyExists,
    LibnxBinderError_DeadObject,
    LibnxBinderError_FailedTransaction,
    LibnxBinderError_BadIndex,
    LibnxBinderError_NotEnoughData,
    LibnxBinderError_WouldBlock,
    LibnxBinderError_TimedOut,
    LibnxBinderError_UnknownTransaction,
    LibnxBinderError_FdsNotAllowed,
};

/// libnx nvidia error codes
enum {
    LibnxNvidiaError_Unknown=1,
    LibnxNvidiaError_NotImplemented,       ///< Maps to Nvidia: 1
    LibnxNvidiaError_NotSupported,         ///< Maps to Nvidia: 2
    LibnxNvidiaError_NotInitialized,       ///< Maps to Nvidia: 3
    LibnxNvidiaError_BadParameter,         ///< Maps to Nvidia: 4
    LibnxNvidiaError_Timeout,              ///< Maps to Nvidia: 5
    LibnxNvidiaError_InsufficientMemory,   ///< Maps to Nvidia: 6
    LibnxNvidiaError_ReadOnlyAttribute,    ///< Maps to Nvidia: 7
    LibnxNvidiaError_InvalidState,         ///< Maps to Nvidia: 8
    LibnxNvidiaError_InvalidAddress,       ///< Maps to Nvidia: 9
    LibnxNvidiaError_InvalidSize,          ///< Maps to Nvidia: 10
    LibnxNvidiaError_BadValue,             ///< Maps to Nvidia: 11
    LibnxNvidiaError_AlreadyAllocated,     ///< Maps to Nvidia: 13
    LibnxNvidiaError_Busy,                 ///< Maps to Nvidia: 14
    LibnxNvidiaError_ResourceError,        ///< Maps to Nvidia: 15
    LibnxNvidiaError_CountMismatch,        ///< Maps to Nvidia: 16
    LibnxNvidiaError_SharedMemoryTooSmall, ///< Maps to Nvidia: 0x1000
    LibnxNvidiaError_FileOperationFailed,  ///< Maps to Nvidia: 0x30003
    LibnxNvidiaError_IoctlFailed,          ///< Maps to Nvidia: 0x3000F
};
