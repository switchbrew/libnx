/**
 * @file usbds.h
 * @brief USB (usb:ds) service IPC wrapper.
 * @brief Switch-as-device<>host USB comms, see also here: https://switchbrew.org/wiki/USB_services
 * @author SciresM, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/usb.h"
#include "../kernel/event.h"

#define USBDS_DEFAULT_InterfaceNumber 0x4 ///Value for usb_interface_descriptor bInterfaceNumber for automatically allocating the actual bInterfaceNumber.

typedef struct {
    u16 idVendor; ///< VID
    u16 idProduct; ///< PID
    u16 bcdDevice;
    char Manufacturer[0x20];
    char Product[0x20];
    char SerialNumber[0x20];
} UsbDsDeviceInfo;

typedef struct {
    u32 id; ///< urbId from post-buffer cmds
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

    Event SetupEvent;
    Event CtrlInCompletionEvent;
    Event CtrlOutCompletionEvent;
} UsbDsInterface;

typedef struct {
    bool initialized;
    Service h;
    Event CompletionEvent;
} UsbDsEndpoint;

typedef enum {
    UsbComplexId_Default = 0x2
} UsbComplexId;

typedef enum {
    UsbDeviceSpeed_Full = 0x2,  ///< USB 1.1 Full Speed
    UsbDeviceSpeed_High = 0x3,  ///< USB 2.0 High Speed
    UsbDeviceSpeed_Super = 0x4, ///< USB 3.0 Super Speed
} UsbDeviceSpeed;

/// Opens a session with usb:ds.
Result usbDsInitialize(void);
/// Closes the usb:ds session. Any interfaces/endpoints which are left open are automatically closed, since otherwise usb-sysmodule won't fully reset usb:ds to defaults.
void usbDsExit(void);

/// Helpers
Result usbDsWaitReady(u64 timeout);
Result usbDsParseReportData(UsbDsReportData *reportdata, u32 urbId, u32 *requestedSize, u32 *transferredSize);

/// IDsService
Event* usbDsGetStateChangeEvent(void);
Result usbDsGetState(u32* out);

/// Removed in 5.0.0
Result usbDsGetDsInterface(UsbDsInterface** out, struct usb_interface_descriptor* descriptor, const char* interface_name);
Result usbDsSetVidPidBcd(const UsbDsDeviceInfo* deviceinfo);

/// Added in 5.0.0
Result usbDsRegisterInterface(UsbDsInterface** out);
Result usbDsRegisterInterfaceEx(UsbDsInterface** out, u32 intf_num);
Result usbDsClearDeviceData(void);
Result usbDsAddUsbStringDescriptor(u8* out_index, const char* string);
Result usbDsAddUsbLanguageStringDescriptor(u8* out_index, const u16* lang_ids, u16 num_langs);
Result usbDsDeleteUsbStringDescriptor(u8 index);
Result usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed speed, struct usb_device_descriptor* descriptor);
Result usbDsSetBinaryObjectStore(const void* bos, size_t bos_size);
Result usbDsEnable(void);
Result usbDsDisable(void);

/// IDsInterface
void usbDsInterface_Close(UsbDsInterface* interface);

Result usbDsInterface_GetSetupPacket(UsbDsInterface* interface, void* buffer, size_t size);
Result usbDsInterface_EnableInterface(UsbDsInterface* interface);
Result usbDsInterface_DisableInterface(UsbDsInterface* interface);
Result usbDsInterface_CtrlInPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32* urbId);
Result usbDsInterface_CtrlOutPostBufferAsync(UsbDsInterface* interface, void* buffer, size_t size, u32* urbId);
Result usbDsInterface_GetCtrlInReportData(UsbDsInterface* interface, UsbDsReportData* out);
Result usbDsInterface_GetCtrlOutReportData(UsbDsInterface* interface, UsbDsReportData* out);
Result usbDsInterface_StallCtrl(UsbDsInterface* interface);

/// Removed in 5.0.0
Result usbDsInterface_GetDsEndpoint(UsbDsInterface* interface, UsbDsEndpoint** endpoint, struct usb_endpoint_descriptor* descriptor);

/// Added in 5.0.0
Result usbDsInterface_RegisterEndpoint(UsbDsInterface* interface, UsbDsEndpoint** endpoint, u8 endpoint_address);
Result usbDsInterface_AppendConfigurationData(UsbDsInterface* interface, UsbDeviceSpeed speed, const void* buffer, size_t size);


/// IDsEndpoint
void usbDsEndpoint_Close(UsbDsEndpoint* endpoint);

Result usbDsEndpoint_Cancel(UsbDsEndpoint* endpoint);
Result usbDsEndpoint_PostBufferAsync(UsbDsEndpoint* endpoint, void* buffer, size_t size, u32* urbId);
Result usbDsEndpoint_GetReportData(UsbDsEndpoint* endpoint, UsbDsReportData* out);
Result usbDsEndpoint_Stall(UsbDsEndpoint* endpoint);
Result usbDsEndpoint_SetZlt(UsbDsEndpoint* endpoint, bool zlt); // Sets Zero Length Termination for endpoint

