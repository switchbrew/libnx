#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "kernel/tmem.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/audren.h"

u32 g_audrenRevision;

static Service g_audrenIAudioRenderer;
static TransferMemory g_audrenWorkBuf;
static Event g_audrenEvent;

typedef struct {
    s32 sample_rate;
    s32 sample_count;
    s32 mix_buffer_count;
    s32 submix_count;
    s32 voice_count;
    s32 sink_count;
    s32 effect_count;
    s32 unk1;
    u8  unk2;
    u8  _padding1[3];
    s32 splitter_count;
    s32 unk3;
    s32 unk4;
    u32 revision;
} AudioRendererParameter;

static Result _audrenOpenAudioRenderer(Service* srv, Service* srv_out, const AudioRendererParameter* param);
static Result _audrenGetWorkBufferSize(Service* srv, const AudioRendererParameter* param, u64* out_size);
static Result _audrenQuerySystemEvent(Event* out_event);

NX_GENERATE_SERVICE_GUARD_PARAMS(audren, (const AudioRendererConfig* config), (config));

Result _audrenInitialize(const AudioRendererConfig* config) {
    Result rc=0;

    // Validate configuration
    if (config->num_voices < 1 || config->num_voices > 1024)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (config->num_effects < 0 || config->num_effects > 256)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (config->num_sinks < 1 || config->num_sinks > 256)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (config->num_mix_objs < 1 || config->num_mix_objs > 256)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (config->num_mix_buffers < 1 || config->num_mix_buffers > 256)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    // Choose revision (i.e. if splitters are used then at least revision 2 must be used)
    u32 hosver = hosversionGet();
    /*if (hosver >= MAKEHOSVERSION(6,1,0))
        g_audrenRevision = AUDREN_REVISION_6;
    else if (hosver >= MAKEHOSVERSION(6,0,0))
        g_audrenRevision = AUDREN_REVISION_5;
    else*/ if (hosver >= MAKEHOSVERSION(4,0,0))
        g_audrenRevision = AUDREN_REVISION_4;
    else if (hosver >= MAKEHOSVERSION(3,0,0))
        g_audrenRevision = AUDREN_REVISION_3;
    else if (hosver >= MAKEHOSVERSION(2,0,0))
        g_audrenRevision = AUDREN_REVISION_2;
    else
        g_audrenRevision = AUDREN_REVISION_1;

    // Prepare parameter structure
    AudioRendererParameter param = {0};
    param.sample_rate      = config->output_rate == AudioRendererOutputRate_32kHz ? 32000 : 48000;
    param.sample_count     = config->output_rate == AudioRendererOutputRate_32kHz ? AUDREN_SAMPLES_PER_FRAME_32KHZ : AUDREN_SAMPLES_PER_FRAME_48KHZ;
    param.mix_buffer_count = config->num_mix_buffers;
    param.submix_count     = config->num_mix_objs - 1;
    param.voice_count      = config->num_voices;
    param.sink_count       = config->num_sinks;
    param.effect_count     = config->num_effects;
    param.revision         = g_audrenRevision;

    // Open IAudioRendererManager
    Service audrenMgrSrv;
    rc = smGetService(&audrenMgrSrv, "audren:u");
    if (R_SUCCEEDED(rc)) {
        // Get required work buffer size
        u64 workBufSize = 0;
        rc = _audrenGetWorkBufferSize(&audrenMgrSrv, &param, &workBufSize);
        if (R_SUCCEEDED(rc)) {
            // Create transfermem work buffer object
            workBufSize = (workBufSize + 0xFFF) &~ 0xFFF; // 1.x fails hard and returns a non-page-aligned work buffer size
            rc = tmemCreate(&g_audrenWorkBuf, workBufSize, Perm_None);
            if (R_SUCCEEDED(rc)) {
                // Create the IAudioRenderer service
                rc = _audrenOpenAudioRenderer(&audrenMgrSrv, &g_audrenIAudioRenderer, &param);
                if (R_SUCCEEDED(rc)) {
                    // Finally, get the handle to the system event
                    rc = _audrenQuerySystemEvent(&g_audrenEvent);
                }
            }
        }
        serviceClose(&audrenMgrSrv);
    }
    return rc;
}

void _audrenCleanup(void) {
    eventClose(&g_audrenEvent);
    serviceClose(&g_audrenIAudioRenderer);
    tmemClose(&g_audrenWorkBuf);
}

Service* audrenGetServiceSession_AudioRenderer(void) {
    return &g_audrenIAudioRenderer;
}

Event* audrenGetFrameEvent(void) {
    return &g_audrenEvent;
}

static Result _audrenCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _audrenCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _audrenCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _audrenCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _audrenCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

void audrenWaitFrame(void) {
    eventWait(&g_audrenEvent, UINT64_MAX);
}

Result _audrenOpenAudioRenderer(Service* srv, Service* srv_out, const AudioRendererParameter* param) {
    const struct {
        AudioRendererParameter param;
        u32 pad;
        u64 work_buffer_size;
        u64 aruid;
    } in = { *param, 0, g_audrenWorkBuf.size, appletGetAppletResourceUserId() };

    return serviceDispatchIn(srv, 0, in,
        .in_send_pid = true,
        .in_num_handles = 2,
        .in_handles = { g_audrenWorkBuf.handle, CUR_PROCESS_HANDLE },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result _audrenGetWorkBufferSize(Service* srv, const AudioRendererParameter* param, u64* out_size) {
    return serviceDispatchInOut(srv, 1, *param, *out_size);
}

Result audrenGetState(u32* out_state) {
    return _audrenCmdNoInOutU32(&g_audrenIAudioRenderer, out_state, 3);
}

Result audrenRequestUpdateAudioRenderer(const void* in_param_buf, size_t in_param_buf_size, void* out_param_buf, size_t out_param_buf_size, void* perf_buf, size_t perf_buf_size) {
    bool new_cmd = hosversionAtLeast(3,0,0);

    u32 tmpattr = new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect;
    return serviceDispatch(&g_audrenIAudioRenderer, new_cmd==0 ? 4 : 10,
        .buffer_attrs = {
            tmpattr | SfBufferAttr_Out,
            tmpattr | SfBufferAttr_Out,
            tmpattr | SfBufferAttr_In,
        },
        .buffers = {
            { out_param_buf, out_param_buf_size },
            { perf_buf, perf_buf_size },
            { in_param_buf, in_param_buf_size },
        },
    );
}

Result audrenStartAudioRenderer(void) {
    return _audrenCmdNoIO(&g_audrenIAudioRenderer, 5);
}

Result audrenStopAudioRenderer(void) {
    return _audrenCmdNoIO(&g_audrenIAudioRenderer, 6);
}

Result _audrenQuerySystemEvent(Event* out_event) {
    return _audrenCmdGetEvent(&g_audrenIAudioRenderer, out_event, true, 7);
}

Result audrenSetAudioRendererRenderingTimeLimit(int percent) {
    return serviceDispatchIn(&g_audrenIAudioRenderer, 8, percent);
}
