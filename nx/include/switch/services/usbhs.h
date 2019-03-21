/**
 * @file usbhs.h
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
/// [7.0.0+]: The filter struct has to be unique, it can't be used by anything else (including other processes). Hence, Flags has to be non-zero. When initialized with usb:hs:a and VID and/or PID filtering is enabled, the VID/PID will be checked against a blacklist.
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
    u32 xferId;
    Result res;
    u32 requestedSize;
    u32 transferredSize;
    u64 unk_x10;
} UsbHsXferReport;

/// The interface service object. These Events have autoclear=false.
typedef struct {
    Service s;
    Event event0;         ///< Unknown.
    Event eventCtrlXfer;  ///< [2.0.0+] Signaled when CtrlXferAsync finishes.
    s32 ID;

    UsbHsInterface inf;   ///< Initialized with the input interface from \ref usbHsAcquireUsbIf, then overwritten with the cmd output. Pre-3.0.0 this only overwrites the first 0x1B8-bytes (data before pathstr).
} UsbHsClientIfSession;

typedef struct {
    Service s;
    Event eventXfer;                      ///< [2.0.0+] Signaled when PostBufferAsync finishes.

    struct usb_endpoint_descriptor desc;
} UsbHsClientEpSession;

/// Initialize/exit usb:hs.
Result usbHsInitialize(void);
void usbHsExit(void);

/// Returns the Event loaded during init with autoclear=false.
/// Signaled when a device was removed.
/// When signaled, the user should use \ref usbHsQueryAcquiredInterfaces and cleanup state for all interfaces which are not listed in the output interfaces (none of the IDs match \ref usbHsIfGetID output).
Event* usbHsGetInterfaceStateChangeEvent(void);

/**
 * @brief Returns an array of all \ref UsbHsInterface. Internally this loads the same interfaces as \ref usbHsQueryAvailableInterfaces, followed by \ref usbHsQueryAcquiredInterfaces. However, ID in \ref UsbHsInterface is set to -1, hence the output from this should not be used with \ref usbHsAcquireUsbIf.
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
 * @brief Acquires/opens the specified interface. This returns an error if the interface was already acquired by another process.
 * @param[in] s The service object.
 * @param[in] interface Interface to use.
 */
Result usbHsAcquireUsbIf(UsbHsClientIfSession* s, UsbHsInterface *interface);

/// UsbHsClientIfSession

/// Closes the specified interface session.
void usbHsIfClose(UsbHsClientIfSession* s);

/// Returns whether the specified interface session was initialized.
static inline bool usbHsIfIsActive(UsbHsClientIfSession* s) {
    return serviceIsActive(&s->s);
}

/// Returns the ID which can be used for comparing with the ID in the output interfaces from \ref usbHsQueryAcquiredInterfaces.
static inline s32 usbHsIfGetID(UsbHsClientIfSession* s) {
    return s->ID;
}

/**
 * @brief Selects an interface.
 * @param[in] s The service object.
 * @param[out] inf The output interface info. If NULL, the output is stored within s instead.
 * @param[in] id ID
 */
Result usbHsIfSetInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id);

/**
 * @brief Gets an interface.
 * @param[in] s The service object.
 * @param[out] inf The output interface info. If NULL, the output is stored within s instead.
 */
Result usbHsIfGetInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf);

/**
 * @brief Gets an alternate interface.
 * @param[in] s The service object.
 * @param[out] inf The output interface info. If NULL, the output is stored within s instead.
 * @param[in] id ID
 */
Result usbHsIfGetAlternateInterface(UsbHsClientIfSession* s, UsbHsInterfaceInfo* inf, u8 id);

/// On 1.0.0 this is stubbed, just returns 0 with out=0.
Result usbHsIfGetCurrentFrame(UsbHsClientIfSession* s, u32* out);

/// Uses a control transfer, this will block until the transfer finishes. The buffer address and size should be aligned to 0x1000-bytes, where wLength is the original size.
Result usbHsIfCtrlXfer(UsbHsClientIfSession* s, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, void* buffer, u32* transferredSize);

/**
 * @brief Opens an endpoint. maxUrbCount*maxXferSize must be non-zero.
 * @param[in] s The interface object.
 * @param[out] ep The endpoint object.
 * @param[in] maxUrbCount maxUrbCount, must be <0x11.
 * @param[in] maxXferSize Max transfer size for a packet. This can be desc->wMaxPacketSize. Must be <=0xFF0000.
 * @param[in] desc Endpoint descriptor.
 */
Result usbHsIfOpenUsbEp(UsbHsClientIfSession* s, UsbHsClientEpSession* ep, u16 maxUrbCount, u32 maxXferSize, struct usb_endpoint_descriptor *desc);

/// Resets the device: has the same affect as unplugging the device and plugging it back in.
Result usbHsIfResetDevice(UsbHsClientIfSession* s);

/// UsbHsClientEpSession

/// Closes the specified endpoint session.
void usbHsEpClose(UsbHsClientEpSession* s);

/// Uses a data transfer with the specified endpoint, this will block until the transfer finishes. The buffer address and size should be aligned to 0x1000-bytes, where the input size is the original size.
Result usbHsEpPostBuffer(UsbHsClientEpSession* s, void* buffer, u32 size, u32* transferredSize);

