#include <string.h>
#include <switch.h>

#define TOTAL_INTERFACES 4
#define TOTAL_ENDPOINTS 15*2

static Handle g_usbDsServiceSession = 0;
static Handle g_usbDsStateChangeEvent = 0;

static UsbDsInterface g_usbDsInterfaceTable[TOTAL_INTERFACES];
static UsbDsEndpoint g_usbDsEndpointTable[TOTAL_INTERFACES*TOTAL_ENDPOINTS];

static void _usbDsFreeTables(void);

static Result _usbDsBindDevice(usbComplexId complexId);
static Result _usbDsBindClientProcess(Handle prochandle);
static Result _usbDsGetEvent(Handle sessionhandle, Handle* handle_out, u64 cmd_id);
static Result _usbDsSetVidPidBcd(const usbDsDeviceInfo* deviceinfo);

static Result _usbDsGetSession(Handle sessionhandle, Handle* handle_out, u64 cmd_id, const void* buf0, size_t buf0size, const void* buf1, size_t buf1size);

Result usbDsInitialize(usbComplexId complexId, const usbDsDeviceInfo* deviceinfo) {
    if(g_usbDsServiceSession!=0)return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;

    rc = smGetService(&g_usbDsServiceSession, "usb:ds");

    if (R_SUCCEEDED(rc))rc = _usbDsBindDevice(complexId);
    if (R_SUCCEEDED(rc))rc = _usbDsBindClientProcess(CUR_PROCESS_HANDLE);
    if (R_SUCCEEDED(rc))rc = _usbDsGetEvent(g_usbDsServiceSession, &g_usbDsStateChangeEvent, 3);//GetStateChangeEvent
    if (R_SUCCEEDED(rc))rc = _usbDsSetVidPidBcd(deviceinfo);

    if (R_FAILED(rc)) {
        if(g_usbDsStateChangeEvent) {
            svcCloseHandle(g_usbDsStateChangeEvent);
            g_usbDsStateChangeEvent = 0;
        }

        if(g_usbDsServiceSession) {
            svcCloseHandle(g_usbDsServiceSession);
            g_usbDsServiceSession = 0;
        }
    }

    return rc;
}

void usbDsExit(void)
{
    if(g_usbDsServiceSession==0)return;

    _usbDsFreeTables();

    if(g_usbDsStateChangeEvent) {
        svcCloseHandle(g_usbDsStateChangeEvent);
        g_usbDsStateChangeEvent = 0;
    }

    if(g_usbDsServiceSession) {
        svcCloseHandle(g_usbDsServiceSession);
        g_usbDsServiceSession = 0;
    }
}

Handle usbDsGetServiceSession(void)
{
    return g_usbDsServiceSession;
}

Handle usbDsGetStateChangeEvent(void)
{
	return g_usbDsStateChangeEvent;
}

static UsbDsInterface* _usbDsAllocateInterface(void)
{
    u32 pos;
    UsbDsInterface* ptr = NULL;

    for(pos=0; pos<TOTAL_INTERFACES; pos++)
    {
        ptr = &g_usbDsInterfaceTable[pos];
        if(ptr->initialized)continue;
        memset(ptr, 0, sizeof(UsbDsInterface));
        ptr->initialized = true;
        ptr->interface_index = pos;
        return ptr;
    }

    return NULL;
}

static UsbDsEndpoint* _usbDsAllocateEndpoint(UsbDsInterface* interface)
{
    u32 pos;
    UsbDsEndpoint* ptr = NULL;

    if(interface->interface_index>TOTAL_INTERFACES)return NULL;

    for(pos=0; pos<TOTAL_ENDPOINTS; pos++)
    {
        ptr = &g_usbDsEndpointTable[(interface->interface_index*TOTAL_ENDPOINTS) + pos];
        if(ptr->initialized)continue;
        memset(ptr, 0, sizeof(UsbDsEndpoint));
        ptr->initialized = true;
        return ptr;
    }

    return NULL;
}

