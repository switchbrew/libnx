/**
 * @file usb.h
 * @brief Common USB (usb:*) service IPC header.
 * @author SciresM, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../kernel/event.h"

/// Names starting with "libusb" were changed to "usb" to avoid collision with actual libusb if it's ever used.

/// Imported from libusb with changed names.
/* Descriptor sizes per descriptor type */
#define USB_DT_INTERFACE_SIZE        9
#define USB_DT_ENDPOINT_SIZE         7
#define USB_DT_DEVICE_SIZE         0x12
#define USB_DT_SS_ENDPOINT_COMPANION_SIZE 6

#define USB_ENDPOINT_ADDRESS_MASK 0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK 0x80

#define USB_TRANSFER_TYPE_MASK 0x03 /* in bmAttributes */

/// Imported from libusb, with some adjustments.
struct usb_endpoint_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType; ///< Must match USB_DT_ENDPOINT.
    uint8_t  bEndpointAddress; ///< Should be one of the usb_endpoint_direction values, the endpoint-number is automatically allocated.
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} PACKED;

/// Imported from libusb, with some adjustments.
struct usb_interface_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType; ///< Must match USB_DT_INTERFACE.
    uint8_t  bInterfaceNumber; ///< See also USBDS_DEFAULT_InterfaceNumber.
    uint8_t  bAlternateSetting; ///< Must match 0.
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface; ///< Ignored.
};

/// Imported from libusb, with some adjustments.
struct usb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType; ///< Must match USB_DT_Device.
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
};

/// Imported from libusb, with some adjustments.
struct usb_config_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  MaxPower;
} PACKED;

/// Imported from libusb, with some adjustments.
struct usb_ss_endpoint_companion_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType; ///< Must match USB_DT_SS_ENDPOINT_COMPANION.
    uint8_t  bMaxBurst;
    uint8_t  bmAttributes;
    uint16_t wBytesPerInterval;
};

/// Imported from libusb, with some adjustments.
struct usb_string_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType; ///< Must match USB_DT_STRING.
    uint16_t wData[0x40];
};

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
enum usb_standard_request {
    /** Request status of the specific recipient */
    USB_REQUEST_GET_STATUS = 0x00,

    /** Clear or disable a specific feature */
    USB_REQUEST_CLEAR_FEATURE = 0x01,

    /* 0x02 is reserved */

    /** Set or enable a specific feature */
    USB_REQUEST_SET_FEATURE = 0x03,

    /* 0x04 is reserved */

    /** Set device address for all future accesses */
    USB_REQUEST_SET_ADDRESS = 0x05,

    /** Get the specified descriptor */
    USB_REQUEST_GET_DESCRIPTOR = 0x06,

    /** Used to update existing descriptors or add new descriptors */
    USB_REQUEST_SET_DESCRIPTOR = 0x07,

    /** Get the current device configuration value */
    USB_REQUEST_GET_CONFIGURATION = 0x08,

    /** Set device configuration */
    USB_REQUEST_SET_CONFIGURATION = 0x09,

    /** Return the selected alternate setting for the specified interface */
    USB_REQUEST_GET_INTERFACE = 0x0A,

    /** Select an alternate interface for the specified interface */
    USB_REQUEST_SET_INTERFACE = 0x0B,

    /** Set then report an endpoint's synchronization frame */
    USB_REQUEST_SYNCH_FRAME = 0x0C,

    /** Sets both the U1 and U2 Exit Latency */
    USB_REQUEST_SET_SEL = 0x30,

    /** Delay from the time a host transmits a packet to the time it is
     * received by the device. */
    USB_SET_ISOCH_DELAY = 0x31,
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

