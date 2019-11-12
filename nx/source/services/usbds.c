#include "service_guard.h"
#include <string.h>
#include "arm/cache.h"
#include "services/usbds.h"
#include "runtime/hosversion.h"
#include "runtime/util/utf.h"

#define TOTAL_INTERFACES 4
#define TOTAL_ENDPOINTS_IN 16
#define TOTAL_ENDPOINTS_OUT 16
#define TOTAL_ENDPOINTS (TOTAL_ENDPOINTS_IN+TOTAL_ENDPOINTS_OUT)

static Service g_usbDsSrv;
static Event g_usbDsStateChangeEvent = {0};

static UsbDsInterface g_usbDsInterfaceTable[TOTAL_INTERFACES];
static UsbDsEndpoint g_usbDsEndpointTable[TOTAL_INTERFACES][TOTAL_ENDPOINTS];

static void _usbDsFreeTables(void);

static Result _usbDsBindDevice(UsbComplexId complexId);
static Result _usbDsBindClientProcess(Handle prochandle);
static Result _usbDsGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(usbDs);

Result _usbDsInitialize(void) {
    Result rc = 0;

    rc = smGetService(&g_usbDsSrv, "usb:ds");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_usbDsSrv);
    }

    if (R_SUCCEEDED(rc))
        rc = _usbDsBindDevice(UsbComplexId_Default);
    if (R_SUCCEEDED(rc))
        rc = _usbDsBindClientProcess(CUR_PROCESS_HANDLE);

    // GetStateChangeEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&g_usbDsSrv, &g_usbDsStateChangeEvent, false, 3);

    // Result code doesn't matter here, users can call themselves later, too. This prevents foot shooting.
    if (R_SUCCEEDED(rc) && hosversionAtLeast(5,0,0))
        usbDsClearDeviceData();

    return rc;
}

void _usbDsCleanup(void) {
    if (hosversionAtLeast(5,0,0) && serviceIsActive(&g_usbDsSrv)) {
        usbDsDisable();
    }

    _usbDsFreeTables();

    eventClose(&g_usbDsStateChangeEvent);
    serviceClose(&g_usbDsSrv);
}

Service* usbDsGetServiceSession(void) {
    return &g_usbDsSrv;
}

Event* usbDsGetStateChangeEvent(void) {
    return &g_usbDsStateChangeEvent;
}

static UsbDsInterface* _usbDsTryAllocateInterface(u8 num) {
    if (num >= TOTAL_INTERFACES) return NULL;
    UsbDsInterface* ptr = &g_usbDsInterfaceTable[num];
    if (ptr->initialized) return NULL;
    memset(ptr, 0, sizeof(UsbDsInterface));
    ptr->initialized = true;
    ptr->interface_index = num;
    return ptr;
}

static UsbDsEndpoint* _usbDsAllocateEndpoint(UsbDsInterface* interface) {
    u32 pos;
    UsbDsEndpoint* ptr = NULL;

    if (interface->interface_index>TOTAL_INTERFACES) return NULL;

    for (pos=0; pos<TOTAL_ENDPOINTS; pos++) {
        ptr = &g_usbDsEndpointTable[interface->interface_index][pos];
        if (ptr->initialized) continue;
        memset(ptr, 0, sizeof(UsbDsEndpoint));
        ptr->initialized = true;
        return ptr;
    }

    return NULL;
}

static void _usbDsFreeEndpoint(UsbDsEndpoint* endpoint) {
    if (!endpoint->initialized)
        return;
    
    /* Cancel any ongoing transactions. */
    usbDsEndpoint_Cancel(endpoint);

    eventClose(&endpoint->CompletionEvent);

    serviceAssumeDomain(&endpoint->s);
    serviceClose(&endpoint->s);

    endpoint->initialized = false;
}

static void _usbDsFreeInterface(UsbDsInterface* interface) {
    if (!interface->initialized)
        return;
    
    /* Disable interface. */
    usbDsInterface_DisableInterface(interface);
    
    /* Close endpoints. */
    for (u32 ep = 0; ep < TOTAL_ENDPOINTS; ep++) {
        _usbDsFreeEndpoint(&g_usbDsEndpointTable[interface->interface_index][ep]);
    }

    eventClose(&interface->CtrlOutCompletionEvent);
    eventClose(&interface->CtrlInCompletionEvent);
    eventClose(&interface->SetupEvent);

    serviceAssumeDomain(&interface->s);
    serviceClose(&interface->s);

    interface->initialized = false;
}

static void _usbDsFreeTables(void) {
    for (u32 intf = 0; intf < TOTAL_INTERFACES; intf++) {
        _usbDsFreeInterface(&g_usbDsInterfaceTable[intf]);
    }
}

