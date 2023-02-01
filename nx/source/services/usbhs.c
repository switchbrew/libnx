#include "service_guard.h"
#include <string.h>
#include "arm/cache.h"
#include "runtime/hosversion.h"
#include "services/usbhs.h"

static Service g_usbHsSrv;
static Event g_usbHsInterfaceStateChangeEvent = {0};

static Result _usbHsBindClientProcess(Handle prochandle);
static Result _usbHsGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(usbHs);

Result _usbHsInitialize(void) {
    Result rc = 0;

    rc = smGetService(&g_usbHsSrv, "usb:hs");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_usbHsSrv);
    }

    if (R_SUCCEEDED(rc) && hosversionAtLeast(2,0,0))
        rc = _usbHsBindClientProcess(CUR_PROCESS_HANDLE);

    // GetInterfaceStateChangeEvent
    if (R_SUCCEEDED(rc))
        rc = _usbHsGetEvent(&g_usbHsSrv, &g_usbHsInterfaceStateChangeEvent, false, hosversionAtLeast(2,0,0) ? 6 : 5);

    return rc;
}

void _usbHsCleanup(void) {
    eventClose(&g_usbHsInterfaceStateChangeEvent);
    serviceClose(&g_usbHsSrv);
}

Service* usbHsGetServiceSession(void) {
    return &g_usbHsSrv;
}

Event* usbHsGetInterfaceStateChangeEvent(void) {
    return &g_usbHsInterfaceStateChangeEvent;
}

