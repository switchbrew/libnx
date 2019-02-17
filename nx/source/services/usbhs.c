#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/cache.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
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

    if (R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0))
        rc = _usbHsBindClientProcess(CUR_PROCESS_HANDLE);

    // GetInterfaceStateChangeEvent
    if (R_SUCCEEDED(rc))
        rc = _usbHsGetEvent(&g_usbHsSrv, &g_usbHsInterfaceStateChangeEvent, hosversionAtLeast(2,0,0) ? 6 : 5);

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

static Result _usbHsCmdNoIO(Service* s, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
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
    raw->cmd_id = hosversionAtLeast(2,0,0) ? base_cmdid+1 : base_cmdid;
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
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 3 : 2;

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
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 4 : 3;
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
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 5 : 4;
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
    s->ID = interface->inf.ID;

    struct {
        u64 magic;
        u64 cmd_id;
        s32 ID;
    } *raw;

    if (hosversionBefore(3,0,0)) {
        ipcAddRecvBuffer(&c, &s->inf.inf, sizeof(UsbHsInterfaceInfo), BufferType_Normal);
    }
    else {
        //These buffer addresses are the inverse of what official sw does - needed to get the correct UsbHsInterface output for some reason.
        ipcAddRecvBuffer(&c, &s->inf.pathstr, sizeof(UsbHsInterface) - sizeof(UsbHsInterfaceInfo), BufferType_Normal);
        ipcAddRecvBuffer(&c, &s->inf.inf, sizeof(UsbHsInterfaceInfo), BufferType_Normal);
    }

    raw = serviceIpcPrepareHeader(&g_usbHsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 7 : 6;
    raw->ID = s->ID;

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
        if (hosversionAtLeast(2,0,0)) rc = _usbHsGetEvent(&s->s, &s->eventCtrlXfer, 6);

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

static Result _usbHsIfGetInf(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    if (inf==NULL) inf = &s->inf.inf;

    struct {
        u64 magic;
        u64 cmd_id;
        u8  id;
    } *raw;

    ipcAddRecvBuffer(&c, inf, sizeof(UsbHsInterfaceInfo), BufferType_Normal);

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = id;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result usbHsIfSetInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id) {
    return _usbHsIfGetInf(s, inf, id, 1);
}

Result usbHsIfGetAlternateInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id) {
    return _usbHsIfGetInf(s, inf, id, 3);
}

Result usbHsIfGetInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf) {
    IpcCommand c;
    ipcInitialize(&c);

    if (inf==NULL) inf = &s->inf.inf;

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvBuffer(&c, inf, sizeof(UsbHsInterfaceInfo), BufferType_Normal);

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result usbHsIfGetCurrentFrame(UsbHsClientIfSession* s, u32* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 4 : 5;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

static Result _usbHsIfSubmitControlRequest(UsbHsClientIfSession* s, u8 bRequest, u8 bmRequestType, u16 wValue, u16 wIndex, u16 wLength, void* buffer, u32 timeoutInMs, u32* transferredSize) {
    bool dir = (bmRequestType & USB_ENDPOINT_IN) != 0;
    size_t bufsize = (wLength + 0xFFF) & ~0xFFF;

    armDCacheFlush(buffer, wLength);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 bRequest;
        u8 bmRequestType;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
        u32 timeoutInMs;
    } PACKED *raw;

    if (dir) ipcAddRecvBuffer(&c, buffer, bufsize, BufferType_Normal);
    if (!dir) ipcAddSendBuffer(&c, buffer, bufsize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = dir ? 6 : 7;
    raw->bRequest = bRequest;
    raw->bmRequestType = bmRequestType;
    raw->wValue = wValue;
    raw->wIndex = wIndex;
    raw->wLength = wLength;
    raw->timeoutInMs = timeoutInMs;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 transferredSize;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && transferredSize) *transferredSize = resp->transferredSize;
    }

    if (dir) armDCacheFlush(buffer, wLength);

    return rc;
}

static Result _usbHsIfCtrlXferAsync(UsbHsClientIfSession* s, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, void* buffer) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 bmRequestType;
        u8 bRequest;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
        u64 buffer;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->bmRequestType = bmRequestType;
    raw->bRequest = bRequest;
    raw->wValue = wValue;
    raw->wIndex = wIndex;
    raw->wLength = wLength;
    raw->buffer = (u64)buffer;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _usbHsIfGetCtrlXferReport(UsbHsClientIfSession* s, UsbHsXferReport* report) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvBuffer(&c, report, sizeof(UsbHsXferReport), BufferType_Normal);

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result usbHsIfCtrlXfer(UsbHsClientIfSession* s, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, void* buffer, u32* transferredSize) {
    Result rc=0;
    UsbHsXferReport report;

    if (hosversionBefore(2,0,0)) return _usbHsIfSubmitControlRequest(s, bRequest, bmRequestType, wValue, wIndex, wLength, buffer, 0, transferredSize);

    rc = _usbHsIfCtrlXferAsync(s, bmRequestType, bRequest, wValue, wIndex, wLength, buffer);
    if (R_FAILED(rc)) return rc;

    rc = eventWait(&s->eventCtrlXfer, U64_MAX);
    if (R_FAILED(rc)) return rc;
    eventClear(&s->eventCtrlXfer);

    memset(&report, 0, sizeof(report));
    rc = _usbHsIfGetCtrlXferReport(s, &report);
    if (R_FAILED(rc)) return rc;

    *transferredSize = report.transferredSize;
    rc = report.res;

    return rc;
}

