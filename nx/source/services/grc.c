#include "service_guard.h"
#include <string.h>
#include "services/grc.h"
#include "services/applet.h"
#include "audio/audio.h"
#include "runtime/hosversion.h"

static Service g_grcdSrv;

static void _grcGameMovieTrimmerClose(GrcGameMovieTrimmer *t);

static Result _grcCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _grcCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _grcCmdInU64OutU32(Service* srv, u64 inval, u32 *out, u32 cmd_id) {
    return serviceDispatchInOut(srv, cmd_id, inval, *out);
}

static Result _grcCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _grcCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _grcCmdInU64OutEvent(Service* srv, u64 inval, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatchIn(srv, cmd_id, inval,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _grcCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
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

    const struct {
        s32 start;
        s32 end;
        GrcGameMovieId id;
    } in = { start, end, *id };

    return serviceDispatchIn(&t->s, 1, in);
}

static Result _grcGameMovieTrimmerEndTrim(GrcGameMovieTrimmer *t, GrcGameMovieId *id) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchOut(&t->s, 2, *id);
}

static Result _grcGameMovieTrimmerGetNotTrimmingEvent(GrcGameMovieTrimmer *t, Event *out_event) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _grcCmdGetEvent(&t->s, out_event, false, 10);
}

static Result _grcGameMovieTrimmerSetThumbnailRgba(GrcGameMovieTrimmer *t, const void* buffer, size_t size, s32 width, s32 height) {
    if (!serviceIsActive(&t->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        s32 width;
        s32 height;
    } in = { width, height };

    return serviceDispatchIn(&t->s, 20, in,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
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

    if (R_SUCCEEDED(rc)) rc = _grcCmdGetSession(&m->a, &m->s, 0); // GetGrcMovieMaker

    if (R_SUCCEEDED(rc) && hosversionAtLeast(7,0,0)) rc = _grcCmdInU64NoOut(&m->s, capsGetShimLibraryVersion(), 9); // SetAlbumShimLibraryVersion

    if (R_SUCCEEDED(rc)) rc = _grcCmdNoInOutU64(&m->a, &m->layer_handle, 1); // GetLayerHandle

    if (R_SUCCEEDED(rc)) rc = _grcCmdGetSession(&m->s, &m->video_proxy, 2); // CreateVideoProxy

    if (R_SUCCEEDED(rc)) rc = _grcCmdInU64OutU32(&m->s, m->layer_handle, (u32*)&binder_id, 10); // OpenOffscreenLayer
    if (R_SUCCEEDED(rc)) m->layer_open = true;

    if (R_SUCCEEDED(rc)) rc = nwindowCreate(&m->win, &m->video_proxy, binder_id, false);
    if (R_SUCCEEDED(rc)) rc = nwindowSetDimensions(&m->win, 1280, 720);

    if (R_SUCCEEDED(rc)) rc = _grcCmdInU64OutEvent(&m->s, m->layer_handle, &m->recording_event, false, 50); // GetOffscreenLayerRecordingFinishReadyEvent
    if (R_SUCCEEDED(rc)) rc = _grcCmdInU64OutEvent(&m->s, m->layer_handle, &m->audio_event, false, 52); // GetOffscreenLayerAudioEncodeReadyEvent

    if (R_FAILED(rc)) grcMovieMakerClose(m);

    return rc;
}

void grcMovieMakerClose(GrcMovieMaker *m) {
    grcMovieMakerAbort(m);

    eventClose(&m->audio_event);
    eventClose(&m->recording_event);

    nwindowClose(&m->win);
    if (m->layer_open) {
        _grcCmdInU64NoOut(&m->s, m->layer_handle, 11); // CloseOffscreenLayer
        m->layer_open = false;
    }

    serviceClose(&m->video_proxy);
    serviceClose(&m->s);
    serviceClose(&m->a);
    tmemClose(&m->tmem);
}

Result grcMovieMakerAbort(GrcMovieMaker *m) {
    Result rc=0;

    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (!m->started_flag)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _grcCmdInU64NoOut(&m->s, m->layer_handle, 21); // AbortOffscreenRecording
    if (R_SUCCEEDED(rc)) m->started_flag = false;
    return rc;
}

Result grcMovieMakerStart(GrcMovieMaker *m, const GrcOffscreenRecordingParameter *param) {
    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u64 layer_handle;
        GrcOffscreenRecordingParameter param;
    } in = { m->layer_handle, *param };

    Result rc = serviceDispatchIn(&m->s, 24, in);
    if (R_SUCCEEDED(rc)) m->started_flag = true;
    return rc;
}

static Result _grcMovieMakerCompleteOffscreenRecordingFinishEx0(GrcMovieMaker *m, s32 width, s32 height, const void* userdata, size_t userdata_size, const void* thumbnail, size_t thumbnail_size) {
    const struct {
        s32 width;
        s32 height;
        u64 layer_handle;
    } in = { width, height, m->layer_handle };

    return serviceDispatchIn(&m->s, 25, in,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { userdata, userdata_size },
            { thumbnail, thumbnail_size },
        },
    );
}

