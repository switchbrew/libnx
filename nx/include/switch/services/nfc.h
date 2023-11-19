/**
 * @file nfc.h
 * @brief Nintendo Figurine (amiibo) Platform (nfp:user) service IPC wrapper.
 * @author averne
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/mii.h"

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
    NfcState_NonInitialized = 0,
    NfcState_Initialized    = 1,
} NfcState;

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
    NfcDeviceState_Initialized     = 0,
    NfcDeviceState_SearchingForTag = 1,
    NfcDeviceState_TagFound        = 2,
    NfcDeviceState_TagRemoved      = 3,
    NfcDeviceState_TagMounted      = 4,
    NfcDeviceState_Unavailable     = 5,
    NfcDeviceState_Finalized       = 6,
} NfcDeviceState;

typedef enum {
    NfpApplicationAreaVersion_3DS    = 0,
    NfpApplicationAreaVersion_WiiU   = 1,
    NfpApplicationAreaVersion_3DSv2  = 2,
    NfpApplicationAreaVersion_Switch = 3,
    NfpApplicationAreaVersion_NotSet = 0xFF,
} NfpApplicationAreaVersion;

typedef enum {
    NfpDeviceType_Amiibo = 0,
} NfpDeviceType;

typedef enum {
    NfpMountTarget_Rom = 1,
    NfpMountTarget_Ram = 2,
    NfpMountTarget_All = 3,
} NfpMountTarget;

typedef enum {
    NfcProtocol_None  = 0,
    NfcProtocol_TypeA = BIT(0), ///< ISO14443A
    NfcProtocol_TypeB = BIT(1), ///< ISO14443B
    NfcProtocol_TypeF = BIT(2), ///< Sony FeliCa
    NfcProtocol_All   = 0xFFFFFFFF,
} NfcProtocol;

typedef enum {
    NfcTagType_None   = 0,
    NfcTagType_Type1  = BIT(0), ///< ISO14443A RW. Topaz
    NfcTagType_Type2  = BIT(1), ///< ISO14443A RW. Ultralight, NTAGX, ST25TN
    NfcTagType_Type3  = BIT(2), ///< ISO14443A RW/RO. Sony FeliCa
    NfcTagType_Type4A = BIT(3), ///< ISO14443A RW/RO. DESFire
    NfcTagType_Type4B = BIT(4), ///< ISO14443B RW/RO. DESFire
    NfcTagType_Type5  = BIT(5), ///< ISO15693 RW/RO. SLI, SLIX, ST25TV
    NfcTagType_Mifare = BIT(6), ///< Mifare clasic. Skylanders
    NfcTagType_All    = 0xFFFFFFFF,
} NfcTagType;

typedef enum {
    NfcMifareCommand_Read      = 0x30,
    NfcMifareCommand_AuthA     = 0x60,
    NfcMifareCommand_AuthB     = 0x61,
    NfcMifareCommand_Write     = 0xA0,
    NfcMifareCommand_Transfer  = 0xB0,
    NfcMifareCommand_Decrement = 0xC0,
    NfcMifareCommand_Increment = 0xC1,
    NfcMifareCommand_Store     = 0xC2,
} NfcMifareCommand;

typedef struct {
    u8  uuid[10];
    u8  uuid_length;
    u8  reserved1[0x15];
    u32 protocol;
    u32 tag_type;
    u8  reserved2[0x30];
} NX_PACKED NfpTagInfo;

typedef struct {
    u8  uuid[10];
    u8  uuid_length;
    u8  reserved1[0x15];
    u32 protocol;
    u32 tag_type;
    u8  reserved2[0x30];
} NX_PACKED NfcTagInfo;

typedef struct {
    u16 last_write_year;
    u8  last_write_month;
    u8  last_write_day;
    u16 write_counter;
    u16 version;
    u32 application_area_size;
    u8  reserved[0x34];
} NX_PACKED NfpCommonInfo;

typedef struct {
    u8 amiibo_id[0x8];
    u8 reserved[0x38];
} NX_PACKED NfpModelInfo;

typedef struct {
    MiiCharInfo mii;
    u16 first_write_year;
    u8 first_write_month;
    u8 first_write_day;
    char amiibo_name[(10*4)+1]; ///< utf-8, null-terminated
    u8 font_region;
    u8 reserved[0x7A];
} NX_PACKED NfpRegisterInfo;

typedef struct {
    u8 mii_store_data[0x44];
    u16 first_write_year;
    u8 first_write_month;
    u8 first_write_day;
    char amiibo_name[(10*4)+1]; ///< utf-8, null-terminated
    u8 font_region;
    u8 reserved[0x8E];
} NX_PACKED NfpRegisterInfoPrivate;

typedef struct {
    u64 application_id;
    u32 application_area_id;
    u16 crc_change_counter;
    u8 flags;
    u8 tag_type;
    u8 application_area_version;
    u8 reserved[0x2F];
} NX_PACKED NfpAdminInfo;

typedef struct {
    u8 magic;
    u8 reserved1[0x1];
    u8 write_counter;
    u8 reserved2[0x1];
    u32 settings_crc;
    u8 reserved3[0x38];
    u16 last_write_year;
    u8  last_write_month;
    u8  last_write_day;
    u16 application_write_counter;
    u16 version;
    u32 application_area_size;
    u8  reserved4[0x34];
    MiiCharInfo mii;
    MiiNfpStoreDataExtension mii_store_data_extension;
    u16 first_write_year;
    u8 first_write_month;
    u8 first_write_day;
    u16 amiibo_name[10+1]; ///< utf-16, null-terminated
    u8 settings_flag;      ///< bit4 = amiibo was initialized in console settings, bit5 = has application area
    u8 unknown1;           ///< Normally zero
    u32 register_info_crc;
    u32 unknown2[0x5];     ///< Normally zero
    u8 reserved5[0x64];
    u64 application_id;
    u32 access_id;
    u16 settings_crc_counter;
    u8 font_region;
    u8 tag_type;
    u8 console_type;
    u8 application_id_byte; ///< (Original Program ID >> 0x24) & 0xF byte (Program ID has this byte swapped with console type)
    u8 reserved6[0x2E];
    u8 application_area[0xD8];
} NX_PACKED NfpData;

typedef struct {
    u8 mifare_command;
    u8 unknown; ///< Usually 1
    u8 reserved1[0x6];
    u8 sector_key[0x6];
    u8 reserved2[0x2];
} NX_PACKED NfcSectorKey;

typedef struct {
    u8 sector_number;
    u8 reserved[0x7];
    NfcSectorKey sector_key;
} NX_PACKED NfcMifareReadBlockParameter;

typedef struct {
    u8 data[0x10];
    u8 sector_number;
    u8 reserved[0x7];
} NX_PACKED NfcMifareReadBlockData;

typedef struct {
    u8 data[0x10];
    u8 sector_number;
    u8 reserved[0x7];
    NfcSectorKey sector_key;
} NfcMifareWriteBlockParameter;

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

/// Initialize nfc:mf:u.
Result nfcMfInitialize();

/// Exit nfc:mf:u.
void nfcMfExit(void);

/// Gets the Service object for the actual nfp:* service session.
Service* nfpGetServiceSession(void);

/// Gets the Service object for the interface from nfp:*.
Service* nfpGetServiceSession_Interface(void);

/// Gets the Service object for the actual nfc:* service session.
Service* nfcGetServiceSession(void);

/// Gets the Service object for the interface from nfc:*.
Service* nfcGetServiceSession_Interface(void);

/// Gets the Service object for the actual nfc:mf:u service session.
Service* nfcMfGetServiceSession(void);

/// Gets the Service object for the interface from nfc:mf:u.
Service* nfcMfGetServiceSession_Interface(void);

Result nfpListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count);
Result nfpStartDetection(const NfcDeviceHandle *handle);
Result nfpStopDetection(const NfcDeviceHandle *handle);
Result nfpMount(const NfcDeviceHandle *handle, NfpDeviceType device_type, NfpMountTarget mount_target);
Result nfpUnmount(const NfcDeviceHandle *handle);

/// Only available with [4.0.0+].
Result nfcListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count);
/// Only available with [4.0.0+].
Result nfcStartDetection(const NfcDeviceHandle *handle, NfcProtocol protocol);
/// Only available with [4.0.0+].
Result nfcStopDetection(const NfcDeviceHandle *handle);

Result nfcMfListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count);
Result nfcMfStartDetection(const NfcDeviceHandle *handle);
Result nfcMfStopDetection(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_System.
Result nfpOpenApplicationArea(const NfcDeviceHandle *handle, u32 app_id);

/// Not available with ::NfpServiceType_System.
Result nfpGetApplicationArea(const NfcDeviceHandle *handle, void* buf, size_t buf_size, u32 *out_size);

/// Not available with ::NfpServiceType_System.
Result nfpSetApplicationArea(const NfcDeviceHandle *handle, const void* buf, size_t buf_size);
Result nfpFlush(const NfcDeviceHandle *handle);
Result nfpRestore(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_System.
Result nfpCreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size);

/// Not available with ::NfpServiceType_System.
/// Only available with [3.0.0+].
Result nfpRecreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size);

/// Not available with ::NfpServiceType_System.
Result nfpGetApplicationAreaSize(const NfcDeviceHandle *handle, u32 *out_app_area_size);

/// Not available with ::NfpServiceType_User.
Result nfpDeleteApplicationArea(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_User.
Result nfpExistsApplicationArea(const NfcDeviceHandle *handle, bool *out);

Result nfpGetTagInfo(const NfcDeviceHandle *handle, NfpTagInfo *out);
Result nfpGetRegisterInfo(const NfcDeviceHandle *handle, NfpRegisterInfo *out);
Result nfpGetCommonInfo(const NfcDeviceHandle *handle, NfpCommonInfo *out);
Result nfpGetModelInfo(const NfcDeviceHandle *handle, NfpModelInfo *out);
/// Not available with ::NfpServiceType_User.
Result nfpGetAdminInfo(const NfcDeviceHandle *handle, NfpAdminInfo *out);

/// Only available with [4.0.0+].
Result nfcGetTagInfo(const NfcDeviceHandle *handle, NfcTagInfo *out);

Result nfcMfGetTagInfo(const NfcDeviceHandle *handle, NfcTagInfo *out);

/// Returned event will have autoclear off.
Result nfpAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event);
/// Returned event will have autoclear off.
Result nfpAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event);

/// Returned event will have autoclear off.
/// Only available with [4.0.0+].
Result nfcAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event);
/// Returned event will have autoclear off.
/// Only available with [4.0.0+].
Result nfcAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event);

/// Returned event will have autoclear off.
Result nfcMfAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event);
/// Returned event will have autoclear off.
Result nfcMfAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event);

Result nfpGetState(NfpState *out);
Result nfpGetDeviceState(const NfcDeviceHandle *handle, NfpDeviceState *out);
Result nfpGetNpadId(const NfcDeviceHandle *handle, u32 *out);

/// Only available with [4.0.0+].
Result nfcGetState(NfcState *out);
/// Only available with [4.0.0+].
Result nfcGetDeviceState(const NfcDeviceHandle *handle, NfcDeviceState *out);
/// Only available with [4.0.0+].
Result nfcGetNpadId(const NfcDeviceHandle *handle, u32 *out);

Result nfcMfGetState(NfcState *out);
Result nfcMfGetDeviceState(const NfcDeviceHandle *handle, NfcDeviceState *out);
Result nfcMfGetNpadId(const NfcDeviceHandle *handle, u32 *out);

/// Returned event will have autoclear on.
/// Only available with [3.0.0+].
Result nfpAttachAvailabilityChangeEvent(Event *out_event);
/// Returned event will have autoclear on.
/// Only available with [4.0.0+].
Result nfcAttachAvailabilityChangeEvent(Event *out_event);
/// Returned event will have autoclear on.
Result nfcMfAttachAvailabilityChangeEvent(Event *out_event);

/// Not available with ::NfpServiceType_User.
Result nfpFormat(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_User.
Result nfpGetRegisterInfoPrivate(const NfcDeviceHandle *handle, NfpRegisterInfoPrivate *out);
/// Not available with ::NfpServiceType_User.
Result nfpSetRegisterInfoPrivate(const NfcDeviceHandle *handle, const NfpRegisterInfoPrivate *register_info_private);
/// Not available with ::NfpServiceType_User.
Result nfpDeleteRegisterInfo(const NfcDeviceHandle *handle);

/// Only available with ::NfpServiceType_Debug.
Result nfpGetAll(const NfcDeviceHandle *handle, NfpData *out);
/// Only available with ::NfpServiceType_Debug.
Result nfpSetAll(const NfcDeviceHandle *handle, const NfpData *nfp_data);

/// Only available with ::NfpServiceType_Debug.
Result nfpFlushDebug(const NfcDeviceHandle *handle);
/// Only available with ::NfpServiceType_Debug.
Result nfpBreakTag(const NfcDeviceHandle *handle, u32 break_type);
/// Only available with ::NfpServiceType_Debug.
Result nfpReadBackupData(const NfcDeviceHandle *handle, void* out_buf, size_t buf_size, u32 *out_size);
/// Only available with ::NfpServiceType_Debug.
Result nfpWriteBackupData(const NfcDeviceHandle *handle, const void* buf, size_t buf_size);
/// Only available with ::NfpServiceType_Debug.
Result nfpWriteNtf(const NfcDeviceHandle *handle, u32 write_type, const void* buf, size_t buf_size);

/// This uses nfc:*.
Result nfcIsNfcEnabled(bool *out);

/// Only available with [4.0.0+].
Result nfcReadMifare(const NfcDeviceHandle *handle, NfcMifareReadBlockData *out_block_data, const NfcMifareReadBlockParameter *read_block_parameter, s32 count);
/// Only available with [4.0.0+].
Result nfcWriteMifare(const NfcDeviceHandle *handle, const NfcMifareWriteBlockParameter *write_block_parameter, s32 count);

Result nfcMfReadMifare(const NfcDeviceHandle *handle, NfcMifareReadBlockData *out_block_data, const NfcMifareReadBlockParameter *read_block_parameter, s32 count);
Result nfcMfWriteMifare(const NfcDeviceHandle *handle, const NfcMifareWriteBlockParameter *write_block_parameter, s32 count);

/// Only available with [4.0.0+].
Result nfcSendCommandByPassThrough(const NfcDeviceHandle *handle, u64 timeout, const void* cmd_buf, size_t cmd_buf_size, void* reply_buf, size_t reply_buf_size, u64 *out_size);
/// Only available with [4.0.0+].
Result nfcKeepPassThroughSession(const NfcDeviceHandle *handle);
/// Only available with [4.0.0+].
Result nfcReleasePassThroughSession(const NfcDeviceHandle *handle);
