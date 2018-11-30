/**
 * @file usb.h
 * @brief USB (usb:hs) devices service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/usb.h"
#include "../kernel/event.h"

typedef enum {
    ///< These use \ref usb_device_descriptor. Bit2..6 require [6.0.0+], these are ignored on eariler versions.
    UsbHsInterfaceFilterFlags_idVendor           = BIT(0),
    UsbHsInterfaceFilterFlags_idProduct          = BIT(1),
    UsbHsInterfaceFilterFlags_bcdDevice_Min      = BIT(2),
    UsbHsInterfaceFilterFlags_bcdDevice_Max      = BIT(3),
    UsbHsInterfaceFilterFlags_bDeviceClass       = BIT(4),
    UsbHsInterfaceFilterFlags_bDeviceSubClass    = BIT(5),
    UsbHsInterfaceFilterFlags_bDeviceProtocol    = BIT(6),

    ///< These use \ref usb_interface_descriptor.
    UsbHsInterfaceFilterFlags_bInterfaceClass    = BIT(7),
    UsbHsInterfaceFilterFlags_bInterfaceSubClass = BIT(8),
    UsbHsInterfaceFilterFlags_bInterfaceProtocol = BIT(9),
} UsbHsInterfaceFilterFlags;

/// Interface filtering struct. When the associated flag bit is set, the associated descriptor field and struct field are compared, on mismatch the interface is filtered out.
typedef struct {
    u16 Flags;              ///< See \ref UsbHsInterfaceFilterFlags. Setting this to 0 is equivalent to disabling filtering.
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice_Min;      ///< Descriptor value must be >= bcdDevice_Min.
    u16 bcdDevice_Max;      ///< Descriptor value must be <= bcdDevice_Max.
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
} UsbHsInterfaceFilter;

/// Descriptors which are not available are set to all-zero.
typedef struct {
    s32 ID;
    u32 deviceID_2;
    u32 unk_x8;

    struct usb_interface_descriptor interface_desc;
    u8 pad_x15[0x7];
    struct usb_endpoint_descriptor output_endpoint_descs[15];
    u8 pad_x85[0x7];
    struct usb_endpoint_descriptor input_endpoint_descs[15];
    u8 pad_xf5[0x6];
    struct usb_ss_endpoint_companion_descriptor output_ss_endpoint_companion_descs[15];  ///< ?
    u8 pad_x155[0x6];
    struct usb_ss_endpoint_companion_descriptor input_ss_endpoint_companion_descs[15];   ///< ?
    u8 pad_x1b5[0x3];
} PACKED UsbHsInterfaceInfo;

/// Interface struct. Note that devices have a seperate \ref UsbHsInterface for each interface.
typedef struct {
    UsbHsInterfaceInfo inf;

    char pathstr[0x40];
    u32 busID;
    u32 deviceID;

    struct usb_device_descriptor device_desc;
    struct usb_config_descriptor config_desc;
    u8 pad_x21b[0x5];

    u64 timestamp; ///< Unknown u64 timestamp for when the device was inserted?
} PACKED UsbHsInterface;

typedef struct {
    u32 unk_x0;
    Result res;
    u32 unk_x8;
    u32 transferredSize;
    u64 unk_x10;
} UsbHsXferReport;

/// The interface service object. These Events have autoclear=false.
typedef struct {
    Service s;
    Event event0;         ///< Unknown.
    Event eventCtrlXfer;  ///< [2.0.0+] Signaled when CtrlXferAsync finishes.

    UsbHsInterface inf;   ///< Initialized with the input interface from \ref usbHsAcquireUsbIf, then the first 0x1B8-bytes are overwritten with the cmd output (data before pathstr).
} UsbHsClientIfSession;

typedef struct {
    Service s;

    struct usb_endpoint_descriptor desc;
} UsbHsClientEpSession;

/// Initialize/exit usb:hs.
Result usbHsInitialize(void);
void usbHsExit(void);

/// Returns the Event loaded during init with autoclear=false.
Event* usbHsGetInterfaceStateChangeEvent(void);

/**
 * @brief Returns an array of all \ref UsbHsInterface. Internally this loads the same interfaces as \ref usbHsQueryAvailableInterfaces, followed by \ref usbHsQueryAcquiredInterfaces.
 * @param[in] filter \ref UsbHsInterfaceFilter.
 * @param[out] interfaces Array of output interfaces.
 * @param[in] interfaces_maxsize Max byte-size of the interfaces buffer.
 * @param[out] total_entries Total number of output interfaces.
 */
Result usbHsQueryAllInterfaces(const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries);

/**
 * @brief Returns an array of \ref UsbHsInterface which are available.
 * @param[in] filter \ref UsbHsInterfaceFilter.
 * @param[out] interfaces Array of output interfaces.
 * @param[in] interfaces_maxsize Max byte-size of the interfaces buffer.
 * @param[out] total_entries Total number of output interfaces.
 */
Result usbHsQueryAvailableInterfaces(const UsbHsInterfaceFilter* filter, UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries);

/**
 * @brief Returns an array of \ref UsbHsInterface which were previously acquired.
 * @param[out] interfaces Array of output interfaces.
 * @param[in] interfaces_maxsize Max byte-size of the interfaces buffer.
 * @param[out] total_entries Total number of output interfaces.
 */
Result usbHsQueryAcquiredInterfaces(UsbHsInterface* interfaces, size_t interfaces_maxsize, s32* total_entries);

/**
 * @brief Creates an event which is signaled when an interface is available which passes the filtering checks.
 * @param[out] event Event object.
 * @param[in] autoclear Event autoclear.
 * @param[in] index Event index, must be 0..2.
 * @param[in] filter \ref UsbHsInterfaceFilter.
 */
Result usbHsCreateInterfaceAvailableEvent(Event* event, bool autoclear, u8 index, const UsbHsInterfaceFilter* filter);

/**
 * @brief Destroys an event setup by \ref usbHsCreateInterfaceAvailableEvent. This *must* be used at some point during cleanup.
 * @param[in] event Event object to close.
 * @param[in] index Event index, must be 0..2.
 */
Result usbHsDestroyInterfaceAvailableEvent(Event* event, u8 index);

/**
 * @brief Acquires/opens the specified interface.
 * @param[in] s The service object.
 * @param[in] interface Interface to use.
 */
Result usbHsAcquireUsbIf(UsbHsClientIfSession* s, UsbHsInterface *interface);

/// UsbHsClientIfSession

/// Closes the specified interface session.
void usbHsIfClose(UsbHsClientIfSession* s);