static Result _usbHsGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _usbHsGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _usbHsGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _usbHsCmdNoIO(Service* srv, u64 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _usbHsCmdInU8NoOut(Service* srv, u8 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _usbDsCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _usbHsCmdRecvBufNoOut(Service* srv, void* buffer, size_t size, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static Result _usbHsBindClientProcess(Handle prochandle) {
    serviceAssumeDomain(&g_usbHsSrv);
    return serviceDispatch(&g_usbHsSrv, 0,
        .in_num_handles = 1,
        .in_handles = { prochandle },
    );
}

// The INPUT/OUTPUT endpoint descriptors were swapped with [8.0.0+], however the sysmodule code which writes this output struct was basically unchanged.
static void _usbHsConvertInterfaceInfoToV8(UsbHsInterfaceInfo *info) {
    UsbHsInterfaceInfo tmp;
    if (hosversionAtLeast(8,0,0) || info==NULL) return;

    memcpy(&tmp, info, sizeof(UsbHsInterfaceInfo));

    memcpy(info->output_endpoint_descs, tmp.input_endpoint_descs, sizeof(tmp.input_endpoint_descs));
    memcpy(info->input_endpoint_descs, tmp.output_endpoint_descs, sizeof(tmp.output_endpoint_descs));
    memcpy(info->output_ss_endpoint_companion_descs, tmp.input_ss_endpoint_companion_descs, sizeof(tmp.input_ss_endpoint_companion_descs));
    memcpy(info->input_ss_endpoint_companion_descs, tmp.output_ss_endpoint_companion_descs, sizeof(tmp.output_ss_endpoint_companion_descs));
}

static void _usbHsConvertInterfacesToV8(UsbHsInterface* interfaces, s32 total_entries) {
    for (s32 i=0; i<total_entries; i++) {
        _usbHsConvertInterfaceInfoToV8(&interfaces[i].inf);
    }
}

static Result _usbHsQueryInterfaces(u32 base_cmdid, const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    serviceAssumeDomain(&g_usbHsSrv);
    s32 tmp=0;
    Result rc = serviceDispatchInOut(&g_usbHsSrv, hosversionAtLeast(2,0,0) ? base_cmdid+1 : base_cmdid, *filter, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { interfaces, interfaces_maxsize } },
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = tmp;
    if (R_SUCCEEDED(rc)) _usbHsConvertInterfacesToV8(interfaces, tmp);
    return rc;
}

Result usbHsQueryAllInterfaces(const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    return _usbHsQueryInterfaces(0, filter, interfaces, interfaces_maxsize, total_entries);
}

Result usbHsQueryAvailableInterfaces(const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    return _usbHsQueryInterfaces(1, filter, interfaces, interfaces_maxsize, total_entries);
}

Result usbHsQueryAcquiredInterfaces(UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries) {
    serviceAssumeDomain(&g_usbHsSrv);
    s32 tmp=0;
    Result rc = serviceDispatchOut(&g_usbHsSrv, hosversionAtLeast(2,0,0) ? 3 : 2, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { interfaces, interfaces_maxsize } },
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = tmp;
    if (R_SUCCEEDED(rc)) _usbHsConvertInterfacesToV8(interfaces, tmp);
    return rc;
}

Result usbHsCreateInterfaceAvailableEvent(Event* out_event, bool autoclear, u8 index, const UsbHsInterfaceFilter* filter) {
    const struct {
        u8 index;
        u8 pad;
        UsbHsInterfaceFilter filter;
    } in = { index, 0, *filter };

    Handle tmp_handle = INVALID_HANDLE;
    serviceAssumeDomain(&g_usbHsSrv);
    Result rc = serviceDispatchIn(&g_usbHsSrv, hosversionAtLeast(2,0,0) ? 4 : 3, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

Result usbHsDestroyInterfaceAvailableEvent(Event* event, u8 index) {
    Result rc = _usbHsCmdInU8NoOut(&g_usbHsSrv, index, hosversionAtLeast(2,0,0) ? 5 : 4);
    eventClose(event);
    return rc;
}

static Result _usbHsAcquireUsbIfOld(UsbHsClientIfSession* s, UsbHsInterface *interface) { // pre-2.0.0
    serviceAssumeDomain(&g_usbHsSrv);
    return serviceDispatchIn(&g_usbHsSrv, 6, s->ID,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { &s->inf.inf, sizeof(UsbHsInterfaceInfo) } },
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
}

static Result _usbHsAcquireUsbIf(UsbHsClientIfSession* s, UsbHsInterface *interface) { // [2.0.0+]
    // These buffer addresses are the inverse of what official sw does - needed to get the correct UsbHsInterface output for some reason.
    serviceAssumeDomain(&g_usbHsSrv);
    return serviceDispatchIn(&g_usbHsSrv, 7, s->ID,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { &s->inf.pathstr, sizeof(UsbHsInterface) - sizeof(UsbHsInterfaceInfo) },
            { &s->inf.inf, sizeof(UsbHsInterfaceInfo) },
        },
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
}

Result usbHsAcquireUsbIf(UsbHsClientIfSession* s, UsbHsInterface *interface) {
    Result rc=0;

    memset(s, 0, sizeof(UsbHsClientIfSession));
    memcpy(&s->inf, interface, sizeof(UsbHsInterface));
    s->ID = interface->inf.ID;

    if (hosversionAtLeast(2,0,0))
        rc = _usbHsAcquireUsbIf(s, interface);
    else
        rc = _usbHsAcquireUsbIfOld(s, interface);

    if (R_SUCCEEDED(rc)) {
        _usbHsConvertInterfaceInfoToV8(&interface->inf);
        rc = _usbHsGetEvent(&s->s, &s->event0, false, 0);
        if (hosversionAtLeast(2,0,0)) rc = _usbHsGetEvent(&s->s, &s->eventCtrlXfer, false, 6);

        if (R_FAILED(rc)) usbHsIfClose(s);
    }

    return rc;
}

void usbHsIfClose(UsbHsClientIfSession* s) {
    serviceAssumeDomain(&s->s);
    serviceClose(&s->s);
    eventClose(&s->event0);
    eventClose(&s->eventCtrlXfer);
    memset(s, 0, sizeof(UsbHsClientIfSession));
}

static Result _usbHsIfGetInf(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id, u32 cmd_id) {
    if (inf==NULL) inf = &s->inf.inf;

    serviceAssumeDomain(&s->s);
    Result rc = serviceDispatchIn(&s->s, cmd_id, id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { inf, sizeof(UsbHsInterfaceInfo) } },
    );
    if (R_SUCCEEDED(rc)) _usbHsConvertInterfaceInfoToV8(inf);
    return rc;
}

Result usbHsIfSetInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id) {
    return _usbHsIfGetInf(s, inf, id, 1);
}

Result usbHsIfGetAlternateInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id) {
    return _usbHsIfGetInf(s, inf, id, 3);
}

Result usbHsIfGetInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf) {
    if (inf==NULL) inf = &s->inf.inf;

    Result rc = _usbHsCmdRecvBufNoOut(&s->s, inf, sizeof(UsbHsInterfaceInfo), 2);
    if (R_SUCCEEDED(rc)) _usbHsConvertInterfaceInfoToV8(inf);
    return rc;
}

Result usbHsIfGetCurrentFrame(UsbHsClientIfSession* s, u32* out) {
    return _usbDsCmdNoInOutU32(&s->s, out, hosversionAtLeast(2,0,0) ? 4 : 5);
}

static Result _usbHsIfSubmitControlRequest(UsbHsClientIfSession* s, u8 bRequest, u8 bmRequestType, u16 wValue, u16 wIndex, u16 wLength, void* buffer, u32 timeoutInMs, u32* transferredSize) {
    bool dir = (bmRequestType & USB_ENDPOINT_IN) != 0;
    size_t bufsize = (wLength + 0xFFF) & ~0xFFF;

    armDCacheFlush(buffer, wLength);

    const struct {
        u8 bRequest;
        u8 bmRequestType;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
        u32 timeoutInMs;
    } in = { bRequest, bmRequestType, wValue, wIndex, wLength, timeoutInMs };

    serviceAssumeDomain(&s->s);
    Result rc = serviceDispatchInOut(&s->s, dir ? 6 : 7, in, *transferredSize,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | (dir ? SfBufferAttr_Out : SfBufferAttr_In) },
        .buffers = { { buffer, bufsize } },
    );
    if (dir) armDCacheFlush(buffer, wLength);
    return rc;
}