static Result _usbDsGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _usbDsGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _usbDsGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _usbDsCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _usbDsCmdInU8NoOut(Service* srv, u8 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _usbDsCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return _usbDsCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _usbDsCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _usbDsCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _usbDsCmdSendBufNoOut(Service* srv, const void* buffer, size_t size, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _usbDsBindDevice(UsbComplexId complexId) {
    return _usbDsCmdInU32NoOut(&g_usbDsSrv, complexId, 0);
}

static Result _usbDsBindClientProcess(Handle prochandle) {
    serviceAssumeDomain(&g_usbDsSrv);
    return serviceDispatch(&g_usbDsSrv, 1,
        .in_num_handles = 1,
        .in_handles = { prochandle },
    );
}

Result usbDsGetState(u32 *out) {
    return _usbDsCmdNoInOutU32(&g_usbDsSrv, out, 4);
}

Result usbDsWaitReady(u64 timeout) {
    Result rc;
    u32 state = 0;

    rc = usbDsGetState(&state);
    if (R_FAILED(rc)) return rc;

    while (R_SUCCEEDED(rc) && state != 5) {
        eventWait(&g_usbDsStateChangeEvent, timeout);
        eventClear(&g_usbDsStateChangeEvent);
        rc = usbDsGetState(&state);
    }

    return rc;
}

Result usbDsParseReportData(UsbDsReportData *reportdata, u32 urbId, u32 *requestedSize, u32 *transferredSize) {
    Result rc = 0;
    u32 pos;
    u32 count = reportdata->report_count;
    UsbDsReportEntry *entry = NULL;
    if (count>8) count = 8;

    for(pos=0; pos<count; pos++) {
        entry = &reportdata->report[pos];
        if (entry->id == urbId) break;
    }

    if (pos == count) return MAKERESULT(Module_Libnx, LibnxError_NotFound);

    switch(entry->urb_status) {
	case 0x3:
            rc = 0;
        break;

        case 0x4:
            rc = 0x828c;
        break;

        case 0x5:
            rc = 0x748c;
        break;

        default:
            rc = 0x108c;
        break;
    }

    if (R_SUCCEEDED(rc)) {
        if (requestedSize) *requestedSize = entry->requestedSize;
        if (transferredSize) *transferredSize = entry->transferredSize;
    }

    return rc;
}

Result usbDsSetVidPidBcd(const UsbDsDeviceInfo* deviceinfo) {
    if (hosversionAtLeast(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _usbDsCmdSendBufNoOut(&g_usbDsSrv, deviceinfo, sizeof(UsbDsDeviceInfo), 5);
}

static Result _usbDsPostBuffer(Service* srv, void* buffer, size_t size, u32 *urbId, u32 cmd_id) {
    armDCacheFlush(buffer, size);

    const struct {
        u32 size;
        u32 padding;
        u64 buffer;
    } in = { (u32)size, 0, (u64)buffer };

    serviceAssumeDomain(srv);
    return serviceDispatchInOut(srv, cmd_id, in, *urbId);
}

static Result _usbDsGetReport(Service* srv, UsbDsReportData *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _usbDsGetDsInterface(Service* srv, Service* srv_out, const void* buf0, size_t buf0size, const void* buf1, size_t buf1size, u8 *out) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, 2, *out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { buf0, buf0size },
            { buf1, buf1size },
        },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result usbDsGetDsInterface(UsbDsInterface** interface, struct usb_interface_descriptor* _descriptor, const char *interface_name) {
    if (hosversionAtLeast(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct usb_interface_descriptor send_desc = *_descriptor;
    Service srv;
    Result rc;
    for (u32 i = 0; i < TOTAL_INTERFACES; i++) {
        send_desc.bInterfaceNumber = i;
        rc = _usbDsGetDsInterface(&g_usbDsSrv, &srv, &send_desc, USB_DT_INTERFACE_SIZE, interface_name, strlen(interface_name)+1, NULL);
        if (R_SUCCEEDED(rc)) {
            break;
        }
    }
    
    if (R_FAILED(rc)) {
        return rc;
    }
    
    UsbDsInterface* ptr = _usbDsTryAllocateInterface(send_desc.bInterfaceNumber);
    if(ptr == NULL) {
        serviceAssumeDomain(&srv);
        serviceClose(&srv);
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }
    
    ptr->s = srv;

    // GetSetupEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->SetupEvent, false, 1);
    // GetCtrlInCompletionEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->CtrlInCompletionEvent, false, 7);
    // GetCtrlOutCompletionEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->CtrlOutCompletionEvent, false, 9);

    if (R_FAILED(rc))
        _usbDsFreeInterface(ptr);

    if (R_SUCCEEDED(rc))
        *interface = ptr;

    return rc;
}

static Result _usbDsRegisterInterface(Service* srv_out, u8 intf_num) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_usbDsSrv);
    return serviceDispatchIn(&g_usbDsSrv, 2, intf_num,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result usbDsRegisterInterface(UsbDsInterface** interface) {
    Service srv;
    Result rc;
    u8 intf_num;
    for (intf_num = 0; intf_num < TOTAL_INTERFACES; intf_num++) {
        rc = _usbDsRegisterInterface(&srv, intf_num);
        if (R_SUCCEEDED(rc)) break;
    }
    
    if (R_FAILED(rc)) {
        return rc;
    }
    
    UsbDsInterface* ptr = _usbDsTryAllocateInterface(intf_num);
    if (ptr == NULL) {
        serviceAssumeDomain(&srv);
        serviceClose(&srv);
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }
    
    ptr->s = srv;

    // GetSetupEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->SetupEvent, false, 1);
    // GetCtrlInCompletionEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->CtrlInCompletionEvent, false, 7);
    // GetCtrlOutCompletionEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->CtrlOutCompletionEvent, false, 9);

    if (R_FAILED(rc))
        _usbDsFreeInterface(ptr);

    if (R_SUCCEEDED(rc))
        *interface = ptr;

    return rc;
}

Result usbDsRegisterInterfaceEx(UsbDsInterface** interface, u8 intf_num) {
    UsbDsInterface* ptr = _usbDsTryAllocateInterface(intf_num);
    if (ptr == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    Result rc=0;
    rc = _usbDsRegisterInterface(&ptr->s, intf_num);

    // GetSetupEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->SetupEvent, false, 1);
    // GetCtrlInCompletionEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->CtrlInCompletionEvent, false, 7);
    // GetCtrlOutCompletionEvent
    if (R_SUCCEEDED(rc))
        rc = _usbDsGetEvent(&ptr->s, &ptr->CtrlOutCompletionEvent, false, 9);

    if (R_FAILED(rc))
        _usbDsFreeInterface(ptr);

    if (R_SUCCEEDED(rc))
        *interface = ptr;

    return rc;
}

Result usbDsClearDeviceData(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _usbDsCmdNoIO(&g_usbDsSrv, 5);
}

static Result _usbDsAddUsbStringDescriptorRaw(u8 *out_index, struct usb_string_descriptor *descriptor) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_usbDsSrv);
    return serviceDispatchOut(&g_usbDsSrv, 6, *out_index,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { descriptor, sizeof(*descriptor) } },
    );
}

Result usbDsAddUsbStringDescriptor(u8* out_index, const char* string) {
    struct usb_string_descriptor descriptor = {
        .bDescriptorType = USB_DT_STRING,
        .wData = {0},
    };
    
    // Convert
    u32 len = (u32)utf8_to_utf16(descriptor.wData, (const uint8_t *)string, sizeof(descriptor.wData)/sizeof(u16) - 1);
    if (len > sizeof(descriptor.wData)/sizeof(u16)) len = sizeof(descriptor.wData)/sizeof(u16);
    
    // Set length
    descriptor.bLength = 2 + 2 * len;
    
    return _usbDsAddUsbStringDescriptorRaw(out_index, &descriptor);
}

Result usbDsAddUsbLanguageStringDescriptor(u8* out_index, const u16* lang_ids, u16 num_langs) {
    if (num_langs > 0x40) num_langs = 0x40;
    
    struct usb_string_descriptor descriptor = {
        .bLength = 2 + 2 * num_langs,
        .bDescriptorType = USB_DT_STRING,
        .wData = {0},
    };
    
    for (u32 i = 0; i < num_langs; i++) {
        descriptor.wData[i] = lang_ids[i];
    }
    
    return _usbDsAddUsbStringDescriptorRaw(out_index, &descriptor);
}

Result usbDsDeleteUsbStringDescriptor(u8 index) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _usbDsCmdInU8NoOut(&g_usbDsSrv, index, 7);
}


Result usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed speed, struct usb_device_descriptor* descriptor) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=speed;
    serviceAssumeDomain(&g_usbDsSrv);
    return serviceDispatchIn(&g_usbDsSrv, 8, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { descriptor, USB_DT_DEVICE_SIZE } },
    );
}

