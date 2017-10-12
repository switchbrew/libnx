#include <string.h>
#include <malloc.h>
#include <switch.h>

//TODO: devoptab support

static bool g_usbDevInitialized = false;

static UsbDsInterface* interface = NULL;
static UsbDsEndpoint *g_usbDev_endpoint_in = NULL, *g_usbDev_endpoint_out = NULL;

static u8 *g_usbDev_endpoint_in_buffer = NULL, *g_usbDev_endpoint_out_buffer = NULL;

static Result _usbDevInit(void);

static Result _usbDevWrite(const void* buffer, size_t size, size_t *transferredSize);

Result usbDevInitialize(void)
{
    if (g_usbDevInitialized) return 0;

    Result ret=0;

    usbDsDeviceInfo deviceinfo = {
        .idVendor = 0x0403, // "Future Technology Devices International, Ltd"
        .idProduct = 0x6001, // "FT232 USB-Serial (UART) IC"
        .bcdDevice = 0x0200,
        .Manufacturer = "libnx",
        .Product = "usbDev",
        .SerialNumber = "1337",
    };

    ret = usbDsInitialize(USBCOMPLEXID_Default, &deviceinfo);

    if (R_SUCCEEDED(ret)) {
        //The buffer for PostBufferAsync commands must be 0x1000-byte aligned.
		g_usbDev_endpoint_in_buffer = memalign(0x1000, 0x1000);
		if (g_usbDev_endpoint_in_buffer==NULL) ret = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);

        if (R_SUCCEEDED(ret)) {
		    g_usbDev_endpoint_out_buffer = memalign(0x1000, 0x1000);
		    if (g_usbDev_endpoint_out_buffer==NULL) ret = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
        }

        if (R_SUCCEEDED(ret)) {
            memset(g_usbDev_endpoint_in_buffer, 0, 0x1000);
            memset(g_usbDev_endpoint_out_buffer, 0, 0x1000);
            ret = _usbDevInit();
        }

        if (R_FAILED(ret)) {
            usbDsExit();

            if (g_usbDev_endpoint_in_buffer) {
                free(g_usbDev_endpoint_in_buffer);
                g_usbDev_endpoint_in_buffer = NULL;
            }

            if (g_usbDev_endpoint_out) {
                free(g_usbDev_endpoint_out_buffer);
                g_usbDev_endpoint_out_buffer = NULL;
            }
        }
    }

    if (R_SUCCEEDED(ret)) g_usbDevInitialized=true;

    return ret;
}

void usbDevExit(void)
{
    if (!g_usbDevInitialized) return;

    usbDsExit();

    g_usbDevInitialized = false;

    g_usbDev_endpoint_in = NULL;
    g_usbDev_endpoint_out = NULL;

    if (g_usbDev_endpoint_in_buffer) {
        free(g_usbDev_endpoint_in_buffer);
        g_usbDev_endpoint_in_buffer = NULL;
    }

    if (g_usbDev_endpoint_out) {
        free(g_usbDev_endpoint_out_buffer);
        g_usbDev_endpoint_out_buffer = NULL;
    }
}

static Result _usbDevInit(void)
{
    Result ret=0;
    size_t transferredSize=0;

    struct usb_interface_descriptor interface_descriptor = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = USBDS_DEFAULT_InterfaceNumber,
        .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass = USB_CLASS_VENDOR_SPEC,
        .bInterfaceProtocol = USB_CLASS_VENDOR_SPEC,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_in = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_IN,
        .bmAttributes = USB_TRANSFER_TYPE_BULK,
        .wMaxPacketSize = 0x200,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_out = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_OUT,
        .bmAttributes = USB_TRANSFER_TYPE_BULK,
        .wMaxPacketSize = 0x200,
    };

    //Setup interface.
    ret = usbDsGetDsInterface(&interface, &interface_descriptor, "usb");
    if (R_FAILED(ret)) return ret;

    //Setup endpoints.
    ret = usbDsInterface_GetDsEndpoint(interface, &g_usbDev_endpoint_in, &endpoint_descriptor_in);//device->host
    if (R_FAILED(ret)) return ret;

    ret = usbDsInterface_GetDsEndpoint(interface, &g_usbDev_endpoint_out, &endpoint_descriptor_out);//host->device
    if (R_FAILED(ret)) return ret;

    ret = usbDsInterface_EnableInterface(interface);
    if (R_FAILED(ret)) return ret;

    //Host-side serial handling breaks with binary data without this.
    ret = _usbDevWrite("\n", 1, &transferredSize);
    if (R_SUCCEEDED(ret) && transferredSize!=1) ret = MAKERESULT(MODULE_LIBNX, LIBNX_IOERROR);

    return ret;
}

