#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/pgl.h"
#include "runtime/hosversion.h"

static union {
    Service cmif;
    TipcService tipc;
} g_pglSrv;

NX_GENERATE_SERVICE_GUARD(pgl);

static bool _pglShouldUseTipc(void) {
    return hosversionAtLeast(12,0,0);
}

Result _pglInitialize(void) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (_pglShouldUseTipc()) {
        Handle handle;
        Result rc = smGetServiceOriginal(&handle, smEncodeName("pgl"));

        if (R_SUCCEEDED(rc)) {
            tipcCreate(&g_pglSrv.tipc, handle);
        }

        return rc;
    } else {
        return smGetService(&g_pglSrv.cmif, "pgl");
    }
}

void _pglCleanup(void) {
    if (_pglShouldUseTipc()) {
        tipcClose(&g_pglSrv.tipc);
    } else {
        serviceClose(&g_pglSrv.cmif);
    }
}

Service* pglGetServiceSessionCmif(void) {
    return &g_pglSrv.cmif;
}

TipcService* pglGetServiceSessionTipc(void) {
    return &g_pglSrv.tipc;
}

static Result _pglCmdInBool(bool inval, u32 cmd_id) {
    const u8 in = inval;
    if (_pglShouldUseTipc()) {
        return tipcDispatchIn(&g_pglSrv.tipc, cmd_id, in);
    } else {
        return serviceDispatchIn(&g_pglSrv.cmif, cmd_id, in);
    }
}

static Result _pglCmdOutBool(bool *out, u32 cmd_id) {
    u8 outval = 0;
    Result rc;

    if (_pglShouldUseTipc()) {
        rc = tipcDispatchOut(&g_pglSrv.tipc, cmd_id, outval);
    } else {
        rc = serviceDispatchOut(&g_pglSrv.cmif, cmd_id, outval);
    }

    if (R_SUCCEEDED(rc)) {
        if (out) *out = outval & 1;
    }
    return rc;
}

static Result _pglCmdInU64(u64 inval, u32 cmd_id) {
    if (_pglShouldUseTipc()) {
        return tipcDispatchIn(&g_pglSrv.tipc, cmd_id, inval);
    } else {
        return serviceDispatchIn(&g_pglSrv.cmif, cmd_id, inval);
    }
}

Result pglLaunchProgram(u64 *out_pid, const NcmProgramLocation *loc, u32 pm_launch_flags, u8 pgl_launch_flags) {
    if (_pglShouldUseTipc()) {
        const struct {
            NcmProgramLocation loc;
            u32 pm_flags;
            u8 pgl_flags;
        } in = { *loc, pm_launch_flags, pgl_launch_flags };

        return tipcDispatchInOut(&g_pglSrv.tipc, 0, in, *out_pid);
    } else {
        const struct {
            u8 pgl_flags;
            u32 pm_flags;
            NcmProgramLocation loc;
        } in = { pgl_launch_flags, pm_launch_flags, *loc };

        return serviceDispatchInOut(&g_pglSrv.cmif, 0, in, *out_pid);
    }
}

Result pglTerminateProcess(u64 pid) {
    return _pglCmdInU64(pid, 1);
}

Result pglLaunchProgramFromHost(u64 *out_pid, const char *content_path, u32 pm_launch_flags) {
    if (_pglShouldUseTipc()) {
        return tipcDispatchInOut(&g_pglSrv.tipc, 2, pm_launch_flags, *out_pid,
            .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
            .buffers = { { content_path, strlen(content_path) + 1 } },
        );
    } else {
        return serviceDispatchInOut(&g_pglSrv.cmif, 2, pm_launch_flags, *out_pid,
            .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
            .buffers = { { content_path, strlen(content_path) + 1 } },
        );
    }
}

Result pglGetHostContentMetaInfo(PglContentMetaInfo *out, const char *content_path) {
    if (_pglShouldUseTipc()) {
        return tipcDispatchOut(&g_pglSrv.tipc, 4, *out,
            .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
            .buffers = { { content_path, strlen(content_path) + 1 } },
        );
    } else {
        return serviceDispatchOut(&g_pglSrv.cmif, 4, *out,
            .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
            .buffers = { { content_path, strlen(content_path) + 1 } },
        );
    }
}

Result pglGetApplicationProcessId(u64 *out) {
    if (_pglShouldUseTipc()) {
        return tipcDispatchOut(&g_pglSrv.tipc, 5, *out);
    } else {
        return serviceDispatchOut(&g_pglSrv.cmif, 5, *out);
    }
}

Result pglBoostSystemMemoryResourceLimit(u64 size) {
    return _pglCmdInU64(size, 6);
}

Result pglIsProcessTracked(bool *out, u64 pid) {
    u8 outval = 0;
    Result rc;

    if (_pglShouldUseTipc()) {
        rc = tipcDispatchInOut(&g_pglSrv.tipc, 7, pid, outval);
    } else {
        rc = serviceDispatchInOut(&g_pglSrv.cmif, 7, pid, outval);
    }

    if (R_SUCCEEDED(rc)) {
        if (out) *out = outval & 1;
    }
    return rc;
}

Result pglEnableApplicationCrashReport(bool en) {
    return _pglCmdInBool(en, 8);
}

Result pglIsApplicationCrashReportEnabled(bool *out) {
    return _pglCmdOutBool(out, 9);
}

Result pglEnableApplicationAllThreadDumpOnCrash(bool en) {
    return _pglCmdInBool(en, 10);
}

Result pglTriggerApplicationSnapShotDumper(PglSnapShotDumpType dump_type, const char *arg) {
    if (hosversionAtLeast(12,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    _Static_assert(sizeof(dump_type) == sizeof(u32), "PglSnapShotDumpType");
    return serviceDispatchIn(&g_pglSrv.cmif, 12, dump_type,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { arg, strlen(arg) + 1 } },
    );
}

Result pglGetEventObserver(PglEventObserver *out) {
    if (_pglShouldUseTipc()) {
        return tipcDispatch(&g_pglSrv.tipc, 20,
            .out_num_objects = 1,
            .out_objects = &out->t,
        );
    } else {
        return serviceDispatch(&g_pglSrv.cmif, 20,
            .out_num_objects = 1,
            .out_objects = &out->s,
        );
    }
}

Result pglEventObserverGetProcessEvent(PglEventObserver *observer, Event *out) {
    Handle evt_handle;
    Result rc;

    if (_pglShouldUseTipc()) {
        rc = tipcDispatch(&observer->t, 0,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = &evt_handle,
        );
    } else {
        rc = serviceDispatch(&observer->s, 0,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = &evt_handle,
        );
    }

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(out, evt_handle, true);
    }

    return rc;
}

Result pglEventObserverGetProcessEventInfo(PglEventObserver *observer, PmProcessEventInfo *out) {
    if (_pglShouldUseTipc()) {
        return tipcDispatchOut(&observer->t, 1, *out);
    } else {
        return serviceDispatchOut(&observer->s, 1, *out);
    }
}

void pglEventObserverClose(PglEventObserver *observer) {
    if (_pglShouldUseTipc()) {
        tipcClose(&observer->t);
    } else {
        serviceClose(&observer->s);
    }
}