static Result _usbHsIfCtrlXferAsync(UsbHsClientIfSession* s, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, void* buffer) {
    const struct {
        u8 bmRequestType;
        u8 bRequest;
        u16 wValue;
        u16 wIndex;
        u16 wLength;
        u64 buffer;
    } in = { bmRequestType, bRequest, wValue, wIndex, wLength, (u64)buffer };

    serviceAssumeDomain(&s->s);
    return serviceDispatchIn(&s->s, 5, in);
}

static Result _usbHsIfGetCtrlXferReport(UsbHsClientIfSession* s, UsbHsXferReport* report) {
    return _usbHsCmdRecvBufNoOut(&s->s, report, sizeof(UsbHsXferReport), 7);
}

Result usbHsIfCtrlXfer(UsbHsClientIfSession* s, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, void* buffer, u32* transferredSize) {
    Result rc=0;
    UsbHsXferReport report;

    if (hosversionBefore(2,0,0)) return _usbHsIfSubmitControlRequest(s, bRequest, bmRequestType, wValue, wIndex, wLength, buffer, 0, transferredSize);

    rc = _usbHsIfCtrlXferAsync(s, bmRequestType, bRequest, wValue, wIndex, wLength, buffer);
    if (R_FAILED(rc)) return rc;

    rc = eventWait(&s->eventCtrlXfer, UINT64_MAX);
    if (R_FAILED(rc)) return rc;
    eventClear(&s->eventCtrlXfer);

    memset(&report, 0, sizeof(report));
    rc = _usbHsIfGetCtrlXferReport(s, &report);
    if (R_FAILED(rc)) return rc;

    *transferredSize = report.transferredSize;
    rc = report.res;

    return rc;
}

static Result _usbHsIfOpenUsbEp(UsbHsClientIfSession* s, UsbHsClientEpSession* ep, u16 maxUrbCount, u32 maxXferSize, struct usb_endpoint_descriptor *desc) {
    const struct {
        u16 maxUrbCount;
        u16 pad;
        u32 epType;
        u32 epNumber;
        u32 epDirection;
        u32 maxXferSize;
    } in = {
        maxUrbCount,
        0,
        (desc->bmAttributes & USB_TRANSFER_TYPE_MASK) + 1,
        desc->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK,
        (desc->bEndpointAddress & USB_ENDPOINT_IN) == 0 ? 0x1 : 0x2,
        maxXferSize
    };

    serviceAssumeDomain(&s->s);
    return serviceDispatchInOut(&s->s, hosversionAtLeast(2,0,0) ? 9 : 4, in, ep->desc,
        .out_num_objects = 1,
        .out_objects = &ep->s,
    );
}

Result usbHsIfOpenUsbEp(UsbHsClientIfSession* s, UsbHsClientEpSession* ep, u16 maxUrbCount, u32 maxXferSize, struct usb_endpoint_descriptor *desc) {
    memset(ep, 0, sizeof(UsbHsClientEpSession));
    Result rc = _usbHsIfOpenUsbEp(s, ep, maxUrbCount, maxXferSize, desc);
    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(2,0,0)) {
            rc = _usbHsCmdNoIO(&ep->s, 3);//Populate
            if (R_SUCCEEDED(rc)) rc = _usbHsGetEvent(&ep->s, &ep->eventXfer, false, 2);
        }

        if (R_SUCCEEDED(rc)) ep->maxUrbCount = maxUrbCount;

        if (R_FAILED(rc)) {
            serviceAssumeDomain(&ep->s);
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

    serviceAssumeDomain(&s->s);
    serviceClose(&s->s);
    eventClose(&s->eventXfer);
    memset(s, 0, sizeof(UsbHsClientEpSession));
}