static Result _usbDevRead(void* buffer, size_t size, size_t *transferredSize)
{
    Result ret=0;
    u32 urbId=0;
    u8 *bufptr = (u8*)buffer;
    s32 tmpindex=0;
    u32 chunksize=0;
    u32 tmp_transferredSize = 0;
    size_t total_transferredSize=0;
    usbDsReportData reportdata;

    //Makes sure endpoints are ready for data-transfer / wait for init if needed.
    ret = usbDsWaitReady();
    if (R_FAILED(ret)) return ret;

    while(size)
    {
        chunksize = 0x1000;
        if (size<chunksize) chunksize = size;

        //Start a host->device transfer.
        ret = usbDsEndpoint_PostBufferAsync(g_usbDev_endpoint_out, g_usbDev_endpoint_out_buffer, chunksize, &urbId);
        if (R_FAILED(ret)) return ret;

        //Wait for the transfer to finish.
        svcWaitSynchronization(&tmpindex, &g_usbDev_endpoint_out->CompletionEvent, 1, U64_MAX);
        svcClearEvent(g_usbDev_endpoint_out->CompletionEvent);

        ret = usbDsEndpoint_GetReportData(g_usbDev_endpoint_out, &reportdata);
        if (R_FAILED(ret)) return ret;

        ret = usbDsParseReportData(&reportdata, urbId, NULL, &tmp_transferredSize);
        if (R_FAILED(ret)) return ret;

        if (tmp_transferredSize > chunksize) tmp_transferredSize = chunksize;
        total_transferredSize+= (size_t)tmp_transferredSize;

        memcpy(bufptr, g_usbDev_endpoint_out_buffer, chunksize);
        bufptr+= chunksize;
        size-= chunksize;

        if(tmp_transferredSize < chunksize)break;
    }

    if (transferredSize) *transferredSize = total_transferredSize;

    return ret;
}

static Result _usbDevWrite(const void* buffer, size_t size, size_t *transferredSize)
{
    Result ret=0;
    u32 urbId=0;
    u32 chunksize=0;
    u32 bufpos=0;
    u32 transfer_size=0;
    u32 total_chunks=0;
    u32 last_chunksize=0;
    u8 *bufptr = (u8*)buffer;
    s32 tmpindex=0;
    u32 tmp_transferredSize = 0;
    u32 partial_transfer=0;
    size_t total_transferredSize=0;
    usbDsReportData reportdata;

    //Makes sure endpoints are ready for data-transfer / wait for init if needed.
    ret = usbDsWaitReady();
    if (R_FAILED(ret)) return ret;

    while(size)
    {
        memset(g_usbDev_endpoint_in_buffer, 0, 0x1000);
        transfer_size = 0;
        total_chunks = 0;

        for(bufpos=0; bufpos<0x1000; bufpos+=0x200)
        {
            chunksize = 0x200;
            if(size<chunksize)chunksize = size+2;
            last_chunksize = chunksize;

            g_usbDev_endpoint_in_buffer[bufpos+0] = 0x11;//2-byte header then the actual data.
            g_usbDev_endpoint_in_buffer[bufpos+1] = 0x1;
            memcpy(&g_usbDev_endpoint_in_buffer[bufpos+2], bufptr, chunksize-2);

            size-= chunksize-2;
            bufptr+= chunksize-2;
            transfer_size+= chunksize;
            total_chunks++;

            if(size==0)break;
        }

        //Start a device->host transfer.
        ret = usbDsEndpoint_PostBufferAsync(g_usbDev_endpoint_in, g_usbDev_endpoint_in_buffer, transfer_size, &urbId);
        if(R_FAILED(ret))return ret;

        //Wait for the transfer to finish.
        svcWaitSynchronization(&tmpindex, &g_usbDev_endpoint_in->CompletionEvent, 1, U64_MAX);
        svcClearEvent(g_usbDev_endpoint_in->CompletionEvent);

        ret = usbDsEndpoint_GetReportData(g_usbDev_endpoint_in, &reportdata);
        if (R_FAILED(ret)) return ret;

        ret = usbDsParseReportData(&reportdata, urbId, NULL, &tmp_transferredSize);
        if (R_FAILED(ret)) return ret;

        if (tmp_transferredSize > transfer_size) tmp_transferredSize = transfer_size;

        partial_transfer = 0;
        for(bufpos=0; bufpos<transfer_size; bufpos+=0x200)
        {
            if(tmp_transferredSize < bufpos+2) {
                partial_transfer = 1;
                break;
            }

            chunksize = 0x200;
            if(total_chunks==1)chunksize = last_chunksize;
            total_chunks--;

            if (tmp_transferredSize < bufpos+chunksize) {
                total_transferredSize+= tmp_transferredSize - bufpos - 2;
                partial_transfer = 1;
                break;
            }

            total_transferredSize+= chunksize-2;
        }
        if (partial_transfer) break;
    }

    if (transferredSize) *transferredSize = total_transferredSize;

    return ret;
}

size_t usbDevRead(void* buffer, size_t size)
{
    size_t transferredSize=0;
    Result ret = _usbDevRead(buffer, size, &transferredSize);
    if (R_FAILED(ret)) fatalSimple(ret);
    return transferredSize;
}

size_t usbDevWrite(const void* buffer, size_t size)
{
    size_t transferredSize=0;
    Result ret = _usbDevWrite(buffer, size, &transferredSize);
    if (R_FAILED(ret)) fatalSimple(ret);
    return transferredSize;
}