Result usbDsSetBinaryObjectStore(const void* bos, size_t bos_size) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _usbDsCmdSendBufNoOut(&g_usbDsSrv, bos, bos_size, 9);
}

Result usbDsEnable(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _usbDsCmdNoIO(&g_usbDsSrv, 10);
}

Result usbDsDisable(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _usbDsCmdNoIO(&g_usbDsSrv, 11);
}


//IDsInterface

void usbDsInterface_Close(UsbDsInterface* interface) {
    _usbDsFreeInterface(interface);
}

Result usbDsInterface_GetSetupPacket(UsbDsInterface* interface, void* buffer, size_t size) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&interface->s);
    return serviceDispatch(&interface->s, 2,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

static Result _usbDsInterface_GetDsEndpoint(Service* srv, Service* srv_out, const void* buf0, size_t buf0size, u8 *out) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, 0, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf0, buf0size } },
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result usbDsInterface_GetDsEndpoint(UsbDsInterface* interface, UsbDsEndpoint** endpoint, struct usb_endpoint_descriptor* descriptor) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionAtLeast(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    UsbDsEndpoint* ptr = _usbDsAllocateEndpoint(interface);
    if (ptr==NULL) return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    Result rc = _usbDsInterface_GetDsEndpoint(&interface->s, &ptr->s, descriptor, USB_DT_ENDPOINT_SIZE, NULL);

    if (R_SUCCEEDED(rc)) rc = _usbDsGetEvent(&ptr->s, &ptr->CompletionEvent, false, 2);//GetCompletionEvent

    if (R_FAILED(rc)) _usbDsFreeEndpoint(ptr);

    if (R_SUCCEEDED(rc)) *endpoint = ptr;
    return rc;
}

