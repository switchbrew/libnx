#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "services/applet.h"
#include "services/hidsys.h"
#include "services/hid.h"
#include "services/sm.h"
#include "runtime/hosversion.h"

static Service g_hidsysSrv;
static u64 g_hidsysRefCnt;

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
        serviceClose(&g_hidsysSrv);
    }
}

Service* hidsysGetServiceSession(void) {
    return &g_hidsysSrv;
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
    return _hidsysGetEvent(event_out, 101, false);
}

//These functions don't seem to work in the overlaydisp applet context
Result hidsysAcquireCaptureButtonEventHandle(Event* event_out) {
    return _hidsysGetEvent(event_out, 141, false);
}

Result hidsysAcquireSleepButtonEventHandle(Event* event_out) {
    return _hidsysGetEvent(event_out, 121, false);
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

Result hidsysGetUniquePadsFromNpad(HidControllerID id, u64 *UniquePadIds, size_t count, size_t *total_entries) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 id;
    } PACKED *raw;

    ipcAddRecvStatic(&c, UniquePadIds, sizeof(u64)*count, 0);

    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 321;
    raw->id = hidControllerIDToOfficial(id);

    rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

Result hidsysGetUniquePadIds(u64 *UniquePadIds, size_t count, size_t *total_entries) {
    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvStatic(&c, UniquePadIds, sizeof(u64)*count, 0);

    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 703;

    rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 total_entries;
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

Result hidsysSetNotificationLedPattern(const HidsysNotificationLedPattern *pattern, u64 UniquePadId) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        HidsysNotificationLedPattern pattern;
        u64 UniquePadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 830;
    memcpy(&raw->pattern, pattern, sizeof(*pattern));
    raw->UniquePadId = UniquePadId;

    rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;
    }

    return rc;
}

Result hidsysGetUniquePadSerialNumber(u64 UniquePadId, char *serial) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    if (serial) memset(serial, 0, 0x19);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 UniquePadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hidsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 809;
    raw->UniquePadId = UniquePadId;

    Result rc = serviceIpcDispatch(&g_hidsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            char serial[0x18];
        } *resp;

        serviceIpcParse(&g_hidsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && serial)
            memcpy(serial, resp->serial, 0x18);
    }

    return rc;
}
