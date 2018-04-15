/**
 * @file usb.h
 * @brief USB (usb:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"

/// usb:ds Switch-as-device<>host USB comms, see also here: http://switchbrew.org/index.php?title=USB_services

/// Names starting with "libusb" were changed to "usb" to avoid collision with actual libusb if it's ever used.

#define USBDS_DEFAULT_InterfaceNumber 0x4 ///Value for usb_interface_descriptor bInterfaceNumber for automatically allocating the actual bInterfaceNumber.

/// Imported from libusb with changed names.
/* Descriptor sizes per descriptor type */
#define USB_DT_INTERFACE_SIZE        9
#define USB_DT_ENDPOINT_SIZE         7

/// Imported from libusb, with some adjustments.
struct usb_endpoint_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /// Must match USB_DT_ENDPOINT.
    uint8_t  bEndpointAddress; /// Should be one of the usb_endpoint_direction values, the endpoint-number is automatically allocated.
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
};

/// Imported from libusb, with some adjustments.
struct usb_interface_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType; /// Must match USB_DT_INTERFACE.
    uint8_t  bInterfaceNumber; /// See also USBDS_DEFAULT_InterfaceNumber.
    uint8_t  bAlternateSetting; /// Must match 0.
    uint8_t  bNumEndpoints; /// Ignored.
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface; /// Ignored.
};

typedef struct {
    u16 idVendor; /// VID
    u16 idProduct; /// PID
    u16 bcdDevice;
    char Manufacturer[0x20];
    char Product[0x20];
    char SerialNumber[0x20];
} UsbDsDeviceInfo;

typedef struct {
    u32 id; /// urbId from post-buffer cmds
    u32 requestedSize;
    u32 transferredSize;
    u32 urb_status;
} UsbDsReportEntry;

typedef struct {
    UsbDsReportEntry report[8];
    u32 report_count;
} UsbDsReportData;

typedef struct {
    bool initialized;
    u32 interface_index;
    Service  h;

    Handle SetupEvent;
    Handle CtrlInCompletionEvent;
    Handle CtrlOutCompletionEvent;
} UsbDsInterface;

typedef struct {
    bool initialized;
    Service h;
    Handle CompletionEvent;
} UsbDsEndpoint;

typedef enum {
    UsbComplexId_Default = 0x2
} UsbComplexId;

/// Imported from libusb, with changed names.
enum usb_class_code {
    USB_CLASS_PER_INTERFACE = 0,
    USB_CLASS_AUDIO = 1,
    USB_CLASS_COMM = 2,
    USB_CLASS_HID = 3,
    USB_CLASS_PHYSICAL = 5,
    USB_CLASS_PRINTER = 7,
    USB_CLASS_PTP = 6, /* legacy name from libusb-0.1 usb.h */
    USB_CLASS_IMAGE = 6,
    USB_CLASS_MASS_STORAGE = 8,
    USB_CLASS_HUB = 9,
    USB_CLASS_DATA = 10,
    USB_CLASS_SMART_CARD = 0x0b,
    USB_CLASS_CONTENT_SECURITY = 0x0d,
    USB_CLASS_VIDEO = 0x0e,
    USB_CLASS_PERSONAL_HEALTHCARE = 0x0f,
    USB_CLASS_DIAGNOSTIC_DEVICE = 0xdc,
    USB_CLASS_WIRELESS = 0xe0,
    USB_CLASS_APPLICATION = 0xfe,
    USB_CLASS_VENDOR_SPEC = 0xff
};

/// Imported from libusb, with changed names.
enum usb_descriptor_type {
    USB_DT_DEVICE = 0x01,
    USB_DT_CONFIG = 0x02,
    USB_DT_STRING = 0x03,
    USB_DT_INTERFACE = 0x04,
    USB_DT_ENDPOINT = 0x05,
    USB_DT_BOS = 0x0f,
    USB_DT_DEVICE_CAPABILITY = 0x10,
    USB_DT_HID = 0x21,
    USB_DT_REPORT = 0x22,
    USB_DT_PHYSICAL = 0x23,
    USB_DT_HUB = 0x29,
    USB_DT_SUPERSPEED_HUB = 0x2a,
    USB_DT_SS_ENDPOINT_COMPANION = 0x30
};

