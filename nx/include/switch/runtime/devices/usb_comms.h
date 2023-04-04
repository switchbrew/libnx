/**
 * @file usb_comms.h
 * @brief USB comms.
 * @author yellows8
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../../types.h"

typedef struct {
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
} UsbCommsInterfaceInfo;

/// Initializes usbComms with the default number of interfaces (1)
Result usbCommsInitialize(void);

/// Initializes usbComms with a specific number of interfaces.
Result usbCommsInitializeEx(u32 num_interfaces, const UsbCommsInterfaceInfo *infos, u16 idVendor, u16 idProduct);

/// Exits usbComms.
void usbCommsExit(void);

/// Sets whether to throw a fatal error in usbComms{Read/Write}* on failure, or just return the transferred size. By default (false) the latter is used.
void usbCommsSetErrorHandling(bool flag);

///@name Synchronous API
///@{

/// Read data with the default interface.
size_t usbCommsRead(void* buffer, size_t size);

/// Write data with the default interface.
size_t usbCommsWrite(const void* buffer, size_t size);

/// Same as usbCommsRead except with the specified interface.
size_t usbCommsReadEx(void* buffer, size_t size, u32 interface);

/// Same as usbCommsWrite except with the specified interface.
size_t usbCommsWriteEx(const void* buffer, size_t size, u32 interface);

///@}

///@name Asynchronous API
///@{

/// Retrieve event used for read completion with the given interface.
Event *usbCommsGetReadCompletionEvent(u32 interface);

/// Start an asynchronous read. The completion event will be signaled when the read completes.
/// The buffer must be page-aligned and no larger than one page.
Result usbCommsReadAsync(void *buffer, size_t size, u32 *urbId, u32 interface);

/// Complete an asynchronous read, clearing the completion event, and return the amount of data which was read.
Result usbCommsGetReadResult(u32 urbId, u32 *transferredSize, u32 interface);


/// Retrieve event used for write completion with the given interface.
Event *usbCommsGetWriteCompletionEvent(u32 interface);

/// Start an asynchronous write. The completion event will be signaled when the write completes.
/// The buffer must be page-aligned and no larger than one page.
Result usbCommsWriteAsync(void *buffer, size_t size, u32 *urbId, u32 interface);

/// Complete an asynchronous write, clearing the completion event, and return the amount of data which was written.
Result usbCommsGetWriteResult(u32 urbId, u32 *transferredSize, u32 interface);

///@}
