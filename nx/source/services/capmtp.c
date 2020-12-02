#include <string.h>

#include "kernel/tmem.h"
#include "runtime/hosversion.h"
#include "services/capmtp.h"
#include "services/sm.h"

static Service g_capmtpRoot, g_capmtp;
static TransferMemory g_tmem;
static Event g_event1, g_event2;

static Result _capmtpOpenSession(Service *srv);
static Result _capmtpOpen(u32 app_count, u32 max_img, u32 max_vid, const char *other_name);
static Result _capmtpClose(void);
static Result _capmtpNoInEventOut(u32 id, Event* event, bool autoclear);

Result capmtpInitialize(void* mem, size_t size, u32 app_count, u32 max_img, u32 max_vid, const char *other_name) {
    Result rc=0;

    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = tmemCreateFromMemory(&g_tmem, mem, size, Perm_None);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capmtpRoot, "capmtp");

    if (R_SUCCEEDED(rc)) rc = _capmtpOpenSession(&g_capmtp);
    if (R_SUCCEEDED(rc)) rc = _capmtpOpen(app_count, max_img, max_vid, other_name);
    if (R_SUCCEEDED(rc)) rc = _capmtpNoInEventOut(5, &g_event1, false);
    if (R_SUCCEEDED(rc)) rc = _capmtpNoInEventOut(7, &g_event2, false);

    if (R_FAILED(rc)) capmtpExit();

    return rc;
}

void capmtpExit(void) {
    _capmtpClose();
    eventClose(&g_event2);
    eventClose(&g_event1);
    serviceClose(&g_capmtpRoot);
    serviceClose(&g_capmtp);
    tmemClose(&g_tmem);
}

static Result _capmtpNoIO(u32 id) {
    return serviceDispatch(&g_capmtp, id);
}

static Result _capmtpNoInEventOut(u32 id, Event* event, bool autoclear) {
    Result rc=0;
    Handle handle=0;

    rc = serviceDispatch(&g_capmtp, id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &handle,
    );

    eventLoadRemote(event, handle, autoclear);

    return rc;
}

static Result _capmtpNoInBoolOut(u32 id, bool* out) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&g_capmtp, id, tmp);
    if (R_SUCCEEDED(rc)) *out = tmp & 1;
    return rc;
}

static Result _capmtpOpenSession(Service *srv) {
    return serviceDispatch(&g_capmtpRoot, 0,
        .out_num_objects = 1,
        .out_objects = srv,
    );
}

static Result _capmtpOpen(u32 app_count, u32 max_img, u32 max_vid, const char *other_name) {
    const struct {
        u32 c[4];
    } in = { { app_count, max_img, max_vid, g_tmem.size } };

    return serviceDispatchIn(&g_capmtp, 0, in,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { other_name, strlen(other_name) + 1 } },
        .in_num_handles = 1,
        .in_handles = { g_tmem.handle },
    );
}

static Result _capmtpClose() {
    return _capmtpNoIO(1);
}

Result capmtpStartCommandHandler(void) {
    return _capmtpNoIO(2);
}

Result capmtpStopCommandHandler(void) {
    return _capmtpNoIO(3);
}

Event *capmtpGetEvent1(void) {
    return &g_event1;
}

Event *capmtpGetEvent2(void) {
    return &g_event2;
}

Result capmtpIsRunning(bool *out) {
    return _capmtpNoInBoolOut(4, out);
}

Result capmtpUnkBool(bool *out) {
    return _capmtpNoInBoolOut(6, out);
}

Result capmtpGetResult(void) {
    return _capmtpNoIO(8);
}
