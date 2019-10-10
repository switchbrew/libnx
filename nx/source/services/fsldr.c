// Copyright 2018 SciresM
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/fs.h"
#include "services/fsldr.h"

static Service g_fsldrSrv;

NX_GENERATE_SERVICE_GUARD(fsldr);

NX_INLINE Result _fsldrSetCurrentProcess(void);

Result _fsldrInitialize(void) {
    Result rc = smGetService(&g_fsldrSrv, "fsp-ldr");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_fsldrSrv);
    }

    if (R_SUCCEEDED(rc) && hosversionAtLeast(4,0,0)) {
        rc = _fsldrSetCurrentProcess();
    }

    return rc;
}

void _fsldrCleanup(void) {
    serviceClose(&g_fsldrSrv);
}

Service* fsldrGetServiceSession(void) {
    return &g_fsldrSrv;
}

Result fsldrOpenCodeFileSystem(u64 tid, const char *path, FsFileSystem* out) {
    char send_path[FS_MAX_PATH + 1];
    strncpy(send_path, path, FS_MAX_PATH);
    return serviceDispatchIn(&g_fsldrSrv, 0, tid,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { send_path,  FS_MAX_PATH },
        },
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result fsldrIsArchivedProgram(u64 pid, bool *out) {
    return serviceDispatchInOut(&g_fsldrSrv, 1, pid, *out);
}

Result _fsldrSetCurrentProcess(void) {
    u64 pid_placeholder = 0;
    return serviceDispatchIn(&g_fsldrSrv, 2, pid_placeholder, .in_send_pid = true);
}
