/**
 * @file nfc.h
 * @brief Nintendo Figurine (amiibo) Platform (nfp:user) service IPC wrapper.
 * @author averne
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../services/hid.h"

typedef struct {
    u8  uuid[10];
    u8  uuid_length;
    u8  reserved1[0x15];
    u32 protocol;
    u32 tag_type;
    u8  reserved2[0x30];
} PACKED NfpuTagInfo;

typedef struct {
    u16 last_write_year;
    u8  last_write_month;
    u8  last_write_day;
    u16 write_counter;
    u16 version;
    u32 application_area_size;
    u8  reserved[0x34];
} PACKED NfpuCommonInfo;

typedef struct {
    u8 amiibo_id[0x8];
    u8 reserved[0x38];
} PACKED NfpuModelInfo;

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
} PACKED NfpuMiiCharInfo;

typedef struct {
    NfpuMiiCharInfo mii;
    u16 first_write_year;
    u8 first_write_month;
    u8 first_write_day;
    char amiibo_name[10+1]; ///< utf-8, null-terminated
    u8 reserved[0x99];
} PACKED NfpuRegisterInfo;

typedef struct {
    u64 unk1;
    u64 reserved1[3];
    u64 unk2;
    u64 reserved2[3];
} NfpuInitConfig;

typedef enum {
    NfpuState_NonInitialized = 0,
    NfpuState_Initialized    = 1,
} NfpuState;

typedef enum {
    NfpuDeviceState_Initialized     = 0,
    NfpuDeviceState_SearchingForTag = 1,
    NfpuDeviceState_TagFound        = 2,
    NfpuDeviceState_TagRemoved      = 3,
    NfpuDeviceState_TagMounted      = 4,
    NfpuDeviceState_Unavailable     = 5,
    NfpuDeviceState_Finalized       = 6,
} NfpuDeviceState;

typedef enum {
    NfpuDeviceType_Amiibo = 0,
} NfpuDeviceType;

typedef enum {
    NfpuMountTarget_Rom = 1,
    NfpuMountTarget_Ram = 2,
    NfpuMountTarget_All = 3,
} NfpuMountTarget;

Result nfpuInitialize(const NfpuInitConfig *config);
void nfpuExit(void);
Service* nfpuGetInterface(void);

Result nfpuStartDetection(HidControllerID id);
Result nfpuStopDetection(HidControllerID id);

/// Returned event will have autoclear off.
Result nfpuAttachActivateEvent(HidControllerID id, Event *out);
/// Returned event will have autoclear off.
Result nfpuAttachDeactivateEvent(HidControllerID id, Event *out);
/// Returned event will have autoclear on.
Result nfpuAttachAvailabilityChangeEvent(Event *out);

Result nfpuGetState(NfpuState *out);
Result nfpuGetDeviceState(HidControllerID id, NfpuDeviceState *out);
Result nfpuListDevices(u32 *count, HidControllerID *out, size_t num_elements);
Result nfpuGetNpadId(HidControllerID id, u32 *out);

Result nfpuMount(HidControllerID id, NfpuDeviceType device_type, NfpuMountTarget mount_target);
Result nfpuUnmount(HidControllerID id);

Result nfpuGetTagInfo(HidControllerID id, NfpuTagInfo *out);
Result nfpuGetRegisterInfo(HidControllerID id, NfpuRegisterInfo *out);
Result nfpuGetCommonInfo(HidControllerID id, NfpuCommonInfo *out);
Result nfpuGetModelInfo(HidControllerID id, NfpuModelInfo *out);

Result nfpuOpenApplicationArea(HidControllerID id, u32 app_id, u32 *npad_id);
Result nfpuGetApplicationArea(HidControllerID id, void* buf, size_t buf_size);
Result nfpuSetApplicationArea(HidControllerID id, const void* buf, size_t buf_size);
Result nfpuCreateApplicationArea(HidControllerID id, u32 app_id, const void* buf, size_t buf_size);

Result nfpuFlush(HidControllerID id);
Result nfpuRestore(HidControllerID id);

/// Calls nfc:user.
Result nfpuIsNfcEnabled(bool *out);
