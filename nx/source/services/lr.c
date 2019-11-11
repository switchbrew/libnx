#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "services/lr.h"
#include "services/fs.h"
#include "runtime/hosversion.h"

static Service g_lrSrv;

NX_GENERATE_SERVICE_GUARD(lr);

Result _lrInitialize(void) {
    return smGetService(&g_lrSrv, "lr");
}

void _lrCleanup(void) {
    serviceClose(&g_lrSrv);
}

Service* lrGetServiceSession(void) {
    return &g_lrSrv;
}

Result lrOpenLocationResolver(NcmStorageId storage, LrLocationResolver* out) {
    const u8 in = (u8)storage;
    return serviceDispatchIn(&g_lrSrv, 0, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result lrOpenRegisteredLocationResolver(LrRegisteredLocationResolver* out) {
    return serviceDispatch(&g_lrSrv, 1,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

/*
    All the LocationResolver/RegisteredLocationResolver "Resolve" commands have a common API.
    This is a helper function to perform the work for those funcs, given a command ID.
*/
static Result _lrResolvePath(Service* s, u64 tid, char *out, u32 cmd_id) {
    char out_path[FS_MAX_PATH] = {0};
    Result rc = serviceDispatchIn(s, cmd_id, tid,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers = { { out_path, FS_MAX_PATH } },
    );

    if (R_SUCCEEDED(rc)) {
        strncpy(out, out_path, FS_MAX_PATH);
        out[FS_MAX_PATH-1] = 0;
    }

    return rc;
}

/*
    All the LocationResolver/RegisteredLocationResolver "Redirect" commands have a common API.
    This is a helper function to perform the work for those funcs, given a command ID.
*/
static Result _lrRedirectPath(Service* s, u64 tid, const char *path, u32 cmd_id) {
    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);
    send_path[FS_MAX_PATH-1] = 0;

    return serviceDispatchIn(s, cmd_id, tid,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer },
        .buffers = { { send_path, FS_MAX_PATH } },
    );
}

/*
    In 9.0.0, "RedirectApplication" commands began taking in a second tid argument.
    This is a helper function to perform the work for those funcs, given a command ID.
*/
static Result _lrRedirectApplicationPath(Service* s, u64 tid, u64 tid2, const char *path, u32 cmd_id) {
    // On < 9.0.0, call the original redirection helper.
    if (hosversionBefore(9,0,0)) {
        return _lrRedirectPath(s, tid, path, cmd_id);
    }

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);
    send_path[FS_MAX_PATH-1] = 0;

    const struct {
        u64 tid;
        u64 tid2;
    } in = { tid, tid2 };

    return serviceDispatchIn(s, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer },
        .buffers = { { send_path, FS_MAX_PATH } },
    );
}

Result lrLrResolveProgramPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, tid, out, 0);
}

Result lrLrRedirectProgramPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return _lrRedirectPath(&lr->s, tid, path, 1);
}

Result lrLrResolveApplicationControlPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, tid, out, 2);
}

Result lrLrResolveApplicationHtmlDocumentPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, tid, out, 3);
}

Result lrLrResolveDataPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, tid, out, 4);
}

Result lrLrRedirectApplicationControlPath(LrLocationResolver* lr, u64 tid, u64 tid2, const char *path) {
    return _lrRedirectApplicationPath(&lr->s, tid, tid2, path, 5);
}

Result lrLrRedirectApplicationHtmlDocumentPath(LrLocationResolver* lr, u64 tid, u64 tid2, const char *path) {
    return _lrRedirectApplicationPath(&lr->s, tid, tid2, path, 6);
}

Result lrLrResolveApplicationLegalInformationPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, tid, out, 7);
}

Result lrLrRedirectApplicationLegalInformationPath(LrLocationResolver* lr, u64 tid, u64 tid2, const char *path) {
    return _lrRedirectApplicationPath(&lr->s, tid, tid2, path, 8);
}

Result lrLrRefresh(LrLocationResolver* lr) {
    return serviceDispatch(&lr->s, 9);
}

Result lrRegLrResolveProgramPath(LrRegisteredLocationResolver* reg, u64 tid, char *out) {
    return _lrResolvePath(&reg->s, tid, out, 0);
}
