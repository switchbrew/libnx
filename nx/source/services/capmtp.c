#include "service_guard.h"
#include "kernel/tmem.h"
#include "runtime/hosversion.h"
#include "runtime/util/utf.h"
#include "services/capmtp.h"
#include "services/sm.h"

static Service g_capmtpRoot;
static Service g_capmtp;
static TransferMemory g_tmem;
static Event g_connectEvent, g_scanErrorEvent;

static Result _capmtpOpenSession(Service *srv);
static Result _capmtpOpen(u32 max_folders, u32 max_img, u32 max_vid, const char *other_name);
static Result _capmtpClose(void);
static Result _capmtpNoInEventOut(u32 id, Event* event, bool autoclear);

NX_GENERATE_SERVICE_GUARD_PARAMS(capmtp, (void* mem, size_t size, u32 max_folders, u32 max_img, u32 max_vid, const char *other_name), (mem, size, max_folders, max_img, max_vid, other_name));

Result _capmtpInitialize(void* mem, size_t size, u32 max_folders, u32 max_img, u32 max_vid, const char *other_name) {
    Result rc=0;

    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = tmemCreateFromMemory(&g_tmem, mem, size, Perm_None);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capmtpRoot, "capmtp");
    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_capmtpRoot);

    if (R_SUCCEEDED(rc)) rc = _capmtpOpenSession(&g_capmtp);

    if (R_SUCCEEDED(rc)) rc = _capmtpOpen(max_folders, max_img, max_vid, other_name);
    if (R_SUCCEEDED(rc)) rc = _capmtpNoInEventOut(5, &g_connectEvent, false);
    if (R_SUCCEEDED(rc)) rc = _capmtpNoInEventOut(7, &g_scanErrorEvent, false);

    return rc;
}

void _capmtpCleanup(void) {
    eventClose(&g_scanErrorEvent);
    eventClose(&g_connectEvent);
    _capmtpClose();
    serviceClose(&g_capmtp);
    serviceClose(&g_capmtpRoot);
    tmemClose(&g_tmem);
}

Service* capmtpGetRootServiceSession(void) {
    return &g_capmtpRoot;
}

Service* capmtpGetServiceSession(void) {
    return &g_capmtp;
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

static Result _capmtpNoInBoolOut(u32 id) {
    u8 tmp=0;
    return R_SUCCEEDED(serviceDispatchOut(&g_capmtp, id, tmp)) && tmp & 1;
}

static Result _capmtpOpenSession(Service *srv) {
    return serviceDispatch(&g_capmtpRoot, 0,
        .out_num_objects = 1,
        .out_objects = srv,
    );
}

static Result _capmtpOpen(u32 max_folders, u32 max_img, u32 max_vid, const char *other_name) {
    u16 buffer[0x100];
    size_t len = utf8_to_utf16(buffer, (const u8*)other_name, sizeof(buffer)/sizeof(u16) - 1);
    buffer[len] = 0;
    const struct {
        u32 tmem_size;
        u32 folder_count;
        u32 max_images;
        u32 max_videos;
    } in = { (u32)g_tmem.size, max_folders, max_img, max_vid };

    return serviceDispatchIn(&g_capmtp, 0, in,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { buffer, 2*(len+1) } },
        .in_num_handles = 1,
        .in_handles = { g_tmem.handle },
    );
}

static Result _capmtpClose(void) {
    return _capmtpNoIO(1);
}

Result capmtpStartCommandHandler(void) {
    return _capmtpNoIO(2);
}

Result capmtpStopCommandHandler(void) {
    return _capmtpNoIO(3);
}

bool capmtpIsRunning(void) {
    return _capmtpNoInBoolOut(4);
}

Event *capmtpGetConnectionEvent(void) {
    return &g_connectEvent;
}

bool capmtpIsConnected(void) {
    return _capmtpNoInBoolOut(6);
}

Event *capmtpGetScanErrorEvent(void) {
    return &g_scanErrorEvent;
}

Result capmtpGetScanError(void) {
    return _capmtpNoIO(8);
}
