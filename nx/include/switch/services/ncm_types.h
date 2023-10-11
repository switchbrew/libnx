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
    NcmContentMetaType_DataPatch            = 0x84, ///< DataPatch
} NcmContentMetaType;

/// ContentMetaAttribute
typedef enum {
    NcmContentMetaAttribute_None                = 0,      ///< None
    NcmContentMetaAttribute_IncludesExFatDriver = BIT(0), ///< IncludesExFatDriver
    NcmContentMetaAttribute_Rebootless          = BIT(1), ///< Rebootless
    NcmContentMetaAttribute_Compacted           = BIT(2), ///< Compacted
} NcmContentMetaAttribute;

/// ContentInstallType
typedef enum {
    NcmContentInstallType_Full         = 0, ///< Full
    NcmContentInstallType_FragmentOnly = 1, ///< FragmentOnly
    NcmContentInstallType_Unknown      = 7, ///< Unknown
} NcmContentInstallType;

/// ContentMetaPlatform
typedef enum  {
    NcmContentMetaPlatform_Nx = 0, ///< Nx
} NcmContentMetaPlatform;

/// ContentId
typedef struct {
    u8 c[0x10]; ///< Id
} NcmContentId;

/// PlaceHolderId
typedef struct {
    Uuid uuid;  ///< UUID
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
    u32 size_low;                ///< Content size (low).
    u8 size_high;                ///< Content size (high).
    u8 attr;                     ///< Content attributes.
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

/// AddOnContentMetaExtendedHeader [15.0.0+]
typedef struct {
    u64 application_id;                  ///< ApplicationId of this add-on-content's corresponding application.
    u32 required_application_version;    ///< Version of the application required by this add-on-content.
    u8 content_accessibilities;          ///< Content accessibilities.
    u8 padding[3];                       ///< Padding.
    u64 data_patch_id;                   ///< DataPatchId of this add-on-content's corresponding data patch.
} NcmAddOnContentMetaExtendedHeader;

/// LegacyAddOnContentMetaExtendedHeader [1.0.0-14.1.2]
typedef struct {
    u64 application_id;               ///< ApplicationId of this add-on-content's corresponding application.
    u32 required_application_version; ///< Version of the application required by this add-on-content.
    u32 padding;                      ///< Padding.
} NcmLegacyAddOnContentMetaExtendedHeader;

/// DataPatchMetaExtendedHeader
typedef struct {
    u64 data_id;                      ///< DataId of this data patch's corresponding add-on-content.
    u64 application_id;               ///< ApplicationId of this data patch's add-on-content's corresponding application.
    u32 required_application_version; ///< Version of the application required by this data patch.
    u32 extended_data_size;           ///< Size of the extended data following the NcmContentInfos.
    u64 padding;                      ///< Padding.
} NcmDataPatchMetaExtendedHeader;

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

/**
 * @brief Retrieves the content size from a \ref NcmContentInfo struct.
 * @param[in] info Pointer to \ref NcmContentInfo struct.
 * @param[out] out Output size.
 */
NX_CONSTEXPR void ncmContentInfoSizeToU64(const NcmContentInfo *info, u64 *out) {
    *out = ((u64)info->size_high << 32) | info->size_low;
}

/**
 * @brief Updates the content size from a \ref NcmContentInfo struct.
 * @param[in] size Input size.
 * @param[out] out Pointer to \ref NcmContentInfo struct.
 */
NX_CONSTEXPR void ncmU64ToContentInfoSize(const u64 size, NcmContentInfo *info) {
    info->size_low = size & 0xFFFFFFFF;
    info->size_high = (u8)(size >> 32);
}