static Result _grcMovieMakerCompleteOffscreenRecordingFinishEx1(GrcMovieMaker *m, s32 width, s32 height, const void* userdata, size_t userdata_size, const void* thumbnail, size_t thumbnail_size, CapsApplicationAlbumEntry *entry) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 width;
        s32 height;
        u64 layer_handle;
    } in = { width, height, m->layer_handle };

    return serviceDispatchInOut(&m->s, 26, in, *entry,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { userdata, userdata_size },
            { thumbnail, thumbnail_size },
        },
    );
}

Result grcMovieMakerFinish(GrcMovieMaker *m, s32 width, s32 height, const void* userdata, size_t userdata_size, const void* thumbnail, size_t thumbnail_size, CapsApplicationAlbumEntry *entry) {
    Result rc=0;

    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(7,0,0) && entry)
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = _grcCmdInU64NoOut(&m->s, m->layer_handle, 22); // RequestOffscreenRecordingFinishReady

    if (R_SUCCEEDED(rc)) rc = eventWait(&m->recording_event, U64_MAX);

    if (hosversionAtLeast(7,0,0))
        rc = _grcMovieMakerCompleteOffscreenRecordingFinishEx1(m, width, height, userdata, userdata_size, thumbnail, thumbnail_size, entry);
    else
        rc = _grcMovieMakerCompleteOffscreenRecordingFinishEx0(m, width, height, userdata, userdata_size, thumbnail, thumbnail_size);

    if (R_FAILED(rc)) grcMovieMakerAbort(m);
    return rc;
}

Result grcMovieMakerGetError(GrcMovieMaker *m) {
    if (!serviceIsActive(&m->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _grcCmdInU64NoOut(&m->s, m->layer_handle, 30); // GetOffscreenLayerError
}

static Result _grcMovieMakerEncodeOffscreenLayerAudioSample(GrcMovieMaker *m, const void* buffer, size_t size, u64 *out_size) {
    return serviceDispatchInOut(&m->s, 41, m->layer_handle, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
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

NX_GENERATE_SERVICE_GUARD(grcd);

Result _grcdInitialize(void) {
    return smGetService(&g_grcdSrv, "grc:d");
}

void _grcdCleanup(void) {
    serviceClose(&g_grcdSrv);
}

Service* grcdGetServiceSession(void) {
    return &g_grcdSrv;
}

Result grcdBegin(void) {
    return _grcCmdNoIO(&g_grcdSrv, 1);
}

Result grcdTransfer(GrcStream stream, void* buffer, size_t size, u32 *num_frames, u32 *data_size, u64 *start_timestamp) {
    struct {
        u32 num_frames;
        u32 data_size;
        u64 start_timestamp;
    } out;

    u32 tmp=stream;
    Result rc = serviceDispatchInOut(&g_grcdSrv, 2, tmp, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (num_frames) *num_frames = out.num_frames;
        if (data_size) *data_size = out.data_size;
        if (start_timestamp) *start_timestamp = out.start_timestamp;
    }
    return rc;
}

