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

// pmbm

Result pmbmGetBootMode(PmBootMode *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "PmBootMode");
    return serviceDispatchOut(&g_pmbmSrv, 0, *out);
}

Result pmbmSetMaintenanceBoot(void) {
    return serviceDispatch(&g_pmbmSrv, 1);
}

static Result _pmCmdInU64OutU64(u64* out, u64 in, Service* srv, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, in, *out);
}

static Result _pmCmdInU64(u64 in, Service* srv, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _pmCmdVoid(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _pmCmdOutResourceLimitValues(PmResourceLimitValues* out, Service* srv, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

// pmdmnt

Result pmdmntGetJitDebugProcessIdList(u32* out_count, u64* out_pids, size_t max_pids) {
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
    return _pmCmdInU64(pid, &g_pmdmntSrv, hosversionAtLeast(5,0,0) ? 1 : 2);
}

Result pmdmntGetProcessId(u64* pid_out, u64 program_id) {
    return _pmCmdInU64OutU64(pid_out, program_id, &g_pmdmntSrv, hosversionAtLeast(5,0,0) ? 2 : 3);
}

Result pmdmntHookToCreateProcess(Event* out_event, u64 program_id) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 3 : 4;
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(&g_pmdmntSrv, cmd_id, program_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );
    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, true);
    return rc;
}

Result pmdmntGetApplicationProcessId(u64* pid_out) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 4 : 5;
    return serviceDispatchOut(&g_pmdmntSrv, cmd_id, *pid_out);
}

Result pmdmntHookToCreateApplicationProcess(Event* out_event) {
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

Result pmdmntClearHook(u32 which) {
    if (hosversionBefore(6,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return serviceDispatchIn(&g_pmdmntSrv, 6, which);
}

Result pmdmntGetProgramId(u64* program_id_out, u64 pid) {
    if (!hosversionIsAtmosphere() && hosversionBefore(14,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdInU64OutU64(program_id_out, pid, &g_pmdmntSrv, 7);
}

// pminfo

Result pminfoGetProgramId(u64* program_id_out, u64 pid) {
    return _pmCmdInU64OutU64(program_id_out, pid, &g_pminfoSrv, 0);
}

Result pminfoGetAppletCurrentResourceLimitValues(PmResourceLimitValues* out) {
    if (!hosversionIsAtmosphere() && hosversionBefore(14,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdOutResourceLimitValues(out, &g_pminfoSrv, 1);
}

Result pminfoGetAppletPeakResourceLimitValues(PmResourceLimitValues* out) {
    if (!hosversionIsAtmosphere() && hosversionBefore(14,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdOutResourceLimitValues(out, &g_pminfoSrv, 2);
}

// pmshell

Result pmshellLaunchProgram(u32 launch_flags, const NcmProgramLocation *location, u64 *pid) {
    const struct {
        u32 launch_flags;
        u32 pad;
        NcmProgramLocation location;
    } in = { launch_flags, 0, *location };
    return serviceDispatchInOut(&g_pmshellSrv, 0, in, *pid);
}

Result pmshellTerminateProcess(u64 processID) {
    return _pmCmdInU64(processID, &g_pmshellSrv, 1);
}

Result pmshellTerminateProgram(u64 program_id) {
    return _pmCmdInU64(program_id, &g_pmshellSrv, 2);
}

Result pmshellGetProcessEventHandle(Event* out_event) {
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

Result pmshellCleanupProcess(u64 pid) {
    if (hosversionAtLeast(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdInU64(pid, &g_pmshellSrv, 5);
}

Result pmshellClearJitDebugOccured(u64 pid) {
    if (hosversionAtLeast(5,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdInU64(pid, &g_pmshellSrv, 6);
}

Result pmshellNotifyBootFinished(void) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 5 : 7;
    return serviceDispatch(&g_pmshellSrv, cmd_id);
}

Result pmshellGetApplicationProcessIdForShell(u64* pid_out) {
    const u64 cmd_id = hosversionAtLeast(5,0,0) ? 6 : 8;
    return serviceDispatchOut(&g_pmshellSrv, cmd_id, *pid_out);
}

Result pmshellBoostSystemMemoryResourceLimit(u64 boost_size) {
    if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdInU64(boost_size, &g_pmshellSrv, hosversionAtLeast(5,0,0) ? 7 : 9);
}

Result pmshellBoostApplicationThreadResourceLimit(void) {
    if (!hosversionIsAtmosphere() && hosversionBefore(7,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdVoid(&g_pmshellSrv, 8);
}

Result pmshellBoostSystemThreadResourceLimit(void) {
    if (!hosversionIsAtmosphere() && hosversionBefore(14,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdVoid(&g_pmshellSrv, 10);
}

Result pmshellGetProcessId(u64* pid_out, u64 program_id) {
    if (!hosversionIsAtmosphere() && hosversionBefore(19,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _pmCmdInU64OutU64(pid_out, program_id, &g_pmshellSrv, 12);
}
