// Copyright 2017 plutoo
#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/pm.h"

#define PM_GENERATE_SERVICE_INIT(name)                  \
static Service g_pm##name##Srv;                         \
                                                        \
NX_GENERATE_SERVICE_GUARD(pm##name);                    \
                                                        \
Result _pm##name##Initialize(void) {                    \
    return smGetService(&g_pm##name##Srv, "pm:"#name);  \
}                                                       \
                                                        \
void _pm##name##Cleanup(void) {                         \
    serviceClose(&g_pm##name##Srv);                      \
}                                                       \
                                                        \
Service* pm##name##GetServiceSession(void) {            \
    return &g_pm##name##Srv;                            \
}

PM_GENERATE_SERVICE_INIT(dmnt);
PM_GENERATE_SERVICE_INIT(shell);
PM_GENERATE_SERVICE_INIT(info);
PM_GENERATE_SERVICE_INIT(bm);

Result pmdmntGetDebugProcesses(u32* out_count, u64* out_pids, size_t max_pids) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 0 : 1;
    return serviceDispatchOut(&g_pmdmntSrv, cmd_id, *out_count,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { out_pids,  max_pids * sizeof(*out_pids) },
        },
    );
}

Result pmdmntStartProcess(u64 pid) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 1 : 2;
    return serviceDispatchIn(&g_pmdmntSrv, cmd_id, pid);
}

Result pmdmntGetTitlePid(u64* pid_out, u64 title_id) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 2 : 3;
    return serviceDispatchInOut(&g_pmdmntSrv, cmd_id, title_id, *pid_out);
}

Result pmdmntEnableDebugForTitleId(Event* out_event, u64 title_id) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 3 : 4;
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(&g_pmdmntSrv, cmd_id, title_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );
    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, true);
    return rc;
}

Result pmdmntGetApplicationPid(u64* pid_out) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 4 : 5;
    return serviceDispatchOut(&g_pmdmntSrv, cmd_id, *pid_out);
}

Result pmdmntEnableDebugForApplication(Event* out_event) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 5 : 6;
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(&g_pmdmntSrv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );
    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, true);
    return rc;
}

Result pmdmntDisableDebug(u32 which) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_pmdmntSrv, 6, which);
}

Result pmshellLaunchProcess(u32 launch_flags, u64 titleID, u64 storageID, u64 *pid) {
    const struct {
        u32 launch_flags;
        u64 titleID;
        u64 storageID;
    } in = { launch_flags, titleID, storageID };
    return serviceDispatchInOut(&g_pmshellSrv, 0, in, *pid);
}

Result pmshellTerminateProcessByProcessId(u64 processID) {
    return serviceDispatchIn(&g_pmshellSrv, 1, processID);
}

Result pmshellTerminateProcessByTitleId(u64 titleID) {
    return serviceDispatchIn(&g_pmshellSrv, 2, titleID);
}

Result pmshellGetProcessEvent(Event* out_event) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(&g_pmshellSrv, 3,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );
    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, true);
    return rc;
}

Result pmshellGetProcessEventInfo(PmProcessEventInfo* out) {
    _Static_assert(sizeof(out->event) == sizeof(u32), "PmProcessEvent");
    return serviceDispatchOut(&g_pmshellSrv, 4, *out);
}

Result pmshellFinalizeDeadProcess(u64 pid) {
    if (hosversionAtLeast(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_pmshellSrv, 5, pid);
}

Result pmshellClearProcessExceptionOccurred(u64 pid) {
    if (hosversionAtLeast(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_pmshellSrv, 6, pid);
}

Result pmshellNotifyBootFinished(void) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 5 : 7;
    return serviceDispatch(&g_pmshellSrv, cmd_id);
}

Result pmshellGetApplicationPid(u64* pid_out) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 6 : 8;
    return serviceDispatchOut(&g_pmshellSrv, cmd_id, *pid_out);
}

Result pmshellBoostSystemMemoryResourceLimit(u64 boost_size) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 7 : 9;
    return serviceDispatchIn(&g_pmshellSrv, cmd_id, boost_size);
}

Result pmshellBoostSystemThreadResourceLimit(void) {
    if (hosversionBefore(7,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatch(&g_pmshellSrv, 8);
}

Result pminfoGetTitleId(u64* title_id_out, u64 pid) {
    return serviceDispatchInOut(&g_pminfoSrv, 0, pid, *title_id_out);
}

Result pmbmGetBootMode(PmBootMode *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "PmBootMode");
    return serviceDispatchOut(&g_pmbmSrv, 0, *out);
}

Result pmbmSetMaintenanceBoot(void) {
    return serviceDispatch(&g_pmbmSrv, 1);
}
