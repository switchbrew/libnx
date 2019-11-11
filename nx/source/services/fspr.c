#include "service_guard.h"
#include "services/fspr.h"
#include "runtime/hosversion.h"

static Service g_fsprSrv;

NX_GENERATE_SERVICE_GUARD(fspr);

Result _fsprInitialize(void) {
    Result rc = smGetService(&g_fsprSrv, "fsp-pr");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_fsprSrv);
    }

    if (R_SUCCEEDED(rc) && hosversionAtLeast(4,0,0)) {
        rc = fsprSetCurrentProcess();
    }

    return rc;
}

void _fsprCleanup(void) {
    serviceClose(&g_fsprSrv);
}

Service* fsprGetServiceSession(void) {
    return &g_fsprSrv;
}

/* Default access controls -- these will give full filesystem permissions to the requester. */
static const uint32_t g_fspr_default_fah[] = {0x1, 0xFFFFFFFF, 0xFFFFFFFF, 0x1C, 0, 0x1C, 0};
static const uint32_t g_fspr_default_fac[] = {0x1, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF};

Result fsprRegisterProgram(u64 pid, u64 tid, NcmStorageId sid, const void *fs_access_header, size_t fah_size, const void *fs_access_control, size_t fac_size) {
    if (fs_access_header == NULL) {
        fs_access_header = g_fspr_default_fah;
        fah_size = sizeof(g_fspr_default_fah);
    }
    if (fs_access_control == NULL) {
        fs_access_control = g_fspr_default_fac;
        fac_size = sizeof(g_fspr_default_fac);
    }

    const struct {
        u8 sid;
        u8 pad[7];
        u64 pid;
        u64 tid;
        u64 fah_size;
        u64 fac_size;
    } in = { sid, {0}, pid, tid, fah_size, fac_size };
    serviceAssumeDomain(&g_fsprSrv);
    return serviceDispatchIn(&g_fsprSrv, 0, in,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
        },
        .buffers = {
            { fs_access_header,  fah_size },
            { fs_access_control, fac_size },
        },
    );
}

Result fsprUnregisterProgram(u64 pid) {
    serviceAssumeDomain(&g_fsprSrv);
    return serviceDispatchIn(&g_fsprSrv, 1, pid);
}

Result fsprSetCurrentProcess(void) {
    if(hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 pid_placeholder = 0;
    serviceAssumeDomain(&g_fsprSrv);
    return serviceDispatchIn(&g_fsprSrv, 2, pid_placeholder, .in_send_pid = true);
}

Result fsprSetEnabledProgramVerification(bool enabled) {
    const u8 in = enabled != 0;
    serviceAssumeDomain(&g_fsprSrv);
    return serviceDispatchIn(&g_fsprSrv, 256, in);
}