/// Imported from libusb, with changed names.
enum usb_endpoint_direction {
    USB_ENDPOINT_IN = 0x80,
    USB_ENDPOINT_OUT = 0x00
};

/// Imported from libusb, with changed names.
enum usb_transfer_type {
    USB_TRANSFER_TYPE_CONTROL = 0,
    USB_TRANSFER_TYPE_ISOCHRONOUS = 1,
    USB_TRANSFER_TYPE_BULK = 2,
    USB_TRANSFER_TYPE_INTERRUPT = 3,
    USB_TRANSFER_TYPE_BULK_STREAM = 4,
};

/// Imported from libusb, with changed names.
enum usb_iso_sync_type {
    USB_ISO_SYNC_TYPE_NONE = 0,
    USB_ISO_SYNC_TYPE_ASYNC = 1,
    USB_ISO_SYNC_TYPE_ADAPTIVE = 2,
    USB_ISO_SYNC_TYPE_SYNC = 3
};

/// Imported from libusb, with changed names.
enum usb_iso_usage_type {
    USB_ISO_USAGE_TYPE_DATA = 0,
    USB_ISO_USAGE_TYPE_FEEDBACK = 1,
    USB_ISO_USAGE_TYPE_IMPLICIT = 2,
};

Result usbDsInitialize(UsbComplexId complexId, const UsbDsDeviceInfo* deviceinfo);

/// Exit usbDs. Any interfaces/endpoints which are left open are automatically closed, since otherwise usb-sysmodule won't fully reset usbds to defaults.
void usbDsExit(void);

Service* usbDsGetServiceSession(void);
Handle usbDsGetStateChangeEvent(void);

Result usbDsGetState(u32 *out);
Result usbDsGetDsInterface(UsbDsInterface** interface, struct usb_interface_descriptor* descriptor, const char *interface_name);

/// Wait for initialization to finish where data-transfer is usable.
Result usbDsWaitReady(void);

/// Parse usbDsReportData from the Get*ReportData commands, where urbId is from the post-buffer commands. Will return the converted urb_status result-value.
Result usbDsParseReportData(UsbDsReportData *reportdata, u32 urbId, u32 *requestedSize, u32 *transferredSize);

/// IDsInterface
void usbDsInterface_Close(UsbDsInterface* interface);
Result usbDsInterface_GetDsEndpoint(UsbDsInterface* interface, UsbDsEndpoint** endpoint, struct usb_endpoint_descriptor* descriptor);
Result usbDsInterface_EnableInterface(UsbDsInterface* interface);
Result usbDsInterface_DisableInterface(UsbDsInterface* interface);
Result usbDsInterface_CtrlInPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32 *urbId);
Result usbDsInterface_CtrlOutPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32 *urbId);
Result usbDsInterface_GetCtrlInReportData(UsbDsInterface* interface, UsbDsReportData *out);
Result usbDsInterface_GetCtrlOutReportData(UsbDsInterface* interface, UsbDsReportData *out);
Result usbDsInterface_StallCtrl(UsbDsInterface* interface);

/// IDsEndpoint

void usbDsEndpoint_Close(UsbDsEndpoint* endpoint);
Result usbDsEndpoint_PostBufferAsync(UsbDsEndpoint* endpoint, void* buffer, size_t size, u32 *urbId);
Result usbDsEndpoint_GetReportData(UsbDsEndpoint* endpoint, UsbDsReportData *out);
Result usbDsEndpoint_StallCtrl(UsbDsEndpoint* endpoint);

