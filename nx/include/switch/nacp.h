/**
 * @file nacp.h
 * @brief Control.nacp structure / related code for nacp.
 * @copyright libnx Authors
 */

#pragma once

/// Language entry. These strings are UTF-8.
typedef struct {
    char name[0x200];
    char author[0x100];
} NacpLanguageEntry;

/// ns ApplicationControlProperty
typedef struct {
    NacpLanguageEntry lang[16];
    u8 isbn[0x25];
    u8 startupUserAccount;
    u8 userAccountSwitchLock;
    u8 addOnContentRegistrationType;
    u32 applicationAttribute;
    u32 supportedLanguages;
    u32 parentalControl;
    u8 screenshot;
    u8 videoCaptureMode;
    u8 dataLossConfirmation;
    u8 playLogPolicy;
    u64 presenceGroupId;
    s8 ratingAge[0x20];
    char version[0x10];
    u64 addOnContentBaseId;
    u64 saveDataOwnerId;
    u64 userAccountSaveDataSize;
    u64 userAccountSaveDataJournalSize;
    u64 deviceSaveDataSize;
    u64 deviceSaveDataJournalSize;
    u64 bcatDeliveryCacheStorageSize;
    u64 applicationErrorCodeCategory;
    u64 localCommunicationIds[0x08];
    u8 logoType;
    u8 logoHandling;
    u8 runtimeAddOnContentInstall;
    u8 reserved_x30F3[0x03];
    u8 crashReport;
    u8 hdcp;
    u64 pseudoDeviceIdSeed;
    char bcatPassphrase[0x41];
    u8 reserved_x3141;
    u8 reserved_x3142[0x06];
    u64 userAccountSaveDataMaxSize;
    u64 userAccountSaveDataMaxJournalSize;
    u64 deviceSaveDataMaxSize;
    u64 deviceSaveDataMaxJournalSize;
    u64 temporaryStorageSize;
    u64 cacheStorageSize;
    u64 cacheStorageJournalSize;
    u64 cacheStorageAndJournalMaxSize;
    u64 cacheStorageMaxIndex;
    u64 playLogQueryableApplicationId[0x10];
    u8 playLogQueryCapability;
    u8 repairFlag;
    u8 programIndex;
    u8 requiredNetworkServiceLicenseOnLaunch;
    u8 reserved_x3214[0xDEC];
} NacpStruct;

/// Get the NacpLanguageEntry from the input nacp corresponding to the current system language (this may fallback to other languages when needed). Output langentry is NULL if none found / content of entry is empty.
Result nacpGetLanguageEntry(NacpStruct* nacp, NacpLanguageEntry** langentry);

