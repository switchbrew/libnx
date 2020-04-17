#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/pgl.h"
#include "runtime/hosversion.h"

static Service g_pglSrv;

NX_GENERATE_SERVICE_GUARD(pgl);

Result _pglInitialize(void) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return smGetService(&g_pglSrv, "pgl");
}

void _pglCleanup(void) {
    serviceClose(&g_pglSrv);
}

Service* pglGetServiceSession(void) {
    return &g_pglSrv;
}

static Result _pglCmdInBool(Service* srv, bool inval, u32 cmd_id) {
    const u8 in = inval;
    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _pglCmdOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 outval = 0;
    Result rc = serviceDispatchOut(srv, cmd_id, outval);
    if (R_SUCCEEDED(rc)) {
        if (out) *out = outval & 1;
    }
    return rc;
}

static Result _pglCmdInU64(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

Result pglLaunchProgram(u64 *out_pid, const NcmProgramLocation *loc, u32 pm_launch_flags, u8 pgl_launch_flags) {
    const struct {
        u8 pgl_flags;
        u32 pm_flags;
        NcmProgramLocation loc;
    } in = { pgl_launch_flags, pm_launch_flags, *loc };
    return serviceDispatchInOut(&g_pglSrv, 0, in, *out_pid);
}

Result pglTerminateProcess(u64 pid) {
    return _pglCmdInU64(&g_pglSrv, pid, 1);
}

Result pglLaunchProgramFromHost(u64 *out_pid, const char *content_path, u32 pm_launch_flags) {
    return serviceDispatchInOut(&g_pglSrv, 2, pm_launch_flags, *out_pid,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { content_path, strlen(content_path) + 1 } },
    );
}

Result pglGetHostContentMetaInfo(PglContentMetaInfo *out, const char *content_path) {
    return serviceDispatchOut(&g_pglSrv, 4, *out,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { content_path, strlen(content_path) + 1 } },
    );
}

Result pglGetApplicationProcessId(u64 *out) {
    return serviceDispatchOut(&g_pglSrv, 5, *out);
}

Result pglBoostSystemMemoryResourceLimit(u64 size) {
    return _pglCmdInU64(&g_pglSrv, size, 6);
}

Result pglIsProcessTracked(bool *out, u64 pid) {
    u8 outval = 0;
    Result rc = serviceDispatchInOut(&g_pglSrv, 7, pid, outval);
    if (R_SUCCEEDED(rc)) {
        if (out) *out = outval & 1;
    }
    return rc;
}

Result pglEnableApplicationCrashReport(bool en) {
    return _pglCmdInBool(&g_pglSrv, en, 8);
}

Result pglIsApplicationCrashReportEnabled(bool *out) {
    return _pglCmdOutBool(&g_pglSrv, out, 9);
}

Result pglEnableApplicationAllThreadDumpOnCrash(bool en) {
    return _pglCmdInBool(&g_pglSrv, en, 10);
}

Result pglTriggerApplicationSnapShotDumper(PglSnapShotDumpType dump_type, const char *arg) {
    _Static_assert(sizeof(dump_type) == sizeof(u32), "PglSnapShotDumpType");
    return serviceDispatchIn(&g_pglSrv, 12, dump_type,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { arg, strlen(arg) + 1 } },
    );
}

Result pglGetEventObserver(PglEventObserver *out) {
    return serviceDispatch(&g_pglSrv, 20,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result pglEventObserverGetProcessEvent(PglEventObserver *observer, Event *out) {
    Handle evt_handle;
    Result rc = serviceDispatch(&observer->s, 0,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &evt_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(out, evt_handle, true);
    }

    return rc;
}

Result pglEventObserverGetProcessEventInfo(PglEventObserver *observer, PmProcessEventInfo *out) {
    return serviceDispatchOut(&observer->s, 1, *out);
}

void pglEventObserverClose(PglEventObserver *observer) {
    serviceClose(&observer->s);
}