static Result _usbHsEpSubmitRequest(UsbHsClientEpSession* s, void* buffer, u32 size, u32 timeoutInMs, u32* transferredSize) {
    bool dir = (s->desc.bEndpointAddress & USB_ENDPOINT_IN) != 0;
    size_t bufsize = (size + 0xFFF) & ~0xFFF;

    armDCacheFlush(buffer, size);

    const struct {
        u32 size;
        u32 timeoutInMs;//?
    } in = { size, timeoutInMs };

    serviceAssumeDomain(&s->s);
    Result rc = serviceDispatchInOut(&s->s, dir ? 1 : 0, in, *transferredSize,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | (dir ? SfBufferAttr_Out : SfBufferAttr_In) },
        .buffers = { { buffer, bufsize } },
    );
    if (dir) armDCacheFlush(buffer, size);
    return rc;
}

Result usbHsEpPostBufferAsync(UsbHsClientEpSession* s, void* buffer, u32 size, u64 id, u32* xferId) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 size;
        u32 pad;
        u64 buffer;
        u64 id;
    } in = { size, 0, (u64)buffer, id };

    serviceAssumeDomain(&s->s);
    return serviceDispatchInOut(&s->s, 4, in, *xferId);
}

Result usbHsEpGetXferReport(UsbHsClientEpSession* s, UsbHsXferReport* reports, u32 max_reports, u32* count) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (s->ringbuf == NULL) {
        serviceAssumeDomain(&s->s);
        return serviceDispatchInOut(&s->s, 5, max_reports, *count,
        .buffer_attrs = { (hosversionBefore(3,0,0) ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_Out },
        .buffers = { { reports, max_reports*sizeof(UsbHsXferReport) } },
        );
    }

    u32 total=0;

    UsbHsRingHeader *hdr = (UsbHsRingHeader*)s->ringbuf;
    UsbHsXferReport *ring_reports = (UsbHsXferReport*)(hdr+1);

    memset(reports, 0, max_reports*sizeof(UsbHsXferReport));

    for (u32 i=0; i<max_reports; i++) {
        u64 write_index = hdr->write_index;
        u64 read_index = hdr->read_index;

        if (write_index == read_index) break;

        if (read_index >= s->max_reports) return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen); // Official sw would Abort here.
        reports[i] = ring_reports[read_index];
        __asm__ __volatile__ ("dmb sy" ::: "memory");
        read_index = hdr->read_index;
        hdr->read_index = (read_index+1) % s->max_reports;
        total++;
    }

    *count = total;

    return 0;
}

Result usbHsEpPostBuffer(UsbHsClientEpSession* s, void* buffer, u32 size, u32* transferredSize) {
    Result rc=0;
    u32 xferId=0;
    u32 count=0;
    UsbHsXferReport report;

    if (hosversionBefore(2,0,0)) return _usbHsEpSubmitRequest(s, buffer, size, 0, transferredSize);

    rc = usbHsEpPostBufferAsync(s, buffer, size, 0, &xferId);
    if (R_FAILED(rc)) return rc;

    rc = eventWait(&s->eventXfer, UINT64_MAX);
    if (R_FAILED(rc)) return rc;
    eventClear(&s->eventXfer);

    memset(&report, 0, sizeof(report));
    rc = usbHsEpGetXferReport(s, &report, 1, &count);
    if (R_FAILED(rc)) return rc;

    if (count<1) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    *transferredSize = report.transferredSize;
    rc = report.res;

    return rc;
}

Result usbHsEpBatchBufferAsync(UsbHsClientEpSession* s, void* buffer, u32* urbs, u32 urbCount, u64 id, u32 unk1, u32 unk2, u32* xferId) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 urbCount;
        u32 unk1;
        u32 unk2;
        u32 pad;
        u64 buffer;
        u64 id;
    } in = { urbCount, unk1, unk2, 0, (u64)buffer, id };

    serviceAssumeDomain(&s->s);
    return serviceDispatchInOut(&s->s, 6, in, *xferId,
        .buffer_attrs = { (hosversionBefore(3,0,0) ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_In },
        .buffers = { { urbs, urbCount*sizeof(u32) } },
    );
}

Result usbHsEpCreateSmmuSpace(UsbHsClientEpSession* s, void* buffer, u32 size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 size;
        u32 pad;
        u64 buffer;
    } in = { size, 0, (u64)buffer };

    serviceAssumeDomain(&s->s);
    return serviceDispatchIn(&s->s, 7, in);
}

Result usbHsEpShareReportRing(UsbHsClientEpSession* s, void* buffer, size_t size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (s->ringbuf)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    s->max_reports = s->maxUrbCount * 0x21;

    TransferMemory tmem={};
    Result rc = tmemCreateFromMemory(&tmem, buffer, size, Perm_Rw);
    if (R_FAILED(rc)) return rc;

    serviceAssumeDomain(&s->s);
    rc = serviceDispatchIn(&s->s, 8, size,
        .in_num_handles = 1,
        .in_handles = { tmem.handle },
    );
    tmemClose(&tmem);
    if (R_SUCCEEDED(rc)) s->ringbuf = buffer;
    return rc;
}

