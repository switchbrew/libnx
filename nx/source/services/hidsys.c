#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/shmem.h"
#include "kernel/rwlock.h"
#include "kernel/event.h"
#include "services/applet.h"
#include "services/hidsys.h"
#include "services/sm.h"

static Service g_hidsysSrv;
static u64 g_hidsysRefCnt;

static Event g_hidsysHomeEvent = {0};
static Event g_hidsysCaptureEvent = {0};
static Event g_hidSysSleepEvent = {0};

static u64 g_hidsysAppletResourceUserId = 0;

Result hidsysInitialize(void) {
    atomicIncrement64(&g_hidsysRefCnt);
    
    if (serviceIsActive(&g_hidsysSrv)) 
        return 0;
    
    Result rc = smGetService(&g_hidsysSrv, "hid:sys");
    if (R_FAILED(rc))
        return rc;
    
    rc = appletGetAppletResourceUserId(&g_hidsysAppletResourceUserId);
    if (R_FAILED(rc))
        g_hidsysAppletResourceUserId = 0;

    return 0; 
}

void hidsysExit(void) {
    if (atomicDecrement64(&g_hidsysRefCnt) == 0) {
        eventClose(&g_hidsysHomeEvent);
        eventClose(&g_hidsysCaptureEvent);
        eventClose(&g_hidSysSleepEvent);
        
        serviceClose(&g_hidsysSrv);
    }
}

Result hidsysEnableAppletToGetInput(bool enable) {  
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmdid;
        u8 permitInput;
        u64 appletResourceUserId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmdid = 503;
    raw->permitInput = enable != 0;
    raw->appletResourceUserId = g_hidsysAppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidsysCmdWithResIdAndPid(u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->AppletResourceUserId = g_hidsysAppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidsysGetHandle(Handle* handle_out, u64 cmd_id) {    
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);
    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->AppletResourceUserId = g_hidsysAppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

static Result _hidsysGetEvent(Event* event_out, u64 cmd_id, bool autoclear) {
    Handle tmp_handle=0;
    Result rc = 0;

    rc = _hidsysGetHandle(&tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(event_out, tmp_handle, autoclear);
    return rc;
}

Result hidsysAcquireHomeButtonEventHandle(Event* event_out) {
    if (eventActive(&g_hidsysHomeEvent))
    {
        *event_out = g_hidsysHomeEvent;
        return 0;
    }
    Result rc = _hidsysGetEvent(&g_hidsysHomeEvent, 101, false);
    if (R_SUCCEEDED(rc))
        *event_out = g_hidsysHomeEvent;
    return rc;
}

//These functions don't seem to work in the overlaydisp applet context
Result hidsysAcquireCaptureButtonEventHandle(Event* event_out) {
    if (eventActive(&g_hidsysCaptureEvent))
    {
        *event_out = g_hidsysCaptureEvent;
        return 0;
    }
    Result rc = _hidsysGetEvent(&g_hidsysCaptureEvent, 141, false);
    if (R_SUCCEEDED(rc))
        *event_out = g_hidsysCaptureEvent;
    return rc;
}

Result hidsysAcquireSleepButtonEventHandle(Event* event_out) {
    if (eventActive(&g_hidSysSleepEvent))
    {
        *event_out = g_hidSysSleepEvent;
        return 0;
    }
    Result rc = _hidsysGetEvent(&g_hidSysSleepEvent, 121, false);
    if (R_SUCCEEDED(rc))
        *event_out = g_hidSysSleepEvent;
    return rc;
}

Result hidsysActivateHomeButton(void) {
    return _hidsysCmdWithResIdAndPid(111);
}

Result hidsysActivateSleepButton(void) {
    return _hidsysCmdWithResIdAndPid(131);
}

Result hidsysActivateCaptureButton(void) {
    return _hidsysCmdWithResIdAndPid(151);
}