Result usbDsInterface_EnableInterface(UsbDsInterface* interface) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsCmdNoIO(&interface->s, 3);
}

Result usbDsInterface_DisableInterface(UsbDsInterface* interface) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsCmdNoIO(&interface->s, 4);
}

Result usbDsInterface_CtrlInPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32 *urbId) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsPostBuffer(&interface->s, buffer, size, urbId, 5);
}

Result usbDsInterface_CtrlOutPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32 *urbId) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsPostBuffer(&interface->s, buffer, size, urbId, 6);
}

Result usbDsInterface_GetCtrlInReportData(UsbDsInterface* interface, UsbDsReportData *out) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsGetReport(&interface->s, out, 8);
}

Result usbDsInterface_GetCtrlOutReportData(UsbDsInterface* interface, UsbDsReportData *out) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsGetReport(&interface->s, out, 10);
}

Result usbDsInterface_StallCtrl(UsbDsInterface* interface) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsCmdNoIO(&interface->s, 11);
}

Result usbDsInterface_RegisterEndpoint(UsbDsInterface* interface, UsbDsEndpoint** endpoint, u8 endpoint_address) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    UsbDsEndpoint* ptr = _usbDsAllocateEndpoint(interface);
    if(ptr==NULL) return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    serviceAssumeDomain(&interface->s);
    Result rc = serviceDispatchIn(&interface->s, 0, endpoint_address,
        .out_num_objects = 1,
        .out_objects = &ptr->s,
    );
    
    if (R_SUCCEEDED(rc)) rc = _usbDsGetEvent(&ptr->s, &ptr->CompletionEvent, false, 2);//GetCompletionEvent

    if (R_FAILED(rc)) _usbDsFreeEndpoint(ptr);

    if (R_SUCCEEDED(rc)) *endpoint = ptr;
    return rc;
}

Result usbDsInterface_AppendConfigurationData(UsbDsInterface* interface, UsbDeviceSpeed speed, const void* buffer, size_t size) {
    if (!interface->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 intf_num;
        u32 speed;
    } in = { interface->interface_index, speed };

    return serviceDispatchIn(&interface->s, 12, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

//IDsEndpoint

void usbDsEndpoint_Close(UsbDsEndpoint* endpoint) {
    _usbDsFreeEndpoint(endpoint);
}

Result usbDsEndpoint_PostBufferAsync(UsbDsEndpoint* endpoint, void* buffer, size_t size, u32 *urbId) {
    if (!endpoint->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsPostBuffer(&endpoint->s, buffer, size, urbId, 0);
}

Result usbDsEndpoint_Cancel(UsbDsEndpoint* endpoint) {
    if (!endpoint->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsCmdNoIO(&endpoint->s, 1);
}

Result usbDsEndpoint_GetReportData(UsbDsEndpoint* endpoint, UsbDsReportData *out) {
    if (!endpoint->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsGetReport(&endpoint->s, out, 3);
}

Result usbDsEndpoint_Stall(UsbDsEndpoint* endpoint) {
    if (!endpoint->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsCmdNoIO(&endpoint->s, 4);
}

Result usbDsEndpoint_SetZlt(UsbDsEndpoint* endpoint, bool zlt) {
    if (!endpoint->initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _usbDsCmdInBoolNoOut(&endpoint->s, zlt, 5);
}

