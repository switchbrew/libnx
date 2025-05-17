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
} NfpDeviceState;

typedef enum {
    NfcDeviceState_Initialized     = 0,
    NfcDeviceState_SearchingForTag = 1,
    NfcDeviceState_TagFound        = 2,
    NfcDeviceState_TagRemoved      = 3,
    NfcDeviceState_TagMounted      = 4,
} NfcDeviceState;

typedef enum {
    NfcMifareDeviceState_Initialized     = 0,
    NfcMifareDeviceState_SearchingForTag = 1,
    NfcMifareDeviceState_TagFound        = 2,
    NfcMifareDeviceState_TagRemoved      = 3,
    NfcMifareDeviceState_TagMounted      = 4,
    NfcMifareDeviceState_Unavailable     = 5,
} NfcMifareDeviceState;

typedef enum {
    NfpApplicationAreaVersion_3DS     = 0,     ///< Application area created by a 3DS game.
    NfpApplicationAreaVersion_WiiU    = 1,     ///< Application area created by a Wii U game.
    NfpApplicationAreaVersion_3DSv2   = 2,     ///< Application area created by a (new?) 3DS game.
    NfpApplicationAreaVersion_Switch  = 3,     ///< Application area created by a Switch game.
    NfpApplicationAreaVersion_Invalid = 0xFF,  ///< Invalid value (application area not created).
} NfpApplicationAreaVersion;

typedef enum {
    NfpDeviceType_Amiibo = 0,
} NfpDeviceType;

typedef enum {
    NfpMountTarget_Rom = BIT(0),
    NfpMountTarget_Ram = BIT(1),
    NfpMountTarget_All = NfpMountTarget_Rom | NfpMountTarget_Ram,
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

typedef enum {
    NfpAmiiboFlag_Valid                 = BIT(0),   ///< Initialized in system settings.
    NfpAmiiboFlag_ApplicationAreaExists = BIT(1),   ///< Application area exists.
} NfpAmiiboFlag;

typedef enum {
    NfpBreakType_Flush  = 0,
    NfpBreakType_Break1 = 1,
    NfpBreakType_Break2 = 2,
} NfpBreakType;

typedef struct {
    u16 year;
    u8 month;
    u8 day;
} NfpDate;

typedef struct {
    u8 uid[10];             ///< UUID.
    u8 uid_length;          ///< UUID length.
    u8 reserved[0x15];
} NfcTagUid;

typedef struct {
    NfcTagUid uid;       ///< UUID.
    u32 protocol;        ///< \ref NfcProtocol
    u32 tag_type;        ///< \ref NfcTagType
    u8 reserved[0x30];
} NfpTagInfo;

typedef struct {
    NfcTagUid uid;       ///< UUID.
    u32 protocol;        ///< \ref NfcProtocol
    u32 tag_type;        ///< \ref NfcTagType
    u8 reserved[0x30];
} NfcTagInfo;

typedef struct {
    NfpDate last_write_date;
    u16 write_counter;
    u16 version;
    u32 application_area_size;
    u8 reserved[0x34];
} NfpCommonInfo;

typedef struct {
    union {
        u8 character_id[3];
        struct {
            u16 game_character_id;
            u8 character_variant;
        } NX_PACKED;
    };
    u8 series_id;       ///< Series.
    u16 numbering_id;   ///< Model number.
    u8 nfp_type;        ///< Figure type.
    u8 reserved[0x39];
} NfpModelInfo;

typedef struct {
    MiiCharInfo mii;
    NfpDate first_write_date;
    char amiibo_name[(10*4)+1];   ///< Amiibo name (utf-8, null-terminated).
    u8 font_region;
    u8 reserved[0x7A];
} NfpRegisterInfo;

typedef struct {
    MiiStoreData mii_store_data;
    NfpDate first_write_date;
    char amiibo_name[(10*4)+1];   ///< Amiibo name (utf-8, null-terminated).
    u8 font_region;
    u8 reserved[0x8E];
} NfpRegisterInfoPrivate;

typedef struct {
    u64 application_id;
    u32 access_id;
    u16 crc32_change_counter;
    u8 flags;
    u8 tag_type;
    u8 application_area_version;
    u8 reserved[0x2F];
} NfpAdminInfo;

typedef struct {
    u8 tag_magic;                                         ///< Tag magic (always 0xA5: https://www.3dbrew.org/wiki/Amiibo#Page_layout).
    u8 reserved1[0x1];
    u16 tag_write_counter;                                ///< Incremented every tag write.
    u32 crc32_1;                                          ///< CRC32 of some internal 8-byte data.
    u8 reserved2[0x38];
    NfpDate last_write_date;                              ///< Updated every write.
    u16 write_counter;                                    ///< Incremented every write, until it maxes out at 0xFFFF.
    u16 version;                                          ///< Version.
    u32 application_area_size;                            ///< Size of the application area.
    u8 reserved3[0x34];
    MiiVer3StoreData mii_v3;                              ///< Ver3StoreData (Mii format used in 3DS).
    u8 pad[0x2];
    u16 mii_v3_crc16;                                     ///< CRC16 of Ver3StoreData.
    MiiNfpStoreDataExtension mii_store_data_extension;    ///< StoreDataExtension
    NfpDate first_write_date;                             ///< Set when the amiibo is first written to.
    u16 amiibo_name[10+1];                                ///< Amiibo name (utf-16, null-terminated).
    u8 font_region;                                       ///< Font region.
    u8 unknown1;                                          ///< Normally zero
    u32 crc32_2;                                          ///< CRC32 of Ver3StoreData + application_id_byte + unknown1 + StoreDataExtension + unknown2 (0x7E bytes total)
    u32 unknown2[0x5];                                    ///< Normally zero
    u8 reserved4[0x64];
    u64 application_id;                                   ///< Modified application ID (Application ID & 0xFFFFFFFF0FFFFFFF | 0x30000000)
    u32 access_id;                                        ///< Application area access ID
    u16 settings_crc32_change_counter;
    u8 flags;                                             ///< \ref NfpAmiiboFlag
    u8 tag_type;                                          ///< \ref NfcTagType
    u8 application_area_version;                          ///< \ref NfpApplicationAreaVersion
    u8 application_id_byte;                               ///< Application ID byte ((Application ID >> 28) & 0xFF)
    u8 reserved5[0x2E];
    u8 application_area[0xD8];                            ///< Application area.
} NfpData;

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
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpOpenApplicationArea(const NfcDeviceHandle *handle, u32 app_id);

/// Not available with ::NfpServiceType_System.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram, and the application area to be opened.
Result nfpGetApplicationArea(const NfcDeviceHandle *handle, void* buf, size_t buf_size, u32 *out_size);

/// Not available with ::NfpServiceType_System.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram, and the application area to be opened.
Result nfpSetApplicationArea(const NfcDeviceHandle *handle, const void* buf, size_t buf_size);

/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpFlush(const NfcDeviceHandle *handle);

Result nfpRestore(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_System.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpCreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size);

/// Not available with ::NfpServiceType_System.
/// Only available with [3.0.0+].
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram, and the application area to be opened.
Result nfpRecreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size);

