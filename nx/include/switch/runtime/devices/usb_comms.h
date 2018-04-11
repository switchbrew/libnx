/**
 * @file usb_comms.h
 * @brief USB comms.
 * @author yellows8
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../../types.h"

Result usbCommsInitialize(void);
void usbCommsExit(void);

/// Same as usbCommsInitialize, except this can be used after usbCommsInitialize (or instead of usbCommsInitialize), for creating new interface(s).
/// bInterface* are the values for the same fields in usb.h \ref usb_interface_descriptor. \ref usbCommsInitialize uses USB_CLASS_VENDOR_SPEC for all of these internally.
Result usbCommsInitializeEx(u32 *interface, u8 bInterfaceClass, u8 bInterfaceSubClass, u8 bInterfaceProtocol);

/// Shutdown the specified interface. If no interfaces are remaining, this then uses \ref usbCommsExit internally.
void usbCommsExitEx(u32 interface);

/// Read data with the default interface.
size_t usbCommsRead(void* buffer, size_t size);

/// Write data with the default interface.
size_t usbCommsWrite(const void* buffer, size_t size);

/// Same as usbCommsRead except with the specified interface.
size_t usbCommsReadEx(void* buffer, size_t size, u32 interface);

/// Same as usbCommsWrite except with the specified interface.
size_t usbCommsWriteEx(const void* buffer, size_t size, u32 interface);
