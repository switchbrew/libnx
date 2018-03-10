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

/// Builds a result code from its constituent components.
#define MAKERESULT(module,description) \
    ((((module)&0x1FF)) | ((description)&0x1FFF)<<9)

/// Module values
enum {
    Module_Kernel=1,
    Module_Libnx=345,
    Module_LibnxNvidia=348,
};

/// Kernel error codes
enum {
    KernelError_Timeout=117,
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
