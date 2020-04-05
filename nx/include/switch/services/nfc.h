/**
 * @file nfc.h
 * @brief Nintendo Figurine (amiibo) Platform (nfp:user) service IPC wrapper.
 * @author averne
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"

/// NfpServiceType
typedef enum {
    NfpServiceType_User           = 0,  ///< Initializes nfp:user.
    NfpServiceType_Debug          = 1,  ///< Initializes nfp:dbg.
    NfpServiceType_System         = 2,  ///< Initializes nfp:sys.
} NfpServiceType;

/// NfcServiceType
typedef enum {
    NfcServiceType_User           = 0,  ///< Initializes nfc:user.
    NfcServiceType_System         = 1,  ///< Initializes nfc:sys.
} NfcServiceType;

typedef enum {
    NfpState_NonInitialized = 0,
    NfpState_Initialized    = 1,
} NfpState;

typedef enum {
    NfpDeviceState_Initialized     = 0,
    NfpDeviceState_SearchingForTag = 1,
    NfpDeviceState_TagFound        = 2,
    NfpDeviceState_TagRemoved      = 3,
    NfpDeviceState_TagMounted      = 4,
    NfpDeviceState_Unavailable     = 5,
    NfpDeviceState_Finalized       = 6,
} NfpDeviceState;

typedef enum {
    NfpDeviceType_Amiibo = 0,
} NfpDeviceType;

typedef enum {
    NfpMountTarget_Rom = 1,
    NfpMountTarget_Ram = 2,
    NfpMountTarget_All = 3,
} NfpMountTarget;

typedef struct {
    u8  uuid[10];
    u8  uuid_length;
    u8  reserved1[0x15];
    u32 protocol;
    u32 tag_type;
    u8  reserved2[0x30];
} PACKED NfpTagInfo;

typedef struct {
    u16 last_write_year;
    u8  last_write_month;
    u8  last_write_day;
    u16 write_counter;
    u16 version;
    u32 application_area_size;
    u8  reserved[0x34];
} PACKED NfpCommonInfo;

typedef struct {
    u8 amiibo_id[0x8];
    u8 reserved[0x38];
} PACKED NfpModelInfo;

typedef struct {
    u8 unk_x0[0x10]; // Hash?
    u16 mii_name[10+1]; ///< utf-16be, null-terminated
    u8 unk_x26;
    u8 mii_color;
    u8 mii_sex;
    u8 mii_height;
    u8 mii_width;
    u8 unk_x2b[2];
    u8 mii_face_shape;
    u8 mii_face_color;
    u8 mii_wrinkles_style;
    u8 mii_makeup_style;
    u8 mii_hair_style;
    u8 mii_hair_color;
    u8 mii_has_hair_flipped;
    u8 mii_eye_style;
    u8 mii_eye_color;
    u8 mii_eye_size;
    u8 mii_eye_thickness;
    u8 mii_eye_angle;
    u8 mii_eye_pos_x;
    u8 mii_eye_pos_y;
    u8 mii_eyebrow_style;
    u8 mii_eyebrow_color;
    u8 mii_eyebrow_size;
    u8 mii_eyebrow_thickness;
    u8 mii_eyebrow_angle;
    u8 mii_eyebrow_pos_x;
    u8 mii_eyebrow_pos_y;
    u8 mii_nose_style;
    u8 mii_nose_size;
    u8 mii_nose_pos;
    u8 mii_mouth_style;
    u8 mii_mouth_color;
    u8 mii_mouth_size;
    u8 mii_mouth_thickness;
    u8 mii_mouth_pos;
    u8 mii_facial_hair_color;
    u8 mii_beard_style;
    u8 mii_mustache_style;
    u8 mii_mustache_size;
    u8 mii_mustache_pos;
    u8 mii_glasses_style;
    u8 mii_glasses_color;
    u8 mii_glasses_size;
    u8 mii_glasses_pos;
    u8 mii_has_mole;
    u8 mii_mole_size;
    u8 mii_mole_pos_x;
    u8 mii_mole_pos_y;
    u8 unk_x57;
} PACKED NfpMiiCharInfo;

typedef struct {
    NfpMiiCharInfo mii;
    u16 first_write_year;
    u8 first_write_month;
    u8 first_write_day;
    char amiibo_name[10+1]; ///< utf-8, null-terminated
    u8 reserved[0x99];
} PACKED NfpRegisterInfo;

typedef struct {
    u64 version;
    u64 reserved[3];
} NfcRequiredMcuVersionData;

/// Nfc/Nfp DeviceHandle
typedef struct {
    u8 handle[0x8];            ///< Handle.
} NfcDeviceHandle;

/// Initialize nfp:*.
Result nfpInitialize(NfpServiceType service_type);

/// Exit nfp:*.
void nfpExit(void);

/// Initialize nfc:*.
Result nfcInitialize(NfcServiceType service_type);

/// Exit nfc:*.
void nfcExit(void);

/// Gets the Service object for the actual nfp:* service session.
Service* nfpGetServiceSession(void);

/// Gets the Service object for the interface from nfp:*.
Service* nfpGetServiceSession_Interface(void);

/// Gets the Service object for the actual nfc:* service session.
Service* nfcGetServiceSession(void);

/// Gets the Service object for the interface from nfc:*.
Service* nfcGetServiceSession_Interface(void);

Result nfpListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count);
Result nfpStartDetection(const NfcDeviceHandle *handle);
Result nfpStopDetection(const NfcDeviceHandle *handle);
Result nfpMount(const NfcDeviceHandle *handle, NfpDeviceType device_type, NfpMountTarget mount_target);
Result nfpUnmount(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_System.
Result nfpOpenApplicationArea(const NfcDeviceHandle *handle, u32 app_id);

/// Not available with ::NfpServiceType_System.
Result nfpGetApplicationArea(const NfcDeviceHandle *handle, void* buf, size_t buf_size);

/// Not available with ::NfpServiceType_System.
Result nfpSetApplicationArea(const NfcDeviceHandle *handle, const void* buf, size_t buf_size);
Result nfpFlush(const NfcDeviceHandle *handle);
Result nfpRestore(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_System.
Result nfpCreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size);

Result nfpGetTagInfo(const NfcDeviceHandle *handle, NfpTagInfo *out);
Result nfpGetRegisterInfo(const NfcDeviceHandle *handle, NfpRegisterInfo *out);
Result nfpGetCommonInfo(const NfcDeviceHandle *handle, NfpCommonInfo *out);
Result nfpGetModelInfo(const NfcDeviceHandle *handle, NfpModelInfo *out);

/// Returned event will have autoclear off.
Result nfpAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event);
/// Returned event will have autoclear off.
Result nfpAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event);

Result nfpGetState(NfpState *out);
Result nfpGetDeviceState(const NfcDeviceHandle *handle, NfpDeviceState *out);
Result nfpGetNpadId(const NfcDeviceHandle *handle, u32 *out);

/// Returned event will have autoclear on.
/// Only available with [3.0.0+].
Result nfpAttachAvailabilityChangeEvent(Event *out_event);

/// This uses nfc:*.
Result nfcIsNfcEnabled(bool *out);