static void _usbDsFreeInterface(UsbDsInterface* interface)
{
    if(!interface->initialized)return;

    if(interface->CtrlOutCompletionEvent) {
        svcCloseHandle(interface->CtrlOutCompletionEvent);
        interface->CtrlOutCompletionEvent = 0;
    }

    if(interface->CtrlInCompletionEvent) {
        svcCloseHandle(interface->CtrlInCompletionEvent);
        interface->CtrlInCompletionEvent = 0;
    }

    if(interface->SetupEvent) {
        svcCloseHandle(interface->SetupEvent);
        interface->SetupEvent = 0;
    }

    if(interface->h) {
        svcCloseHandle(interface->h);
        interface->h = 0;
    }

    interface->initialized = false;
}

static void _usbDsFreeEndpoint(UsbDsEndpoint* endpoint)
{
    if(!endpoint->initialized)return;

    if(endpoint->CompletionEvent) {
        svcCloseHandle(endpoint->CompletionEvent);
        endpoint->CompletionEvent = 0;
    }

    if(endpoint->h) {
        svcCloseHandle(endpoint->h);
        endpoint->h = 0;
    }

    endpoint->initialized = false;
}

static void _usbDsFreeTables(void)
{
    u32 pos, pos2;
    for(pos=0; pos<TOTAL_INTERFACES; pos++)
    {
        for(pos2=0; pos2<TOTAL_ENDPOINTS; pos2++)_usbDsFreeEndpoint(&g_usbDsEndpointTable[(pos*TOTAL_ENDPOINTS) + pos2]);
        _usbDsFreeInterface(&g_usbDsInterfaceTable[pos]);
    }
}

