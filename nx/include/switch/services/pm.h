/**
 * @file pm.h
 * @brief Process management (pm*) service IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @author mdbell
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/sm.h"

typedef enum {
    PmLaunchFlag_None           = 0,

    ///< PmLaunchFlag_* should be used on 5.0.0+.
    PmLaunchFlag_SignalOnExit   = (1 << 0),
    PmLaunchFlag_SignalOnStart  = (1 << 1),
    PmLaunchFlag_SignalOnCrash  = (1 << 2),
    PmLaunchFlag_SignalOnDebug  = (1 << 3),
    PmLaunchFlag_StartSuspended = (1 << 4),
    PmLaunchFlag_DisableAslr    = (1 << 5),

    ///< PmLaunchFlagOld_* should be used on 1.0.0-4.1.0.
    PmLaunchFlagOld_SignalOnExit   = (1 << 0),
    PmLaunchFlagOld_StartSuspended = (1 << 1),
    PmLaunchFlagOld_SignalOnCrash  = (1 << 2),
    PmLaunchFlagOld_DisableAslr    = (1 << 3),
    PmLaunchFlagOld_SignalOnDebug  = (1 << 4),
    ///< PmLaunchFlagOld_SignalOnStart was added in 2.0.0.
    PmLaunchFlagOld_SignalOnStart  = (1 << 5),
} PmLaunchFlag;

typedef enum {
    PmProcessEvent_None = 0,
    PmProcessEvent_Exit = 1,
    PmProcessEvent_Start = 2,
    PmProcessEvent_Crash = 3,
    PmProcessEvent_DebugStart = 4,
    PmProcessEvent_DebugBreak = 5,
} PmProcessEvent;

typedef struct {
    PmProcessEvent event;
    u64 process_id;
} PmProcessEventInfo;

typedef enum {
    PmBootMode_Normal      = 0,
    PmBootMode_Maintenance = 1,
    PmBootMode_SafeMode    = 2,
} PmBootMode;

Result pmdmntInitialize(void);
void pmdmntExit(void);

Service* pmdmntGetServiceSession(void);

Result pminfoInitialize(void);
void pminfoExit(void);

Service* pminfoGetServiceSession(void);

Result pmshellInitialize(void);
void pmshellExit(void);

Service* pmshellGetServiceSession(void);

Result pmbmInitialize();
void pmbmExit();

Service* pmbmGetServiceSession(void);

Result pmdmntGetDebugProcesses(u32* out_count, u64* out_pids, size_t max_pids);
Result pmdmntStartProcess(u64 pid);
Result pmdmntGetTitlePid(u64* pid_out, u64 title_id);
Result pmdmntEnableDebugForTitleId(Handle* handle_out, u64 title_id);
Result pmdmntGetApplicationPid(u64* pid_out);
Result pmdmntEnableDebugForApplication(Handle* handle_out);
Result pmdmntDisableDebug(void);

Result pminfoGetTitleId(u64* title_id_out, u64 pid);

Result pmshellLaunchProcess(u32 launch_flags, u64 titleID, u64 storageID, u64 *pid);
Result pmshellTerminateProcessByProcessId(u64 processID);
Result pmshellTerminateProcessByTitleId(u64 titleID);
Result pmshellGetProcessEvent(Event* out); // Autoclear for pmshellProcessEvent is always true.
Result pmshellGetProcessEventInfo(PmProcessEventInfo* out);
Result pmshellFinalizeDeadProcess(u64 pid);
Result pmshellClearProcessExceptionOccurred(u64 pid);
Result pmshellNotifyBootFinished(void);
Result pmshellGetApplicationPid(u64* pid_out);
Result pmshellBoostSystemMemoryResourceLimit(u64 boost_size);
Result pmshellBoostSystemThreadResourceLimit(void);

Result pmbmGetBootMode(PmBootMode *out);
Result pmbmSetMaintenanceBoot(void);
