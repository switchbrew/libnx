/**
 * @file usbhs.h
 * @brief USB (usb:hs) devices service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/usb.h"
#include "../kernel/event.h"
#include "../kernel/tmem.h"

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
/// The INPUT/OUTPUT endpoint descriptors were swapped with [8.0.0+], libnx converts this struct to the newer layout when running on pre-8.0.0.
typedef struct {
    s32 ID;
    u32 deviceID_2;
    u32 unk_x8;

    struct usb_interface_descriptor interface_desc;
    u8 pad_x15[0x7];
    struct usb_endpoint_descriptor input_endpoint_descs[15];
    u8 pad_x85[0x7];
    struct usb_endpoint_descriptor output_endpoint_descs[15];
    u8 pad_xf5[0x6];
    struct usb_ss_endpoint_companion_descriptor input_ss_endpoint_companion_descs[15];  ///< ?
    u8 pad_x155[0x6];
    struct usb_ss_endpoint_companion_descriptor output_ss_endpoint_companion_descs[15]; ///< ?
    u8 pad_x1b5[0x3];
} NX_PACKED UsbHsInterfaceInfo;

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
} NX_PACKED UsbHsInterface;

typedef struct {
    u32 xferId;
    Result res;
    u32 requestedSize;
    u32 transferredSize;
    u64 id;                  ///< id from \ref usbHsEpPostBufferAsync.
} UsbHsXferReport;

typedef struct {
    vu64 write_index;
    vu64 read_index;
} UsbHsRingHeader;

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
    u32 maxUrbCount;
    u64 max_reports;
    void* ringbuf;

    struct usb_endpoint_descriptor desc;
} UsbHsClientEpSession;

/// Initialize usb:hs.
Result usbHsInitialize(void);

/// Exit usb:hs.
void usbHsExit(void);

/// Gets the Service object for the actual usb:hs service session.
Service* usbHsGetServiceSession(void);

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
 * @param[out] out_event Event object.
 * @param[in] autoclear Event autoclear.
 * @param[in] index Event index, must be 0..2.
 * @param[in] filter \ref UsbHsInterfaceFilter.
 */
Result usbHsCreateInterfaceAvailableEvent(Event* out_event, bool autoclear, u8 index, const UsbHsInterfaceFilter* filter);

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

/// On [1.0.0] this is stubbed, just returns 0 with out=0.
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

/// Gets the Xfer Event which is signaled when PostBufferAsync finishes. This is only valid for [2.0.0+]. If using \ref eventWait with this, then \ref eventClear should be used if the event was signaled (since the autoclear is false).
NX_CONSTEXPR Event* usbHsEpGetXferEvent(UsbHsClientEpSession* s) {
    return &s->eventXfer;
}

/// Gets the buffer size to use with \ref usbHsEpShareReportRing.
NX_CONSTEXPR u32 usbHsEpGetReportRingSize(UsbHsClientEpSession* s) {
    u64 max_reports = s->maxUrbCount * 0x21;
    u32 size = sizeof(UsbHsRingHeader) + max_reports*sizeof(UsbHsXferReport);
    size = (size+0xFFF) & ~0xFFF;
    return size;
}

/**
 * @brief Starts an async data transfer with the specified endpoint. The Event from \ref usbHsEpGetXferEvent can be used to determine when the transfer finished. If you don't need async, \ref usbHsEpPostBuffer can be used instead.
 * @note Only available on [2.0.0+].
 * @param[in] s The endpoint object.
 * @param buffer Data buffer. The buffer address and size should be aligned to 0x1000-bytes.
 * @param[in] size The actual data size.
 * @param[in] id This is an arbitrary value which will be later returned in \ref UsbHsXferReport. For example a value starting at 0 can be used, then if sending multiple requests at once this value can be incremented each time (with 0 for the first request in this set of requests).
 * @param[out] xferId Output xferId.
 */
Result usbHsEpPostBufferAsync(UsbHsClientEpSession* s, void* buffer, u32 size, u64 id, u32* xferId);

/**
 * @brief Gets an array of \ref UsbHsXferReport for the specified endpoint. This should be used after waiting on the Event from \ref usbHsEpGetXferEvent.
 * @note Only available on [2.0.0+].
 * @param[in] s The endpoint object.
 * @param[out] reports Output array of \ref UsbHsXferReport.
 * @param[in] max_reports Size of the reports array in entries.
 * @param[out] count Number of entries written to the array.
 */
Result usbHsEpGetXferReport(UsbHsClientEpSession* s, UsbHsXferReport* reports, u32 max_reports, u32* count);

/**
 * @brief Uses a data transfer with the specified endpoint, this will block until the transfer finishes. This wraps \ref usbHsEpPostBufferAsync and \ref usbHsEpGetXferReport, and also handles the Event (on pre-2.0.0 this handles using the relevant cmds instead). If async is needed, use \ref usbHsEpPostBufferAsync instead.
 * @param[in] s The endpoint object.
 * @param buffer Data buffer. The buffer address and size should be aligned to 0x1000-bytes.
 * @param[in] size The actual data size.
 * @param[out] transferredSize Output transferred size.
 */
Result usbHsEpPostBuffer(UsbHsClientEpSession* s, void* buffer, u32 size, u32* transferredSize);

/**
 * @brief This uses the same functionality internally as \ref usbHsEpPostBufferAsync except the urbs array and unk1/unk2 are specified by the user instead.
 * @note Only available on [2.0.0+].
 * @param[in] s The endpoint object.
 * @param buffer Data buffer. The buffer address and size should be aligned to 0x1000-bytes.
 * @param[in] urbs Input array of u32s for the size of each urb.
 * @param[in] urbCount Total entries in the urbs array.
 * @param[in] id Same as \ref usbHsEpPostBufferAsync.
 * @param[in] unk1 \ref usbHsEpPostBufferAsync would internally pass value 0 here.
 * @param[in] unk2 \ref usbHsEpPostBufferAsync would internally pass value 0 here.
 * @param[out] xferId Output xferId.
 */
Result usbHsEpBatchBufferAsync(UsbHsClientEpSession* s, void* buffer, u32* urbs, u32 urbCount, u64 id, u32 unk1, u32 unk2, u32* xferId);

/**
 * @brief This can be used to map the specified buffer as devicemem, which can then be used with \ref usbHsEpPostBufferAsync / \ref usbHsEpPostBuffer / \ref usbHsEpBatchBufferAsync. If the buffer address passed to those funcs is within this SmmuSpace, the specified buffer must be within the bounds of the SmmuSpace buffer.
 * @note Only available on [4.0.0+].
 * @note A buffer from usbHsEpCreateSmmuSpace can't be reused by another endpoint with the aforementioned funcs.
 * @note This can only be used once per UsbHsClientEpSession object.
 * @param[in] s The endpoint object.
 * @param buffer Buffer address, this must be aligned to 0x1000-bytes.
 * @param[in] size Buffer size, this must be aligned to 0x1000-bytes.
 */
Result usbHsEpCreateSmmuSpace(UsbHsClientEpSession* s, void* buffer, u32 size);

/**
 * @brief This creates TransferMemory which is used to read \ref UsbHsXferReport when \ref usbHsEpGetXferReport is used, instead of using the service cmd.
 * @note Only available on [4.0.0+].
 * @param buffer Buffer, must be 0x1000-byte aligned.
 * @param[in] size Buffer size, \ref usbHsEpGetReportRingSize can be used to calculate this.
 */
Result usbHsEpShareReportRing(UsbHsClientEpSession* s, void* buffer, size_t size);

