#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "kernel/tmem.h"
#include "services/sm.h"
#include "services/grc.h"
#include "services/caps.h"
#include "services/applet.h"
#include "display/native_window.h"
#include "audio/audio.h"
#include "runtime/hosversion.h"

static void _grcGameMovieTrimmerClose(GrcGameMovieTrimmer *t);

static Result _grcCmdNoIO(Service* srv, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _grcCmdInU64(Service* srv, u64 inval, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _grcCmdInU64Out32(Service* srv, u64 inval, u32 *out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

static Result _grcCmdNoInOut64(Service* srv, u64 *out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 out;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) {
            *out = resp->out;
        }
    }

    return rc;
}

static Result _grcGetEvent(Service* srv, Event* out_event, u64 cmd_id, bool autoclear) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(out_event, r.Handles[0], autoclear);
        }
    }

    return rc;
}

static Result _grcCmdInU64OutEvent(Service* srv, u64 inval, Event* out_event, u64 cmd_id, bool autoclear) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(out_event, r.Handles[0], autoclear);
        }
    }

    return rc;
}

static Result _grcGetSession(Service* srv, Service* srv_out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(srv_out, srv, &r, 0);
        }
    }

    return rc;
}

static Result _grcCreateGameMovieTrimmer(GrcGameMovieTrimmer *t, size_t size) {
    Result rc=0;
    Result retryrc = MAKERESULT(212, 4);

    memset(t, 0, sizeof(*t));

    rc = tmemCreate(&t->tmem, size, Perm_None);
    if (R_SUCCEEDED(rc)) {
        rc = appletCreateGameMovieTrimmer(&t->s, &t->tmem);

        while(rc == retryrc) {
            svcSleepThread(100000000);
            rc = appletCreateGameMovieTrimmer(&t->s, &t->tmem);
        }
    }

    if (R_FAILED(rc)) _grcGameMovieTrimmerClose(t);

    return rc;
}

// IGameMovieTrimmer

static void _grcGameMovieTrimmerClose(GrcGameMovieTrimmer *t) {
    serviceClose(&t->s);
    tmemClose(&t->tmem);
}

static Result _grcGameMovieTrimmerBeginTrim(GrcGameMovieTrimmer *t, const GrcGameMovieId *id, s32 start, s32 end) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 start;
        s32 end;
        GrcGameMovieId id;
    } *raw;

    raw = serviceIpcPrepareHeader(&t->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->start = start;
    raw->end = end;
    memcpy(&raw->id, id, sizeof(GrcGameMovieId));

    Result rc = serviceIpcDispatch(&t->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&t->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _grcGameMovieTrimmerEndTrim(GrcGameMovieTrimmer *t, GrcGameMovieId *id) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&t->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&t->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            GrcGameMovieId id;
        } *resp;

        serviceIpcParse(&t->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && id) memcpy(id, &resp->id, sizeof(GrcGameMovieId));
    }

    return rc;
}

static Result _grcGameMovieTrimmerGetNotTrimmingEvent(GrcGameMovieTrimmer *t, Event *out_event) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _grcGetEvent(&t->s, out_event, 10, false);
}

