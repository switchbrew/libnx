#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "kernel/tmem.h"
#include "services/sm.h"
#include "services/grc.h"
#include "services/applet.h"
#include "runtime/hosversion.h"

static void _grcGameMovieTrimmerClose(GrcGameMovieTrimmer *t);

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

