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

/// ApplicationNeighborDetectionGroupConfiguration
typedef struct {
    u64 group_id;                                                                              ///< GroupId
    u8 key[0x10];
} NacpApplicationNeighborDetectionGroupConfiguration;

/// NeighborDetectionClientConfiguration
typedef struct {
    NacpApplicationNeighborDetectionGroupConfiguration send_group_configuration;               ///< SendGroupConfiguration
    NacpApplicationNeighborDetectionGroupConfiguration receivable_group_configurations[0x10];  ///< ReceivableGroupConfigurations
} NacpNeighborDetectionClientConfiguration;

/// ApplicationJitConfiguration
typedef struct {
    u64 flags;                                                                           ///< Flags
    u64 memory_size;                                                                     ///< MemorySize
} NacpApplicationJitConfiguration;

/// ns ApplicationControlProperty
typedef struct {
    NacpLanguageEntry lang[16];                                                          ///< \ref NacpLanguageEntry
    u8 isbn[0x25];                                                                       ///< Isbn
    u8 startup_user_account;                                                             ///< StartupUserAccount
    u8 user_account_switch_lock;                                                         ///< UserAccountSwitchLock
    u8 add_on_content_registration_type;                                                 ///< AddOnContentRegistrationType
    u32 attribute_flag;                                                                  ///< AttributeFlag
    u32 supported_language_flag;                                                         ///< SupportedLanguageFlag
    u32 parental_control_flag;                                                           ///< ParentalControlFlag
    u8 screenshot;                                                                       ///< Screenshot
    u8 video_capture;                                                                    ///< VideoCapture
    u8 data_loss_confirmation;                                                           ///< DataLossConfirmation
    u8 play_log_policy;                                                                  ///< PlayLogPolicy
    u64 presence_group_id;                                                               ///< PresenceGroupId
    s8 rating_age[0x20];                                                                 ///< RatingAge
    char display_version[0x10];                                                          ///< DisplayVersion
    u64 add_on_content_base_id;                                                          ///< AddOnContentBaseId
    u64 save_data_owner_id;                                                              ///< SaveDataOwnerId
    u64 user_account_save_data_size;                                                     ///< UserAccountSaveDataSize
    u64 user_account_save_data_journal_size;                                             ///< UserAccountSaveDataJournalSize
    u64 device_save_data_size;                                                           ///< DeviceSaveDataSize
    u64 device_save_data_journal_size;                                                   ///< DeviceSaveDataJournalSize
    u64 bcat_delivery_cache_storage_size;                                                ///< BcatDeliveryCacheStorageSize
    u64 application_error_code_category;                                                 ///< ApplicationErrorCodeCategory
    u64 local_communication_id[0x8];                                                     ///< LocalCommunicationId
    u8 logo_type;                                                                        ///< LogoType
    u8 logo_handling;                                                                    ///< LogoHandling
    u8 runtime_add_on_content_install;                                                   ///< RuntimeAddOnContentInstall
    u8 runtime_parameter_delivery;                                                       ///< RuntimeParameterDelivery
    u8 reserved_x30f4[0x2];                                                              ///< Reserved
    u8 crash_report;                                                                     ///< CrashReport
    u8 hdcp;                                                                             ///< Hdcp
    u64 pseudo_device_id_seed;                                                           ///< SeedForPseudoDeviceId
    char bcat_passphrase[0x41];                                                          ///< BcatPassphrase
    u8 startup_user_account_option;                                                      ///< StartupUserAccountOption
    u8 reserved_for_user_account_save_data_operation[0x6];                               ///< ReservedForUserAccountSaveDataOperation
    u64 user_account_save_data_size_max;                                                 ///< UserAccountSaveDataSizeMax
    u64 user_account_save_data_journal_size_max;                                         ///< UserAccountSaveDataJournalSizeMax
    u64 device_save_data_size_max;                                                       ///< DeviceSaveDataSizeMax
    u64 device_save_data_journal_size_max;                                               ///< DeviceSaveDataJournalSizeMax
    u64 temporary_storage_size;                                                          ///< TemporaryStorageSize
    u64 cache_storage_size;                                                              ///< CacheStorageSize
    u64 cache_storage_journal_size;                                                      ///< CacheStorageJournalSize
    u64 cache_storage_data_and_journal_size_max;                                         ///< CacheStorageDataAndJournalSizeMax
    u16 cache_storage_index_max;                                                         ///< CacheStorageIndexMax
    u8 reserved_x318a[0x6];                                                              ///< Reserved
    u64 play_log_queryable_application_id[0x10];                                         ///< PlayLogQueryableApplicationId
    u8 play_log_query_capability;                                                        ///< PlayLogQueryCapability
    u8 repair_flag;                                                                      ///< RepairFlag
    u8 program_index;                                                                    ///< ProgramIndex
    u8 required_network_service_license_on_launch;                                       ///< RequiredNetworkServiceLicenseOnLaunchFlag
    u32 reserved_x3214;                                                                  ///< Reserved
    NacpNeighborDetectionClientConfiguration neighbor_detection_client_configuration;    ///< NeighborDetectionClientConfiguration
    NacpApplicationJitConfiguration jit_configuration;                                   ///< JitConfiguration
    u8 reserved_x33c0[0xc40];                                                            ///< Reserved
} NacpStruct;

/// Get the NacpLanguageEntry from the input nacp corresponding to the current system language (this may fallback to other languages when needed). Output langentry is NULL if none found / content of entry is empty.
/// If you're using ns you may want to use \ref nsGetApplicationDesiredLanguage instead.
Result nacpGetLanguageEntry(NacpStruct* nacp, NacpLanguageEntry** langentry);