Result usbHsIfOpenUsbEp(UsbHsClientIfSession* s, UsbHsClientEpSession* ep, u16 maxUrbCount, u32 maxXferSize, struct usb_endpoint_descriptor *desc) {
    IpcCommand c;
    ipcInitialize(&c);

    memset(ep, 0, sizeof(UsbHsClientEpSession));

    struct {
        u64 magic;
        u64 cmd_id;
        u16 maxUrbCount;
        u32 epType;
        u32 epNumber;
        u32 epDirection;
        u32 maxXferSize;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 9 : 4;
    raw->maxUrbCount = maxUrbCount;
    raw->epType = (desc->bmAttributes & USB_TRANSFER_TYPE_MASK) + 1;
    raw->epNumber = desc->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
    raw->epDirection = (desc->bEndpointAddress & USB_ENDPOINT_IN) == 0 ? 0x1 : 0x2;
    raw->maxXferSize = maxXferSize;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            struct usb_endpoint_descriptor desc;
        } PACKED *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) memcpy(&ep->desc, &resp->desc, sizeof(struct usb_endpoint_descriptor));

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&ep->s, &s->s, &r, 0);
        }
    }

    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(2,0,0)) {
            rc = _usbHsCmdNoIO(&ep->s, 3);//Populate
            if (R_SUCCEEDED(rc)) rc = _usbHsGetEvent(&ep->s, &ep->eventXfer, 2);
        }

        if (R_FAILED(rc)) {
            serviceClose(&ep->s);
            eventClose(&ep->eventXfer);
        }
    }

    return rc;
}

Result usbHsIfResetDevice(UsbHsClientIfSession* s) {
    return _usbHsCmdNoIO(&s->s, 8);
}

void usbHsEpClose(UsbHsClientEpSession* s) {
    if (!serviceIsActive(&s->s)) return;

    _usbHsCmdNoIO(&s->s, hosversionAtLeast(2,0,0) ? 1 : 3);//Close

    serviceClose(&s->s);
    eventClose(&s->eventXfer);
    memset(s, 0, sizeof(UsbHsClientIfSession));
}

static Result _usbHsEpSubmitRequest(UsbHsClientEpSession* s, void* buffer, u32 size, u32 timeoutInMs, u32* transferredSize) {
    bool dir = (s->desc.bEndpointAddress & USB_ENDPOINT_IN) != 0;
    size_t bufsize = (size + 0xFFF) & ~0xFFF;

    armDCacheFlush(buffer, size);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 size;
        u32 timeoutInMs;//?
    } *raw;

    if (dir) ipcAddRecvBuffer(&c, buffer, bufsize, BufferType_Normal);
    if (!dir) ipcAddSendBuffer(&c, buffer, bufsize, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = dir ? 1 : 0;
    raw->size = size;
    raw->timeoutInMs = timeoutInMs;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 transferredSize;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && transferredSize) *transferredSize = resp->transferredSize;
    }

    if (dir) armDCacheFlush(buffer, size);

    return rc;
}

static Result _usbHsEpPostBufferAsync(UsbHsClientEpSession* s, void* buffer, u32 size, u64 unk, u32* xferId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 size;
        u64 buffer;
        u64 unk;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->size = size;
    raw->buffer = (u64)buffer;
    raw->unk = unk;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 xferId;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && xferId) *xferId = resp->xferId;
    }

    return rc;
}

static Result _usbHsEpGetXferReport(UsbHsClientEpSession* s, UsbHsXferReport* reports, u32 max_reports, u32* count) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 max_reports;
    } *raw;

    ipcAddRecvBuffer(&c, reports, sizeof(UsbHsXferReport) * max_reports, BufferType_Normal);

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->max_reports = max_reports;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 count;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && count) *count = resp->count;
    }

    return rc;
}

Result usbHsEpPostBuffer(UsbHsClientEpSession* s, void* buffer, u32 size, u32* transferredSize) {
    Result rc=0;
    u32 xferId=0;
    u32 count=0;
    UsbHsXferReport report;

    if (hosversionBefore(2,0,0)) return _usbHsEpSubmitRequest(s, buffer, size, 0, transferredSize);

    rc = _usbHsEpPostBufferAsync(s, buffer, size, 0, &xferId);
    if (R_FAILED(rc)) return rc;

    rc = eventWait(&s->eventXfer, U64_MAX);
    if (R_FAILED(rc)) return rc;
    eventClear(&s->eventXfer);

    memset(&report, 0, sizeof(report));
    rc = _usbHsEpGetXferReport(s, &report, 1, &count);
    if (R_FAILED(rc)) return rc;

    if (count<1) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    *transferredSize = report.transferredSize;
    rc = report.res;

    return rc;
}

