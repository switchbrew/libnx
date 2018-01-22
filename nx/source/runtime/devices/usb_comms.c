#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/fatal.h"
#include "services/usb.h"
#include "runtime/devices/usb_comms.h"

static bool g_usbCommsInitialized = false;

static UsbDsInterface* interface = NULL;
static UsbDsEndpoint *g_usbComms_endpoint_in = NULL, *g_usbComms_endpoint_out = NULL;

static u8 *g_usbComms_endpoint_in_buffer = NULL, *g_usbComms_endpoint_out_buffer = NULL;

static Result _usbCommsInit(void);

static Result _usbCommsWrite(const void* buffer, size_t size, size_t *transferredSize);

Result usbCommsInitialize(void)
{
    if (g_usbCommsInitialized) return 0;

    Result ret=0;

    ret = usbDsInitialize(UsbComplexId_Default, NULL);

    if (R_SUCCEEDED(ret)) {
        //The buffer for PostBufferAsync commands must be 0x1000-byte aligned.
		g_usbComms_endpoint_in_buffer = memalign(0x1000, 0x1000);
		if (g_usbComms_endpoint_in_buffer==NULL) ret = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

        if (R_SUCCEEDED(ret)) {
		    g_usbComms_endpoint_out_buffer = memalign(0x1000, 0x1000);
		    if (g_usbComms_endpoint_out_buffer==NULL) ret = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        }

        if (R_SUCCEEDED(ret)) {
            memset(g_usbComms_endpoint_in_buffer, 0, 0x1000);
            memset(g_usbComms_endpoint_out_buffer, 0, 0x1000);
            ret = _usbCommsInit();

            if (ret != 0) {
                ret += 2000<<9;
            }
        }

        if (R_FAILED(ret)) {
            usbDsExit();

            free(g_usbComms_endpoint_in_buffer);
            g_usbComms_endpoint_in_buffer = NULL;

            free(g_usbComms_endpoint_out_buffer);
            g_usbComms_endpoint_out_buffer = NULL;
        }
    }
    else {
        ret += 1000<<9;
    }

    if (R_SUCCEEDED(ret)) g_usbCommsInitialized=true;

    return ret;
}

void usbCommsExit(void)
{
    if (!g_usbCommsInitialized) return;

    usbDsExit();

    g_usbCommsInitialized = false;

    g_usbComms_endpoint_in = NULL;
    g_usbComms_endpoint_out = NULL;

    free(g_usbComms_endpoint_in_buffer);
    g_usbComms_endpoint_in_buffer = NULL;

    free(g_usbComms_endpoint_out_buffer);
    g_usbComms_endpoint_out_buffer = NULL;
}

static Result _usbCommsInit(void)
{
    Result ret=0;

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
    ret = usbDsInterface_GetDsEndpoint(interface, &g_usbComms_endpoint_in, &endpoint_descriptor_in);//device->host
    if (R_FAILED(ret)) return ret;

    ret = usbDsInterface_GetDsEndpoint(interface, &g_usbComms_endpoint_out, &endpoint_descriptor_out);//host->device
    if (R_FAILED(ret)) return ret;

    ret = usbDsInterface_EnableInterface(interface);
    if (R_FAILED(ret)) return ret;

    return ret;
}

static Result _usbCommsRead(void* buffer, size_t size, size_t *transferredSize)
{
    Result ret=0;
    u32 urbId=0;
    u8 *bufptr = (u8*)buffer;
    u8 *transfer_buffer = NULL;
    u8 transfer_type=0;
    u32 chunksize=0;
    u32 tmp_transferredSize = 0;
    size_t total_transferredSize=0;
    usbDsReportData reportdata;

    //Makes sure endpoints are ready for data-transfer / wait for init if needed.
    ret = usbDsWaitReady();
    if (R_FAILED(ret)) return ret;

    while(size)
    {
        if(((u64)bufptr) & 0xfff)//When bufptr isn't page-aligned copy the data into g_usbComms_endpoint_in_buffer and transfer that, otherwise use the bufptr directly.
        {
            transfer_buffer = g_usbComms_endpoint_out_buffer;
            memset(g_usbComms_endpoint_out_buffer, 0, 0x1000);

            chunksize = 0x1000;
            chunksize-= ((u64)bufptr) & 0xfff;//After this transfer, bufptr will be page-aligned(if size is large enough for another transfer).
            if (size<chunksize) chunksize = size;

            transfer_type = 0;
        }
        else
        {
            transfer_buffer = bufptr;
            chunksize = size;

            transfer_type = 1;
        }

        //Start a host->device transfer.
        ret = usbDsEndpoint_PostBufferAsync(g_usbComms_endpoint_out, transfer_buffer, chunksize, &urbId);
        if (R_FAILED(ret)) return ret;

        //Wait for the transfer to finish.
        svcWaitSynchronizationSingle(g_usbComms_endpoint_out->CompletionEvent, U64_MAX);
        svcClearEvent(g_usbComms_endpoint_out->CompletionEvent);

        ret = usbDsEndpoint_GetReportData(g_usbComms_endpoint_out, &reportdata);
        if (R_FAILED(ret)) return ret;

        ret = usbDsParseReportData(&reportdata, urbId, NULL, &tmp_transferredSize);
        if (R_FAILED(ret)) return ret;

        if (tmp_transferredSize > chunksize) tmp_transferredSize = chunksize;
        total_transferredSize+= (size_t)tmp_transferredSize;

        if (transfer_type==0) memcpy(bufptr, transfer_buffer, tmp_transferredSize);
        bufptr+= tmp_transferredSize;
        size-= tmp_transferredSize;

        if(tmp_transferredSize < chunksize)break;
    }

    if (transferredSize) *transferredSize = total_transferredSize;

    return ret;
}

