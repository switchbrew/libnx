#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/cache.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "services/usb.h"
#include "services/usbhs.h"
#include "services/sm.h"

static Service g_usbHsSrv;
static Event g_usbHsInterfaceStateChangeEvent = {0};

static Result _usbHsBindClientProcess(Handle prochandle);
static Result _usbHsGetEvent(Service* srv, Event* event_out, u64 cmd_id);

Result usbHsInitialize(void) {
    if (serviceIsActive(&g_usbHsSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    Result rc = 0;

    rc = smGetService(&g_usbHsSrv, "usb:hs");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_usbHsSrv);
    }

    if (R_SUCCEEDED(rc) && kernelAbove200())
        rc = _usbHsBindClientProcess(CUR_PROCESS_HANDLE);

    // GetInterfaceStateChangeEvent
    if (R_SUCCEEDED(rc))
        rc = _usbHsGetEvent(&g_usbHsSrv, &g_usbHsInterfaceStateChangeEvent, kernelAbove200() ? 6 : 5);

    if (R_FAILED(rc))
    {
        eventClose(&g_usbHsInterfaceStateChangeEvent);

        serviceClose(&g_usbHsSrv);
    }

    return rc;
}

void usbHsExit(void) {
    if (!serviceIsActive(&g_usbHsSrv))
        return;

    eventClose(&g_usbHsInterfaceStateChangeEvent);

    serviceClose(&g_usbHsSrv);
}

Event* usbHsGetInterfaceStateChangeEvent(void) {
    return &g_usbHsInterfaceStateChangeEvent;
}

static Result _usbHsBindClientProcess(Handle prochandle) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcSendHandleCopy(&c, prochandle);

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_usbHsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_usbHsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _usbHsGetEvent(Service* srv, Event* event_out, u64 cmd_id) {
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
            eventLoadRemote(event_out, r.Handles[0], false);
        }
    }

    return rc;
}

static Result _usbHsQueryInterfaces(u64 base_cmdid, const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        UsbHsInterfaceFilter filter;
    } *raw;

    ipcAddRecvBuffer(&c, interfaces, interfaces_maxsize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = kernelAbove200() ? base_cmdid+1 : base_cmdid;
    raw->filter = *filter;

    Result rc = serviceIpcDispatch(&g_usbHsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_entries;
        } *resp;

        serviceIpcParse(&g_usbHsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

Result usbHsQueryAllInterfaces(const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    return _usbHsQueryInterfaces(0, filter, interfaces, interfaces_maxsize, total_entries);
}

Result usbHsQueryAvailableInterfaces(const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    return _usbHsQueryInterfaces(1, filter, interfaces, interfaces_maxsize, total_entries);
}

Result usbHsQueryAcquiredInterfaces(UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvBuffer(&c, interfaces, interfaces_maxsize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = kernelAbove200() ? 3 : 2;

    Result rc = serviceIpcDispatch(&g_usbHsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_entries;
        } *resp;

        serviceIpcParse(&g_usbHsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

Result usbHsCreateInterfaceAvailableEvent(Event* event, bool autoclear, u8 index, const UsbHsInterfaceFilter* filter) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 index;
        u8 pad;
        UsbHsInterfaceFilter filter;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = kernelAbove200() ? 4 : 3;
    raw->index = index;
    raw->filter = *filter;

    Result rc = serviceIpcDispatch(&g_usbHsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_usbHsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event, r.Handles[0], autoclear);
        }
    }

    return rc;
}

Result usbHsDestroyInterfaceAvailableEvent(Event* event, u8 index) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 index;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = kernelAbove200() ? 5 : 4;
    raw->index = index;

    Result rc = serviceIpcDispatch(&g_usbHsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_usbHsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    eventClose(event);

    return rc;
}

Result usbHsAcquireUsbIf(UsbHsClientIfSession* s, UsbHsInterface *interface) {
    IpcCommand c;
    ipcInitialize(&c);

    memset(s, 0, sizeof(UsbHsClientIfSession));
    memcpy(&s->inf, interface, sizeof(UsbHsInterface));

    struct {
        u64 magic;
        u64 cmd_id;
        s32 ID;
    } *raw;

    ipcAddRecvBuffer(&c, &s->inf.inf, sizeof(UsbHsInterfaceInfo), BufferType_Normal);

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = kernelAbove200() ? 7 : 6;
    raw->ID = interface->inf.ID;

    Result rc = serviceIpcDispatch(&g_usbHsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_usbHsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&s->s, &g_usbHsSrv, &r, 0);
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = _usbHsGetEvent(&s->s, &s->event0, 0);
        if (kernelAbove200()) rc = _usbHsGetEvent(&s->s, &s->event0, 6);

        if (R_FAILED(rc)) {
            serviceClose(&s->s);
            eventClose(&s->event0);
            eventClose(&s->eventCtrlXfer);
        }
    }

    return rc;
}

void usbHsIfClose(UsbHsClientIfSession* s) {
    serviceClose(&s->s);
    eventClose(&s->event0);
    eventClose(&s->eventCtrlXfer);
    memset(s, 0, sizeof(UsbHsClientIfSession));
}

