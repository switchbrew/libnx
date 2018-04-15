#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "kernel/rwlock.h"
#include "services/fatal.h"
#include "services/usb.h"
#include "runtime/devices/usb_comms.h"

#define TOTAL_INTERFACES 4

typedef struct {
    RwLock lock, lock_in, lock_out;
    bool initialized;

    UsbDsInterface* interface;
    UsbDsEndpoint *endpoint_in, *endpoint_out;

    u8 *endpoint_in_buffer, *endpoint_out_buffer;
} usbCommsInterface;

static bool g_usbCommsInitialized = false;

static usbCommsInterface g_usbCommsInterfaces[TOTAL_INTERFACES];

static RwLock g_usbCommsLock;

static Result _usbCommsInterfaceInit(usbCommsInterface *interface, u8 bInterfaceClass, u8 bInterfaceSubClass, u8 bInterfaceProtocol);

static Result _usbCommsWrite(usbCommsInterface *interface, const void* buffer, size_t size, size_t *transferredSize);

Result usbCommsInitializeEx(u32 *interface, u8 bInterfaceClass, u8 bInterfaceSubClass, u8 bInterfaceProtocol)
{
    bool found=0;
    usbCommsInterface *inter = NULL;

    rwlockWriteLock(&g_usbCommsLock);

    if (g_usbCommsInitialized && interface==NULL) {
        rwlockWriteUnlock(&g_usbCommsLock);
        return 0;
    }

    Result rc=0;
    u32 i = 0;

    if (!g_usbCommsInitialized) rc = usbDsInitialize(UsbComplexId_Default, NULL);

    if (R_SUCCEEDED(rc)) {
        for(i=0; i<TOTAL_INTERFACES; i++) {
            inter = &g_usbCommsInterfaces[i];
            rwlockReadLock(&inter->lock);
            if (!inter->initialized) found=1;
            rwlockReadUnlock(&inter->lock);

            if (found) break;
        }

        if (!found) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }

    if (R_SUCCEEDED(rc)) {
        rwlockWriteLock(&inter->lock);
        rwlockWriteLock(&inter->lock_in);
        rwlockWriteLock(&inter->lock_out);
        rc = _usbCommsInterfaceInit(inter, bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol);
        rwlockWriteUnlock(&inter->lock_out);
        rwlockWriteUnlock(&inter->lock_in);
        rwlockWriteUnlock(&inter->lock);
    }

    if (R_FAILED(rc)) {
        usbCommsExit();
    }

    if (R_SUCCEEDED(rc)) g_usbCommsInitialized=true;

    if (R_SUCCEEDED(rc) && interface) *interface = i;

    rwlockWriteUnlock(&g_usbCommsLock);

    return rc;
}

Result usbCommsInitialize(void)
{
    return usbCommsInitializeEx(NULL, USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC);
}

static void _usbCommsInterfaceExit(usbCommsInterface *interface)
{
    rwlockWriteLock(&interface->lock);
    if (!interface->initialized) {
        rwlockWriteUnlock(&interface->lock);
        return;
    }

    rwlockWriteLock(&interface->lock_in);
    rwlockWriteLock(&interface->lock_out);

    interface->initialized = 0;

    usbDsInterface_DisableInterface(interface->interface);
    usbDsEndpoint_Close(interface->endpoint_in);
    usbDsEndpoint_Close(interface->endpoint_out);
    usbDsInterface_Close(interface->interface);

    interface->endpoint_in = NULL;
    interface->endpoint_out = NULL;
    interface->interface = NULL;

    free(interface->endpoint_in_buffer);
    free(interface->endpoint_out_buffer);
    interface->endpoint_in_buffer = NULL;
    interface->endpoint_out_buffer = NULL;

    rwlockWriteUnlock(&interface->lock_out);
    rwlockWriteUnlock(&interface->lock_in);

    rwlockWriteUnlock(&interface->lock);
}

void usbCommsExitEx(u32 interface)
{
    u32 i;
    bool found=0;
    if (interface>=TOTAL_INTERFACES) return;

    _usbCommsInterfaceExit(&g_usbCommsInterfaces[interface]);

    for (i=0; i<TOTAL_INTERFACES; i++)
    {
        rwlockReadLock(&g_usbCommsInterfaces[i].lock);
        if (g_usbCommsInterfaces[i].initialized) found = 1;
        rwlockReadUnlock(&g_usbCommsInterfaces[i].lock);

        if (found) break;
    }

    if (!found) usbCommsExit();
}