static Result _grcGameMovieTrimmerSetThumbnailRgba(GrcGameMovieTrimmer *t, const void* buffer, size_t size, s32 width, s32 height) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buffer, size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 width;
        s32 height;
    } *raw;

    raw = serviceIpcPrepareHeader(&t->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;
    raw->width = width;
    raw->height = height;

    Result rc = serviceIpcDispatch(&t->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&t->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result grcTrimGameMovie(GrcGameMovieId *dst_movieid, const GrcGameMovieId *src_movieid, size_t tmem_size, const void* thumbnail, s32 start, s32 end) {
    Result rc=0;
    GrcGameMovieTrimmer trimmer={0};
    Event trimevent={0};

    rc = _grcCreateGameMovieTrimmer(&trimmer, tmem_size);

    if (R_SUCCEEDED(rc)) {
        if (thumbnail) rc = _grcGameMovieTrimmerSetThumbnailRgba(&trimmer, thumbnail, 1280*720*4, 1280, 720);

        if (R_SUCCEEDED(rc)) rc = _grcGameMovieTrimmerGetNotTrimmingEvent(&trimmer, &trimevent);

        if (R_SUCCEEDED(rc)) rc = _grcGameMovieTrimmerBeginTrim(&trimmer, src_movieid, start, end);

        if (R_SUCCEEDED(rc)) rc = eventWait(&trimevent, U64_MAX);

        if (R_SUCCEEDED(rc)) rc = _grcGameMovieTrimmerEndTrim(&trimmer, dst_movieid);

        eventClose(&trimevent);
        _grcGameMovieTrimmerClose(&trimmer);
    }

    return rc;
}

// IMovieMaker

void grcCreateOffscreenRecordingParameter(GrcOffscreenRecordingParameter *param) {
    memset(param, 0, sizeof(*param));
    param->unk_x10 = 0x103;

    param->video_bitrate = 8000000;
    param->video_width = 1280;
    param->video_height = 720;
    param->video_framerate = 30;
    param->video_keyFrameInterval = 30;

    param->audio_bitrate = hosversionAtLeast(6,0,0) ? 128000 : 1536000;
    param->audio_samplerate = 48000;
    param->audio_channel_count = 2;
    param->audio_sample_format = PcmFormat_Int16;

    param->video_imageOrientation = AlbumImageOrientation_Unknown0;
}

Result grcCreateMovieMaker(GrcMovieMaker *m, size_t size) {
    Result rc=0;
    Result retryrc = MAKERESULT(212, 4);
    s32 binder_id=0;

    memset(m, 0, sizeof(*m));

    rc = tmemCreate(&m->tmem, size, Perm_None);
    if (R_SUCCEEDED(rc)) {
        rc = appletCreateMovieMaker(&m->a, &m->tmem);

        while(rc == retryrc) {
            svcSleepThread(100000000);
            rc = appletCreateMovieMaker(&m->a, &m->tmem);
        }
    }

    if (R_SUCCEEDED(rc)) rc = _grcGetSession(&m->a, &m->s, 0); // GetGrcMovieMaker

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _grcCmdInU64(&m->s, capsGetShimLibraryVersion(), 9); // SetAlbumShimLibraryVersion

    if (R_SUCCEEDED(rc)) rc = _grcCmdNoInOut64(&m->a, &m->layer_handle, 1); // GetLayerHandle


    if (R_SUCCEEDED(rc)) rc = _grcGetSession(&m->s, &m->video_proxy, 2); // CreateVideoProxy

    if (R_SUCCEEDED(rc)) rc = _grcCmdInU64Out32(&m->s, m->layer_handle, (u32*)&binder_id, 10); // OpenOffscreenLayer
    if (R_SUCCEEDED(rc)) m->layer_open = true;

    if (R_SUCCEEDED(rc)) rc = nwindowCreate(&m->win, &m->video_proxy, binder_id, false);
    if (R_SUCCEEDED(rc)) rc = nwindowSetDimensions(&m->win, 1280, 720);

    if (R_SUCCEEDED(rc)) rc = _grcCmdInU64OutEvent(&m->s, m->layer_handle, &m->recording_event, 50, false); // GetOffscreenLayerRecordingFinishReadyEvent
    if (R_SUCCEEDED(rc)) rc = _grcCmdInU64OutEvent(&m->s, m->layer_handle, &m->audio_event, 52, false); // GetOffscreenLayerAudioEncodeReadyEvent

    if (R_FAILED(rc)) grcMovieMakerClose(m);

    return rc;
}

void grcMovieMakerClose(GrcMovieMaker *m) {
    grcMovieMakerAbort(m);

    eventClose(&m->audio_event);
    eventClose(&m->recording_event);

    nwindowClose(&m->win);
    if (m->layer_open) {
        _grcCmdInU64(&m->s, m->layer_handle, 11); // CloseOffscreenLayer
        m->layer_open = false;
    }

    serviceClose(&m->video_proxy);
    serviceClose(&m->s);
    serviceClose(&m->a);
    tmemClose(&m->tmem);
}

Result grcMovieMakerStart(GrcMovieMaker *m, const GrcOffscreenRecordingParameter *param) {
    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 layer_handle;
        GrcOffscreenRecordingParameter param;
    } *raw;

    raw = serviceIpcPrepareHeader(&m->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;
    raw->layer_handle = m->layer_handle;
    memcpy(&raw->param, param, sizeof(GrcOffscreenRecordingParameter));

    Result rc = serviceIpcDispatch(&m->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&m->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    if (R_SUCCEEDED(rc)) m->started_flag = true;

    return rc;
}

Result grcMovieMakerAbort(GrcMovieMaker *m) {
    Result rc=0;

    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (!m->started_flag)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _grcCmdInU64(&m->s, m->layer_handle, 21); // AbortOffscreenRecording
    if (R_SUCCEEDED(rc)) m->started_flag = false;
    return rc;
}

static Result _grcMovieMakerCompleteOffscreenRecordingFinishEx0(GrcMovieMaker *m, s32 width, s32 height, const void* buffer0, size_t size0, const void* buffer1, size_t size1) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buffer0, size0, BufferType_Normal);
    ipcAddSendBuffer(&c, buffer1, size1, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 width;
        s32 height;
        u64 layer_handle;
    } *raw;

    raw = serviceIpcPrepareHeader(&m->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 25;
    raw->width = width;
    raw->height = height;
    raw->layer_handle = m->layer_handle;

    Result rc = serviceIpcDispatch(&m->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            CapsApplicationAlbumEntry entry;
        } *resp;

        serviceIpcParse(&m->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _grcMovieMakerCompleteOffscreenRecordingFinishEx1(GrcMovieMaker *m, s32 width, s32 height, const void* buffer0, size_t size0, const void* buffer1, size_t size1, CapsApplicationAlbumEntry *entry) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buffer0, size0, BufferType_Normal);
    ipcAddSendBuffer(&c, buffer1, size1, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 width;
        s32 height;
        u64 layer_handle;
    } *raw;

    raw = serviceIpcPrepareHeader(&m->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 26;
    raw->width = width;
    raw->height = height;
    raw->layer_handle = m->layer_handle;

    Result rc = serviceIpcDispatch(&m->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            CapsApplicationAlbumEntry entry;
        } *resp;

        serviceIpcParse(&m->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && entry) *entry = resp->entry;
    }

    return rc;
}

Result grcMovieMakerFinish(GrcMovieMaker *m, s32 width, s32 height, const void* buffer0, size_t size0, const void* buffer1, size_t size1, CapsApplicationAlbumEntry *entry) {
    Result rc=0;

    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(7,0,0) && entry)
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _grcCmdInU64(&m->s, m->layer_handle, 22); // RequestOffscreenRecordingFinishReady

    if (R_SUCCEEDED(rc)) rc = eventWait(&m->recording_event, U64_MAX);

    if (hosversionAtLeast(7,0,0))
        rc = _grcMovieMakerCompleteOffscreenRecordingFinishEx1(m, width, height, buffer0, size0, buffer1, size1, entry);
    else
        rc = _grcMovieMakerCompleteOffscreenRecordingFinishEx0(m, width, height, buffer0, size0, buffer1, size1);

    if (R_FAILED(rc)) grcMovieMakerAbort(m);
    return rc;
}

Result grcMovieMakerGetError(GrcMovieMaker *m) {
    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _grcCmdInU64(&m->s, m->layer_handle, 30); // GetOffscreenLayerError
}

static Result _grcMovieMakerEncodeOffscreenLayerAudioSample(GrcMovieMaker *m, const void* buffer, size_t size, u64 *out_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buffer, size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 layer_handle;
    } *raw;

    raw = serviceIpcPrepareHeader(&m->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 41;
    raw->layer_handle = m->layer_handle;

    Result rc = serviceIpcDispatch(&m->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 out_size;
        } *resp;

        serviceIpcParse(&m->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out_size) *out_size = resp->out_size;
    }

    return rc;
}