static Result _usbCommsWrite(const void* buffer, size_t size, size_t *transferredSize)
{
    Result ret=0;
    u32 urbId=0;
    u32 chunksize=0;
    u8 *bufptr = (u8*)buffer;
    u8 *transfer_buffer = NULL;
    u32 tmp_transferredSize = 0;
    size_t total_transferredSize=0;
    usbDsReportData reportdata;

    //Makes sure endpoints are ready for data-transfer / wait for init if needed.
    ret = usbDsWaitReady();
    if (R_FAILED(ret)) return ret;

    while(size)
    {
        if(((u64)bufptr) & 0xfff)//When bufptr isn't page-aligned copy the data into g_usbComms_endpoint_in_buffer and transfer that, otherwise use the bufptr directly.
        {
            transfer_buffer = g_usbComms_endpoint_in_buffer;
            memset(g_usbComms_endpoint_in_buffer, 0, 0x1000);

            chunksize = 0x1000;
            chunksize-= ((u64)bufptr) & 0xfff;//After this transfer, bufptr will be page-aligned(if size is large enough for another transfer).
            if (size<chunksize) chunksize = size;

            memcpy(g_usbComms_endpoint_in_buffer, bufptr, chunksize);
        }
        else
        {
            transfer_buffer = bufptr;
            chunksize = size;
        }

        //Start a device->host transfer.
        ret = usbDsEndpoint_PostBufferAsync(g_usbComms_endpoint_in, transfer_buffer, chunksize, &urbId);
        if(R_FAILED(ret))return ret;

        //Wait for the transfer to finish.
        svcWaitSynchronizationSingle(g_usbComms_endpoint_in->CompletionEvent, U64_MAX);
        svcClearEvent(g_usbComms_endpoint_in->CompletionEvent);

        ret = usbDsEndpoint_GetReportData(g_usbComms_endpoint_in, &reportdata);
        if (R_FAILED(ret)) return ret;

        ret = usbDsParseReportData(&reportdata, urbId, NULL, &tmp_transferredSize);
        if (R_FAILED(ret)) return ret;

        if (tmp_transferredSize > chunksize) tmp_transferredSize = chunksize;

        total_transferredSize+= (size_t)tmp_transferredSize;

        bufptr+= tmp_transferredSize;
        size-= tmp_transferredSize;

        if (tmp_transferredSize < chunksize) break;
    }

    if (transferredSize) *transferredSize = total_transferredSize;

    return ret;
}

size_t usbCommsRead(void* buffer, size_t size)
{
    size_t transferredSize=0;
    u32 state=0;
    Result ret, ret2;
    ret = _usbCommsRead(buffer, size, &transferredSize);
    if (R_FAILED(ret)) {
        ret2 = usbDsGetState(&state);
        if (R_SUCCEEDED(ret2)) {
            if (state!=5) ret = _usbCommsRead(buffer, size, &transferredSize); //If state changed during transfer, try again. usbDsWaitReady() will be called from this.
        }
        if (R_FAILED(ret))fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadUsbCommsRead));
    }
    return transferredSize;
}

size_t usbCommsWrite(const void* buffer, size_t size)
{
    size_t transferredSize=0;
    u32 state=0;
    Result ret, ret2;
    ret = _usbCommsWrite(buffer, size, &transferredSize);
    if (R_FAILED(ret)) {
        ret2 = usbDsGetState(&state);
        if (R_SUCCEEDED(ret2)) {
            if (state!=5) ret = _usbCommsWrite(buffer, size, &transferredSize); //If state changed during transfer, try again. usbDsWaitReady() will be called from this.
        }
        if (R_FAILED(ret))fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadUsbCommsWrite));
    }
    return transferredSize;
}

