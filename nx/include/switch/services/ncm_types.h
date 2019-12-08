/**
 * @file ncm_types.h
 * @brief Content Manager (ncm) service types (see ncm.h for the rest).
 * @author Adubbz, zhuowei, and yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../crypto/sha256.h"

/// StorageId
typedef enum {
    NcmStorageId_None          = 0,   ///< None
    NcmStorageId_Host          = 1,   ///< Host
    NcmStorageId_GameCard      = 2,   ///< GameCard
    NcmStorageId_BuiltInSystem = 3,   ///< BuiltInSystem
    NcmStorageId_BuiltInUser   = 4,   ///< BuiltInUser
    NcmStorageId_SdCard        = 5,   ///< SdCard
    NcmStorageId_Any           = 6,   ///< Any
} NcmStorageId;

/// ContentType
typedef enum  {
    NcmContentType_Meta             = 0, ///< Meta
    NcmContentType_Program          = 1, ///< Program
    NcmContentType_Data             = 2, ///< Data
    NcmContentType_Control          = 3, ///< Control
    NcmContentType_HtmlDocument     = 4, ///< HtmlDocument
    NcmContentType_LegalInformation = 5, ///< LegalInformation
    NcmContentType_DeltaFragment    = 6, ///< DeltaFragment
} NcmContentType;

/// ContentMetaType
typedef enum {
    NcmContentMetaType_Unknown              = 0x0,  ///< Unknown
    NcmContentMetaType_SystemProgram        = 0x1,  ///< SystemProgram
    NcmContentMetaType_SystemData           = 0x2,  ///< SystemData
    NcmContentMetaType_SystemUpdate         = 0x3,  ///< SystemUpdate
    NcmContentMetaType_BootImagePackage     = 0x4,  ///< BootImagePackage
    NcmContentMetaType_BootImagePackageSafe = 0x5,  ///< BootImagePackageSafe
    NcmContentMetaType_Application          = 0x80, ///< Application
    NcmContentMetaType_Patch                = 0x81, ///< Patch
    NcmContentMetaType_AddOnContent         = 0x82, ///< AddOnContent
    NcmContentMetaType_Delta                = 0x83, ///< Delta
} NcmContentMetaType;

/// ContentMetaAttribute
typedef enum {
    NcmContentMetaAttribute_None                = 0,      ///< None
    NcmContentMetaAttribute_IncludesExFatDriver = BIT(0), ///< IncludesExFatDriver
    NcmContentMetaAttribute_Rebootless          = BIT(1), ///< Rebootless
} NcmContentMetaAttribute;

/// ContentInstallType
typedef enum {
    NcmContentInstallType_Full         = 0, ///< Full
    NcmContentInstallType_FragmentOnly = 1, ///< FragmentOnly
    NcmContentInstallType_Unknown      = 7, ///< Unknown
} NcmContentInstallType;

/// ContentId
typedef struct {
    alignas(4) u8 c[0x10]; ///< Id
} NcmContentId;

/// PlaceHolderId
typedef struct {
    alignas(8) Uuid uuid;  ///< UUID
} NcmPlaceHolderId;

/// ContentMetaKey
typedef struct {
    u64 id;                             ///< Id.
    u32 version;                        ///< Version.
    u8 type;                            ///< \ref NcmContentMetaType
    u8 install_type;                    ///< \ref NcmContentInstallType
    u8 padding[2];                      ///< Padding.
} NcmContentMetaKey;

/// ApplicationContentMetaKey
typedef struct {
    NcmContentMetaKey key; ///< \ref NcmContentMetaKey
    u64 application_id;    ///< ApplicationId.
} NcmApplicationContentMetaKey;

/// ContentInfo
typedef struct {
    NcmContentId content_id;     ///< \ref NcmContentId
    u8 size[0x6];                ///< Content size.
    u8 content_type;             ///< \ref NcmContentType.
    u8 id_offset;                ///< Offset of this content. Unused by most applications.
} NcmContentInfo;

/// PackagedContentInfo
typedef struct {
    u8 hash[SHA256_HASH_SIZE];
    NcmContentInfo info;
} NcmPackagedContentInfo;

/// ContentMetaInfo
typedef struct {
    u64 id;                             ///< Id.
    u32 version;                        ///< Version.
    u8 type;                            ///< \ref NcmContentMetaType
    u8 attr;                            ///< \ref NcmContentMetaAttribute
    u8 padding[2];                      ///< Padding.
} NcmContentMetaInfo;

/// ContentMetaHeader
typedef struct {
    u16 extended_header_size;           ///< Size of optional struct that comes after this one.
    u16 content_count;                  ///< Number of NcmContentInfos after the extra bytes.
    u16 content_meta_count;             ///< Number of NcmContentMetaInfos that come after the NcmContentInfos.
    u8 attributes;                      ///< Usually None (0).
    u8 storage_id;                      ///< Usually None (0).
} NcmContentMetaHeader;

/// ApplicationMetaExtendedHeader
typedef struct {
    u64 patch_id;                     ///< PatchId of this application's patch.
    u32 required_system_version;      ///< Firmware version required by this application.
    u32 required_application_version; ///< [9.0.0+] Owner application version required by this application. Previously padding.
} NcmApplicationMetaExtendedHeader;

/// PatchMetaExtendedHeader
typedef struct {
    u64 application_id;          ///< ApplicationId of this patch's corresponding application.
    u32 required_system_version; ///< Firmware version required by this patch.
    u32 extended_data_size;      ///< Size of the extended data following the NcmContentInfos.
    u8 reserved[0x8];            ///< Unused.
} NcmPatchMetaExtendedHeader;

/// AddOnContentMetaExtendedHeader
typedef struct {
    u64 application_id;               ///< ApplicationId of this add-on-content's corresponding application.
    u32 required_application_version; ///< Version of the application required by this add-on-content.
    u32 padding;                      ///< Padding.
} NcmAddOnContentMetaExtendedHeader;

/// SystemUpdateMetaExtendedHeader
typedef struct {
    u32 extended_data_size; ///< Size of the extended data after NcmContentInfos and NcmContentMetaInfos.
} NcmSystemUpdateMetaExtendedHeader;

/// ProgramLocation
typedef struct {
    u64 program_id;         ///< ProgramId
    u8 storageID;           ///< \ref NcmStorageId
    u8 pad[7];              ///< Padding
} NcmProgramLocation;
