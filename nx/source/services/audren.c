#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/tmem.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/sm.h"
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

static Result _audrenOpenAudioRenderer(Service* audren_mgr, const AudioRendererParameter* param, u64 aruid);
static Result _audrenGetWorkBufferSize(Service* audren_mgr, const AudioRendererParameter* param, u64* out_size);
static Result _audrenQuerySystemEvent(void);

Result audrenInitialize(const AudioRendererConfig* config)
{
    Result rc;
    if (serviceIsActive(&g_audrenIAudioRenderer))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

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

    // Get aruid
    u64 aruid = 0;
    rc = appletGetAppletResourceUserId(&aruid);
    //if (R_FAILED(rc)) return rc; // apparently audren still inits fine with aruid = 0 so this isn't a fatal error condition

    // Open IAudioRendererManager
    Service audrenMgrSrv;
    rc = smGetService(&audrenMgrSrv, "audren:u");
    if (R_SUCCEEDED(rc))
    {
        // Get required work buffer size
        size_t workBufSize = 0;
        rc = _audrenGetWorkBufferSize(&audrenMgrSrv, &param, &workBufSize);
        if (R_SUCCEEDED(rc))
        {
            // Create transfermem work buffer object
            workBufSize = (workBufSize + 0xFFF) &~ 0xFFF; // 1.x fails hard and returns a non-page-aligned work buffer size
            rc = tmemCreate(&g_audrenWorkBuf, workBufSize, Perm_None);
            if (R_SUCCEEDED(rc))
            {
                // Create the IAudioRenderer service
                rc = _audrenOpenAudioRenderer(&audrenMgrSrv, &param, aruid);
                if (R_SUCCEEDED(rc))
                {
                    // Finally, get the handle to the system event
                    rc = _audrenQuerySystemEvent();
                    if (R_FAILED(rc))
                        serviceClose(&g_audrenIAudioRenderer);
                }
                if (R_FAILED(rc))
                    tmemClose(&g_audrenWorkBuf);
            }
        }
        serviceClose(&audrenMgrSrv);
    }
    return rc;
}

void audrenExit(void)
{
    if (!serviceIsActive(&g_audrenIAudioRenderer))
        return;

    eventClose(&g_audrenEvent);
    serviceClose(&g_audrenIAudioRenderer);
    tmemClose(&g_audrenWorkBuf);
}

void audrenWaitFrame(void)
{
    eventWait(&g_audrenEvent, U64_MAX);
}

Result _audrenOpenAudioRenderer(Service* audren_mgr, const AudioRendererParameter* param, u64 aruid)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;

        AudioRendererParameter param;
        u64 work_buffer_size;
        u64 aruid;
    } *raw;

    ipcSendPid(&c);
    ipcSendHandleCopy(&c, g_audrenWorkBuf.handle);
    ipcSendHandleCopy(&c, CUR_PROCESS_HANDLE);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->param = *param;
    raw->work_buffer_size = g_audrenWorkBuf.size;
    raw->aruid = aruid;

    Result rc = serviceIpcDispatch(audren_mgr);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreate(&g_audrenIAudioRenderer, r.Handles[0]);
    }

    return rc;
}

Result _audrenGetWorkBufferSize(Service* audren_mgr, const AudioRendererParameter* param, size_t* out_size)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;

        AudioRendererParameter param;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->param = *param;

    Result rc = serviceIpcDispatch(audren_mgr);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 work_buf_size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out_size)
            *out_size = resp->work_buf_size;
    }

    return rc;
}

Result audrenGetState(u32* out_state)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(&g_audrenIAudioRenderer);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 state;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out_state)
            *out_state = resp->state;
    }

    return rc;
}

Result audrenRequestUpdateAudioRenderer(const void* in_param_buf, size_t in_param_buf_size, void* out_param_buf, size_t out_param_buf_size, void* perf_buf, size_t perf_buf_size)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddSendBuffer(&c, in_param_buf, in_param_buf_size, BufferType_Normal);
    ipcAddRecvBuffer(&c, out_param_buf, out_param_buf_size, BufferType_Normal);
    ipcAddRecvBuffer(&c, perf_buf, perf_buf_size, BufferType_Normal);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_audrenIAudioRenderer);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result audrenStartAudioRenderer(void)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = serviceIpcDispatch(&g_audrenIAudioRenderer);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result audrenStopAudioRenderer(void)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;

    Result rc = serviceIpcDispatch(&g_audrenIAudioRenderer);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result _audrenQuerySystemEvent(void)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&g_audrenIAudioRenderer);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            eventLoadRemote(&g_audrenEvent, r.Handles[0], true);
    }

    return rc;
}

Result audrenSetAudioRendererRenderingTimeLimit(int percent)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int percent;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->percent = percent;

    Result rc = serviceIpcDispatch(&g_audrenIAudioRenderer);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
