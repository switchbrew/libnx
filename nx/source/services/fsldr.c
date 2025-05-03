// Copyright 2018 SciresM
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
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

Result fsldrOpenCodeFileSystem(FsCodeInfo* out_code_info, u64 tid, NcmStorageId storage_id, const char *path, FsContentAttributes attr, FsFileSystem* out) {
    memset(out_code_info, 0, sizeof(*out_code_info));

    char send_path[FS_MAX_PATH]={0};
    if (hosversionBefore(20,0,0))
        strncpy(send_path, path, FS_MAX_PATH-1);

    if (hosversionAtLeast(20,0,0)) {
        const struct {
            u8 attr;
            u8 storage_id;
            u64 tid;
        } in = { attr, storage_id, tid };

        serviceAssumeDomain(&g_fsldrSrv);
        return serviceDispatchIn(&g_fsldrSrv, 0, in,
            .buffer_attrs = {
                SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            },
            .buffers = {
                { out_code_info,  sizeof(*out_code_info) },
            },
            .out_num_objects = 1,
            .out_objects = &out->s,
        );
    } else if (hosversionAtLeast(17,0,0)) {
        const struct {
            u8 attr;
            u64 tid;
        } in = { attr, tid };

        serviceAssumeDomain(&g_fsldrSrv);
        return serviceDispatchIn(&g_fsldrSrv, 0, in,
            .buffer_attrs = {
                SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In,
                SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            },
            .buffers = {
                { send_path,  FS_MAX_PATH },
                { out_code_info,  sizeof(*out_code_info) },
            },
            .out_num_objects = 1,
            .out_objects = &out->s,
        );
    } else if (hosversionAtLeast(16,0,0)) {
        const struct {
            u8 attr;
            u64 tid;
        } in = { attr, tid };

        serviceAssumeDomain(&g_fsldrSrv);
        return serviceDispatchIn(&g_fsldrSrv, 0, in,
            .buffer_attrs = {
                SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out,
                SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In,
            },
            .buffers = {
                { out_code_info,  sizeof(*out_code_info) },
                { send_path,  FS_MAX_PATH },
            },
            .out_num_objects = 1,
            .out_objects = &out->s,
        );
    } else if (hosversionAtLeast(10,0,0)) {
        serviceAssumeDomain(&g_fsldrSrv);
        return serviceDispatchIn(&g_fsldrSrv, 0, tid,
            .buffer_attrs = {
                SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out,
                SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In,
            },
            .buffers = {
                { out_code_info,  sizeof(*out_code_info) },
                { send_path,  FS_MAX_PATH },
            },
            .out_num_objects = 1,
            .out_objects = &out->s,
        );
    } else {
        serviceAssumeDomain(&g_fsldrSrv);
        return serviceDispatchIn(&g_fsldrSrv, 0, tid,
            .buffer_attrs = {
                SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In,
            },
            .buffers = {
                { send_path,  FS_MAX_PATH },
            },
            .out_num_objects = 1,
            .out_objects = &out->s,
        );
    }

}

Result fsldrIsArchivedProgram(u64 pid, bool *out) {
    serviceAssumeDomain(&g_fsldrSrv);
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_fsldrSrv, 1, pid, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result _fsldrSetCurrentProcess(void) {
    u64 pid_placeholder = 0;
    serviceAssumeDomain(&g_fsldrSrv);
    return serviceDispatchIn(&g_fsldrSrv, 2, pid_placeholder, .in_send_pid = true);
}
