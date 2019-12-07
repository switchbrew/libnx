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

/// Entry forSendDataConfiguration / ReceivableDataConfiguration.
typedef struct {
    u64 id;
    u8 key[0x10];
} NacpSendReceiveConfiguration;

/// ns ApplicationControlProperty
typedef struct {
    NacpLanguageEntry lang[16];                                         ///< \ref NacpLanguageEntry
    u8 isbn[0x25];                                                      ///< Isbn
    u8 startup_user_account;                                            ///< StartupUserAccount
    u8 user_account_switch_lock;                                        ///< UserAccountSwitchLock
    u8 add_on_content_registration_type;                                ///< AddOnContentRegistrationType
    u32 application_attribute;                                          ///< ApplicationAttribute
    u32 supported_languages;                                            ///< SupportedLanguages
    u32 parental_control;                                               ///< ParentalControl
    u8 screenshot;                                                      ///< Screenshot
    u8 video_capture_mode;                                              ///< VideoCaptureMode
    u8 data_loss_confirmation;                                          ///< DataLossConfirmation
    u8 play_log_policy;                                                 ///< PlayLogPolicy
    u64 presence_group_id;                                              ///< PresenceGroupId
    s8 rating_age[0x20];                                                ///< RatingAge
    char display_version[0x10];                                         ///< DisplayVersion
    u64 add_on_content_base_id;                                         ///< AddOnContentBaseId
    u64 save_data_owner_id;                                             ///< SaveDataOwnerId
    u64 user_account_save_data_size;                                    ///< UserAccountSaveDataSize
    u64 user_account_save_data_journal_size;                            ///< UserAccountSaveDataJournalSize
    u64 device_save_data_size;                                          ///< DeviceSaveDataSize
    u64 device_save_data_journal_size;                                  ///< DeviceSaveDataJournalSize
    u64 bcat_delivery_cache_storage_size;                               ///< BcatDeliveryCacheStorageSize
    u64 application_error_code_category;                                ///< ApplicationErrorCodeCategory
    u64 local_communication_ids[0x8];                                   ///< LocalCommunicationIds
    u8 logo_type;                                                       ///< LogoType
    u8 logo_handling;                                                   ///< LogoHandling
    u8 runtime_add_on_content_install;                                  ///< RuntimeAddOnContentInstall
    u8 reserved_x30f3[0x3];                                             ///< Reserved
    u8 crash_report;                                                    ///< CrashReport
    u8 hdcp;                                                            ///< Hdcp
    u64 pseudo_device_id_seed;                                          ///< SeedForPseudoDeviceId
    char bcat_passphrase[0x41];                                         ///< BcatPassphrase
    u8 startup_user_account_option;                                     ///< StartupUserAccountOption
    u8 reserved_for_user_account_save_data_operation[0x6];              ///< ReservedForUserAccountSaveDataOperation
    u64 user_account_save_data_max_size;                                ///< UserAccountSaveDataMaxSize
    u64 user_account_save_data_max_journal_size;                        ///< UserAccountSaveDataMaxJournalSize
    u64 device_save_data_max_size;                                      ///< DeviceSaveDataMaxSize
    u64 device_save_data_max_journal_size;                              ///< DeviceSaveDataMaxJournalSize
    u64 temporary_storage_size;                                         ///< TemporaryStorageSize
    u64 cache_storage_size;                                             ///< CacheStorageSize
    u64 cache_storage_journal_size;                                     ///< CacheStorageJournalSize
    u64 cache_storage_and_journal_max_size;                             ///< CacheStorageMaxSizeAndMaxJournalSize
    u64 cache_storage_max_index;                                        ///< CacheStorageMaxIndex
    u64 play_log_queryable_application_id[0x10];                        ///< PlayLogQueryableApplicationId
    u8 play_log_query_capability;                                       ///< PlayLogQueryCapability
    u8 repair_flag;                                                     ///< RepairFlag
    u8 program_index;                                                   ///< ProgramIndex
    u8 required_network_service_license_on_launch;                      ///< RequiredNetworkServiceLicenseOnLaunchFlag
    u32 reserved_x3214;                                                 ///< Reserved
    NacpSendReceiveConfiguration send_data_configuration;               ///< SendDataConfiguration
    NacpSendReceiveConfiguration receivable_data_configurations[0x10];  ///< ReceivableDataConfigurations
    u64 jit_configuration_flag;                                         ///< JitConfigurationFlag
    u64 memory_size;                                                    ///< MemorySize
    u8 reserved_x33c0[0xc40];                                           ///< Reserved
} NacpStruct;

/// Get the NacpLanguageEntry from the input nacp corresponding to the current system language (this may fallback to other languages when needed). Output langentry is NULL if none found / content of entry is empty.
Result nacpGetLanguageEntry(NacpStruct* nacp, NacpLanguageEntry** langentry);