static Result _usbDsBindDevice(usbComplexId complexId) {
    if(g_usbDsServiceSession==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 complexId;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->complexId = complexId;

    Result rc = ipcDispatch(g_usbDsServiceSession);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _usbDsBindClientProcess(Handle prochandle) {
    if(g_usbDsServiceSession==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcSendHandleCopy(&c, prochandle);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = ipcDispatch(g_usbDsServiceSession);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _usbDsGetEvent(Handle sessionhandle, Handle* handle_out, u64 cmd_id) {
    if(sessionhandle==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

Result usbDsGetState(u32 *out) {
    if(g_usbDsServiceSession==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = ipcDispatch(g_usbDsServiceSession);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out)*out = resp->out;
    }

    return rc;
}

Result usbDsWaitReady(void) {
    Result rc = 0;
    u32 state=0;
    s32 tmpindex = 0;

    rc = usbDsGetState(&state);
    if (R_FAILED(rc)) return rc;

    while(R_SUCCEEDED(rc) && state!=5)
    {
        svcWaitSynchronization(&tmpindex, &g_usbDsStateChangeEvent, 1, U64_MAX);
        svcClearEvent(g_usbDsStateChangeEvent);
        rc = usbDsGetState(&state);
    }

    return rc;
}

static Result _usbDsSetVidPidBcd(const usbDsDeviceInfo* deviceinfo) {
    if(g_usbDsServiceSession==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddSendBuffer(&c, deviceinfo, sizeof(usbDsDeviceInfo), 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;

    Result rc = ipcDispatch(g_usbDsServiceSession);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _usbDsGetSession(Handle sessionhandle, Handle* handle_out, u64 cmd_id, const void* buf0, size_t buf0size, const void* buf1, size_t buf1size) {
    if(sessionhandle==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    if(buf0 && buf0size)ipcAddSendBuffer(&c, buf0, buf0size, 0);
    if(buf1 && buf1size)ipcAddSendBuffer(&c, buf1, buf1size, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

static Result _usbDsCmdNoParams(Handle sessionhandle, u64 cmd_id) {
    if(sessionhandle==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _usbDsPostBuffer(Handle sessionhandle, u64 cmd_id, void* buffer, size_t size, u32 *out) {
    if(sessionhandle==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    osDCacheFlush(buffer, size);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 size;
        u32 padding;
        u64 buffer;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->size = (u32)size;
    raw->padding = 0;
    raw->buffer = (u64)buffer;

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out)*out = resp->out;
    }

    return rc;
}

static Result _usbDsGetReport(Handle sessionhandle, u64 cmd_id, u8 out[0x84]) {
    if(sessionhandle==0)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 out[0x84];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out)memcpy(out, resp->out, sizeof(resp->out));
    }

    return rc;
}

Result usbDsGetDsInterface(UsbDsInterface** interface, struct usb_interface_descriptor* descriptor, const char *interface_name)
{
	UsbDsInterface* ptr = _usbDsAllocateInterface();
        if(ptr==NULL)return MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);

	Result rc = _usbDsGetSession(g_usbDsServiceSession, &ptr->h, 2, descriptor, sizeof(struct usb_interface_descriptor), interface_name, strlen(interface_name)+1);

        if (R_SUCCEEDED(rc)) rc = _usbDsGetEvent(ptr->h, &ptr->SetupEvent, 1);//GetSetupEvent
        if (R_SUCCEEDED(rc)) rc = _usbDsGetEvent(ptr->h, &ptr->CtrlInCompletionEvent, 7);//GetCtrlInCompletionEvent
        if (R_SUCCEEDED(rc)) rc = _usbDsGetEvent(ptr->h, &ptr->CtrlOutCompletionEvent, 9);//GetCtrlOutCompletionEvent

        if (R_FAILED(rc)) _usbDsFreeInterface(ptr);

        if (R_SUCCEEDED(rc)) *interface = ptr;
        return rc;
}

//IDsInterface

void usbDsInterface_Close(UsbDsInterface* interface)
{
	_usbDsFreeInterface(interface);
}

Result usbDsInterface_GetDsEndpoint(UsbDsInterface* interface, UsbDsEndpoint** endpoint, struct usb_endpoint_descriptor* descriptor)
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	UsbDsEndpoint* ptr = _usbDsAllocateEndpoint(interface);
        if(ptr==NULL)return MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);

	Result rc = _usbDsGetSession(interface->h, &ptr->h, 0, descriptor, sizeof(struct usb_endpoint_descriptor), NULL, 0);

        if (R_SUCCEEDED(rc)) rc = _usbDsGetEvent(ptr->h, &ptr->CompletionEvent, 2);//GetCompletionEvent

        if (R_FAILED(rc)) _usbDsFreeEndpoint(ptr);

        if (R_SUCCEEDED(rc)) *endpoint = ptr;
        return rc;
}

Result usbDsInterface_EnableInterface(UsbDsInterface* interface)
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsCmdNoParams(interface->h, 3);
}

Result usbDsInterface_DisableInterface(UsbDsInterface* interface)
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsCmdNoParams(interface->h, 4);
}

Result usbDsInterface_CtrlInPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32 *out)
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsPostBuffer(interface->h, 5, buffer, size, out);
}

Result usbDsInterface_CtrlOutPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32 *out)
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsPostBuffer(interface->h, 6, buffer, size, out);
}

Result usbDsInterface_GetCtrlInReportData(UsbDsInterface* interface, u8 out[0x84])
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsGetReport(interface->h, 8, out);
}

Result usbDsInterface_GetCtrlOutReportData(UsbDsInterface* interface, u8 out[0x84])
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsGetReport(interface->h, 10, out);
}

Result usbDsInterface_StallCtrl(UsbDsInterface* interface)
{
        if(!interface->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsCmdNoParams(interface->h, 11);
}

//IDsEndpoint

void usbDsEndpoint_Close(UsbDsEndpoint* endpoint)
{
	_usbDsFreeEndpoint(endpoint);
}

Result usbDsEndpoint_PostBufferAsync(UsbDsEndpoint* endpoint, void* buffer, size_t size, u32 *out)
{
        if(!endpoint->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsPostBuffer(endpoint->h, 0, buffer, size, out);
}

Result usbDsEndpoint_GetCtrlOutReportData(UsbDsEndpoint* endpoint, u8 out[0x84])
{
        if(!endpoint->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsGetReport(endpoint->h, 3, out);
}

Result usbDsEndpoint_StallCtrl(UsbDsEndpoint* endpoint)
{
        if(!endpoint->initialized)return MAKERESULT(MODULE_LIBNX, LIBNX_NOTINITIALIZED);

	return _usbDsCmdNoParams(endpoint->h, 4);
}