Result grcMovieMakerEncodeAudioSample(GrcMovieMaker *m, const void* buffer, size_t size) {
    Result rc=0;
    u64 out_size=0;
    u8 *bufptr = (u8*)buffer;

    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    for (u64 pos=0; size!=0; pos+=out_size, size-=out_size) {
        rc = eventWait(&m->audio_event, U64_MAX);
        if (R_FAILED(rc)) break;

        rc = _grcMovieMakerEncodeOffscreenLayerAudioSample(m, &bufptr[pos], size, &out_size);
        if (R_FAILED(rc)) break;
        if (out_size > size) out_size = size;
    }

    return rc;
}

// grc:d

static Service g_grcdSrv;
static u64 g_grcdRefCnt;

Result grcdInitialize(void) {
    atomicIncrement64(&g_grcdRefCnt);

    if (serviceIsActive(&g_grcdSrv))
        return 0;

    Result rc = smGetService(&g_grcdSrv, "grc:d");

    if (R_FAILED(rc)) grcdExit();

    return rc;
}

void grcdExit(void) {
    if (atomicDecrement64(&g_grcdRefCnt) == 0)
        serviceClose(&g_grcdSrv);
}

Service* grcdGetServiceSession(void) {
    return &g_grcdSrv;
}

Result grcdBegin(void) {
    return _grcCmdNoIO(&g_grcdSrv, 1);
}

Result grcdRead(GrcStream stream, void* buffer, size_t size, u32 *unk, u32 *data_size, u64 *timestamp) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, buffer, size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 stream;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_grcdSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->stream = stream;

    Result rc = serviceIpcDispatch(&g_grcdSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 unk;
            u32 data_size;
            u64 timestamp;
        } *resp;

        serviceIpcParse(&g_grcdSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (unk) *unk = resp->unk;
            if (data_size) *data_size = resp->data_size;
            if (timestamp) *timestamp = resp->timestamp;
        }
    }

    return rc;
}