void usbCommsExit(void)
{
    u32 i;

    rwlockWriteLock(&g_usbCommsLock);

    usbDsExit();

    g_usbCommsInitialized = false;

    rwlockWriteUnlock(&g_usbCommsLock);

    for (i=0; i<TOTAL_INTERFACES; i++)
    {
        _usbCommsInterfaceExit(&g_usbCommsInterfaces[i]);
    }
}

static Result _usbCommsInterfaceInit(usbCommsInterface *interface, u8 bInterfaceClass, u8 bInterfaceSubClass, u8 bInterfaceProtocol)
{
    Result rc=0;

    struct usb_interface_descriptor interface_descriptor = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = USBDS_DEFAULT_InterfaceNumber,
        .bInterfaceClass = bInterfaceClass,
        .bInterfaceSubClass = bInterfaceSubClass,
        .bInterfaceProtocol = bInterfaceProtocol,
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

    interface->initialized = 1;

    //The buffer for PostBufferAsync commands must be 0x1000-byte aligned.
    interface->endpoint_in_buffer = memalign(0x1000, 0x1000);
    if (interface->endpoint_in_buffer==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    if (R_SUCCEEDED(rc)) {
        interface->endpoint_out_buffer = memalign(0x1000, 0x1000);
        if (interface->endpoint_out_buffer==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }

    if (R_SUCCEEDED(rc)) {
        memset(interface->endpoint_in_buffer, 0, 0x1000);
        memset(interface->endpoint_out_buffer, 0, 0x1000);
    }

    if (R_FAILED(rc)) return rc;

    //Setup interface.
    rc = usbDsGetDsInterface(&interface->interface, &interface_descriptor, "usb");
    if (R_FAILED(rc)) return rc;

    //Setup endpoints.
    rc = usbDsInterface_GetDsEndpoint(interface->interface, &interface->endpoint_in, &endpoint_descriptor_in);//device->host
    if (R_FAILED(rc)) return rc;

    rc = usbDsInterface_GetDsEndpoint(interface->interface, &interface->endpoint_out, &endpoint_descriptor_out);//host->device
    if (R_FAILED(rc)) return rc;

    rc = usbDsInterface_EnableInterface(interface->interface);
    if (R_FAILED(rc)) return rc;

    return rc;
}

static Result _usbCommsRead(usbCommsInterface *interface, void* buffer, size_t size, size_t *transferredSize)
{
    Result rc=0;
    u32 urbId=0;
    u8 *bufptr = (u8*)buffer;
    u8 *transfer_buffer = NULL;
    u8 transfer_type=0;
    u32 chunksize=0;
    u32 tmp_transferredSize = 0;
    size_t total_transferredSize=0;
    UsbDsReportData reportdata;

    //Makes sure endpoints are ready for data-transfer / wait for init if needed.
    rc = usbDsWaitReady();
    if (R_FAILED(rc)) return rc;

    while(size)
    {
        if(((u64)bufptr) & 0xfff)//When bufptr isn't page-aligned copy the data into g_usbComms_endpoint_in_buffer and transfer that, otherwise use the bufptr directly.
        {
            transfer_buffer = interface->endpoint_out_buffer;
            memset(interface->endpoint_out_buffer, 0, 0x1000);

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
        rc = usbDsEndpoint_PostBufferAsync(interface->endpoint_out, transfer_buffer, chunksize, &urbId);
        if (R_FAILED(rc)) return rc;

        //Wait for the transfer to finish.
        svcWaitSynchronizationSingle(interface->endpoint_out->CompletionEvent, U64_MAX);
        svcClearEvent(interface->endpoint_out->CompletionEvent);

        rc = usbDsEndpoint_GetReportData(interface->endpoint_out, &reportdata);
        if (R_FAILED(rc)) return rc;

        rc = usbDsParseReportData(&reportdata, urbId, NULL, &tmp_transferredSize);
        if (R_FAILED(rc)) return rc;

        if (tmp_transferredSize > chunksize) tmp_transferredSize = chunksize;
        total_transferredSize+= (size_t)tmp_transferredSize;

        if (transfer_type==0) memcpy(bufptr, transfer_buffer, tmp_transferredSize);
        bufptr+= tmp_transferredSize;
        size-= tmp_transferredSize;

        if(tmp_transferredSize < chunksize)break;
    }

    if (transferredSize) *transferredSize = total_transferredSize;

    return rc;
}

static Result _usbCommsWrite(usbCommsInterface *interface, const void* buffer, size_t size, size_t *transferredSize)
{
    Result rc=0;
    u32 urbId=0;
    u32 chunksize=0;
    u8 *bufptr = (u8*)buffer;
    u8 *transfer_buffer = NULL;
    u32 tmp_transferredSize = 0;
    size_t total_transferredSize=0;
    UsbDsReportData reportdata;

    //Makes sure endpoints are ready for data-transfer / wait for init if needed.
    rc = usbDsWaitReady();
    if (R_FAILED(rc)) return rc;

    while(size)
    {
        if(((u64)bufptr) & 0xfff)//When bufptr isn't page-aligned copy the data into g_usbComms_endpoint_in_buffer and transfer that, otherwise use the bufptr directly.
        {
            transfer_buffer = interface->endpoint_in_buffer;
            memset(interface->endpoint_in_buffer, 0, 0x1000);

            chunksize = 0x1000;
            chunksize-= ((u64)bufptr) & 0xfff;//After this transfer, bufptr will be page-aligned(if size is large enough for another transfer).
            if (size<chunksize) chunksize = size;

            memcpy(interface->endpoint_in_buffer, bufptr, chunksize);
        }
        else
        {
            transfer_buffer = bufptr;
            chunksize = size;
        }

        //Start a device->host transfer.
        rc = usbDsEndpoint_PostBufferAsync(interface->endpoint_in, transfer_buffer, chunksize, &urbId);
        if(R_FAILED(rc))return rc;

        //Wait for the transfer to finish.
        svcWaitSynchronizationSingle(interface->endpoint_in->CompletionEvent, U64_MAX);
        svcClearEvent(interface->endpoint_in->CompletionEvent);

        rc = usbDsEndpoint_GetReportData(interface->endpoint_in, &reportdata);
        if (R_FAILED(rc)) return rc;

        rc = usbDsParseReportData(&reportdata, urbId, NULL, &tmp_transferredSize);
        if (R_FAILED(rc)) return rc;

        if (tmp_transferredSize > chunksize) tmp_transferredSize = chunksize;

        total_transferredSize+= (size_t)tmp_transferredSize;

        bufptr+= tmp_transferredSize;
        size-= tmp_transferredSize;

        if (tmp_transferredSize < chunksize) break;
    }

    if (transferredSize) *transferredSize = total_transferredSize;

    return rc;
}

size_t usbCommsReadEx(void* buffer, size_t size, u32 interface)
{
    size_t transferredSize=0;
    u32 state=0;
    Result rc, rc2;
    usbCommsInterface *inter = &g_usbCommsInterfaces[interface];
    bool initialized;

    if (interface>=TOTAL_INTERFACES) return 0;

    rwlockReadLock(&inter->lock);
    initialized = inter->initialized;
    rwlockReadUnlock(&inter->lock);
    if (!initialized) return 0;

    rwlockWriteLock(&inter->lock_out);
    rc = _usbCommsRead(inter, buffer, size, &transferredSize);
    rwlockWriteUnlock(&inter->lock_out);
    if (R_FAILED(rc)) {
        rc2 = usbDsGetState(&state);
        if (R_SUCCEEDED(rc2)) {
            if (state!=5) {
                rwlockWriteLock(&inter->lock_out);
                rc = _usbCommsRead(&g_usbCommsInterfaces[interface], buffer, size, &transferredSize); //If state changed during transfer, try again. usbDsWaitReady() will be called from this.
                rwlockWriteUnlock(&inter->lock_out);
            }
        }
        if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadUsbCommsRead));
    }
    return transferredSize;
}

size_t usbCommsRead(void* buffer, size_t size)
{
    return usbCommsReadEx(buffer, size, 0);
}

size_t usbCommsWriteEx(const void* buffer, size_t size, u32 interface)
{
    size_t transferredSize=0;
    u32 state=0;
    Result rc, rc2;
    usbCommsInterface *inter = &g_usbCommsInterfaces[interface];
    bool initialized;

    if (interface>=TOTAL_INTERFACES) return 0;

    rwlockReadLock(&inter->lock);
    initialized = inter->initialized;
    rwlockReadUnlock(&inter->lock);
    if (!initialized) return 0;

    rwlockWriteLock(&inter->lock_in);
    rc = _usbCommsWrite(&g_usbCommsInterfaces[interface], buffer, size, &transferredSize);
    rwlockWriteUnlock(&inter->lock_in);
    if (R_FAILED(rc)) {
        rc2 = usbDsGetState(&state);
        if (R_SUCCEEDED(rc2)) {
            if (state!=5) {
                rwlockWriteLock(&inter->lock_in);
                rc = _usbCommsWrite(&g_usbCommsInterfaces[interface], buffer, size, &transferredSize); //If state changed during transfer, try again. usbDsWaitReady() will be called from this.
                rwlockWriteUnlock(&inter->lock_in);
            }
        }
        if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadUsbCommsWrite));
    }
    return transferredSize;
}

size_t usbCommsWrite(const void* buffer, size_t size)
{
    return usbCommsWriteEx(buffer, size, 0);
}

