#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/audrec.h"

NX_GENERATE_SERVICE_GUARD(audrec);
static Service g_audrecUserSrv;

Result _audrecInitialize(void) {
    return smGetService(&g_audrecUserSrv, "audrec:u");
}

void _audrecCleanup(void) {
    serviceClose(&g_audrecUserSrv);
}

Service* audrecGetServiceSession(void) {
    return &g_audrecUserSrv;
}

Result audrecOpenFinalOutputRecorder(AudrecRecorder* recorder_out, FinalOutputRecorderParameter* param_in, u64 aruid, FinalOutputRecorderParameterInternal* param_out) {
    const struct {
        FinalOutputRecorderParameter param;
        u64 aruid;
    } in = { *param_in, aruid };

    struct {
        FinalOutputRecorderParameterInternal param;
    } out;

    Result rc = serviceDispatchInOut(&g_audrecUserSrv, 0, in, out,
        .in_num_handles = 1,
        .in_handles = { CUR_PROCESS_HANDLE },
        .out_num_objects = 1,
        .out_objects = &recorder_out->s,
    );

    if (R_SUCCEEDED(rc)) {
        *param_out = out.param;
    }

    return rc;
}

Result audrecRecorderStart(AudrecRecorder* recorder) {
    return serviceDispatch(&recorder->s, 1);
}

Result audrecRecorderStop(AudrecRecorder* recorder) {
    return serviceDispatch(&recorder->s, 2);
}

Result audrecRecorderRegisterBufferEvent(AudrecRecorder* recorder, Event* out_event) {
    Handle tmp_handle;

    Result rc = serviceDispatch(&recorder->s, 4,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(out_event, tmp_handle, 1);
    }

    return rc;
}

Result audrecRecorderAppendFinalOutputRecorderBuffer(AudrecRecorder* recorder, u64 buffer_client_ptr, FinalOutputRecorderBuffer* param) {
    const struct {
        u64 buffer_client_ptr;
    } in = { buffer_client_ptr };

    if (hosversionAtLeast(3,0,0)) {
        return serviceDispatchIn(&recorder->s, 8, in,
            .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
            .buffers = { { param, sizeof(*param) } },
        );
    }
    else {
        return serviceDispatchIn(&recorder->s, 3, in,
            .buffer_attrs = { SfBufferAttr_In },
            .buffers = { { param, sizeof(*param) } },
        );
    }
}

Result audrecRecorderGetReleasedFinalOutputRecorderBuffers(AudrecRecorder* recorder, u64* out_buffers, u64* inout_count, u64* out_released) {
    struct {
        u32 count;
        u32 padding;
        u64 released;
    } out;

    const struct {
    } in;

    Result rc;

    if (hosversionAtLeast(3,0,0)) {
        rc = serviceDispatchInOut(&recorder->s, 9, in, out,
            .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
            .buffers = { { out_buffers, sizeof(u64) * (*inout_count) } },
        );
    }
    else {
        rc = serviceDispatchInOut(&recorder->s, 5, in, out,
            .buffer_attrs = { SfBufferAttr_Out },
            .buffers = { { out_buffers, sizeof(u64) * (*inout_count) } },
        );
    }

    if (R_SUCCEEDED(rc)) {
        *inout_count = out.count;
        *out_released = out.released;
    }

    return rc;
}

void audrecRecorderClose(AudrecRecorder* recorder) {
    serviceClose(&recorder->s);
}