/// Not available with ::NfpServiceType_System.
Result nfpGetApplicationAreaSize(const NfcDeviceHandle *handle, u32 *out_app_area_size);

/// Not available with ::NfpServiceType_User.
Result nfpDeleteApplicationArea(const NfcDeviceHandle *handle);

/// Not available with ::NfpServiceType_User.
Result nfpExistsApplicationArea(const NfcDeviceHandle *handle, bool *out);

Result nfpGetTagInfo(const NfcDeviceHandle *handle, NfpTagInfo *out);

/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpGetRegisterInfo(const NfcDeviceHandle *handle, NfpRegisterInfo *out);

/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpGetCommonInfo(const NfcDeviceHandle *handle, NfpCommonInfo *out);

/// Requires the amiibo to be mounted with ::NfpMountTarget_Rom.
Result nfpGetModelInfo(const NfcDeviceHandle *handle, NfpModelInfo *out);

/// Not available with ::NfpServiceType_User.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
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

Result nfpGetState(NfcState *out);
Result nfpGetDeviceState(const NfcDeviceHandle *handle, NfpDeviceState *out);
Result nfpGetNpadId(const NfcDeviceHandle *handle, u32 *out);

/// Only available with [4.0.0+].
Result nfcGetState(NfcState *out);
/// Only available with [4.0.0+].
Result nfcGetDeviceState(const NfcDeviceHandle *handle, NfcDeviceState *out);
/// Only available with [4.0.0+].
Result nfcGetNpadId(const NfcDeviceHandle *handle, u32 *out);

Result nfcMfGetState(NfcState *out);
Result nfcMfGetDeviceState(const NfcDeviceHandle *handle, NfcMifareDeviceState *out);
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
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpGetRegisterInfoPrivate(const NfcDeviceHandle *handle, NfpRegisterInfoPrivate *out);

/// Not available with ::NfpServiceType_User.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpSetRegisterInfoPrivate(const NfcDeviceHandle *handle, const NfpRegisterInfoPrivate *register_info_private);

/// Not available with ::NfpServiceType_User.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpDeleteRegisterInfo(const NfcDeviceHandle *handle);

/// Only available with ::NfpServiceType_Debug.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpGetAll(const NfcDeviceHandle *handle, NfpData *out);

/// Only available with ::NfpServiceType_Debug.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpSetAll(const NfcDeviceHandle *handle, const NfpData *nfp_data);

/// Only available with ::NfpServiceType_Debug.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpFlushDebug(const NfcDeviceHandle *handle);

/// Only available with ::NfpServiceType_Debug.
/// Requires the amiibo to be mounted with ::NfpMountTarget_Ram.
Result nfpBreakTag(const NfcDeviceHandle *handle, NfpBreakType break_type);

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
