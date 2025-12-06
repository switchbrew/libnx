/**
 * @file ns.h
 * @brief NS services IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../nacp.h"
#include "../sf/service.h"
#include "../services/ncm_types.h"
#include "../services/async.h"
#include "../services/acc.h"
#include "../services/fs.h"
#include "../applets/error.h"
#include "../kernel/event.h"
#include "../kernel/tmem.h"

/// ShellEvent
typedef enum {
    NsShellEvent_None  = 0,         ///< None
    NsShellEvent_Exit  = 1,         ///< Exit
    NsShellEvent_Start = 2,         ///< Start
    NsShellEvent_Crash = 3,         ///< Crash
    NsShellEvent_Debug = 4,         ///< Debug
} NsShellEvent;

/// ApplicationControlSource
typedef enum {
    NsApplicationControlSource_CacheOnly   = 0,       ///< Returns data from cache.
    NsApplicationControlSource_Storage     = 1,       ///< Returns data from storage if not present in cache.
    NsApplicationControlSource_StorageOnly = 2,       ///< Returns data from storage without using cache.
} NsApplicationControlSource;

/// BackgroundNetworkUpdateState
typedef enum {
    NsBackgroundNetworkUpdateState_None        = 0,   ///< No sysupdate task exists.
    NsBackgroundNetworkUpdateState_Downloading = 1,   ///< Sysupdate download in progress.
    NsBackgroundNetworkUpdateState_Ready       = 2,   ///< Sysupdate ready, pending install.
} NsBackgroundNetworkUpdateState;

/// LatestSystemUpdate
typedef enum {
    NsLatestSystemUpdate_UpToDate      = 0,           ///< UpToDate
    NsLatestSystemUpdate_Downloaded    = 1,           ///< Downloaded
    NsLatestSystemUpdate_NeedsDownload = 2,           ///< NeedsDownload
} NsLatestSystemUpdate;

/// RequestServerStopper
typedef struct {
    Service s;                     ///< IRequestServerStopper
} NsRequestServerStopper;

/// ProgressMonitorForDeleteUserSaveDataAll
typedef struct {
    Service s;                     ///< IProgressMonitorForDeleteUserSaveDataAll
} NsProgressMonitorForDeleteUserSaveDataAll;

/// ProgressAsyncResult
typedef struct {
    Service s;                     ///< IProgressAsyncResult
    Event event;                   ///< Event with autoclear=false.
} NsProgressAsyncResult;

/// SystemUpdateControl
typedef struct {
    Service s;                     ///< ISystemUpdateControl
    TransferMemory tmem;           ///< TransferMemory for SetupCardUpdate/SetupCardUpdateViaSystemUpdater.
} NsSystemUpdateControl;

/// ApplicationControlData
typedef struct {
    NacpStruct nacp;               ///< \ref NacpStruct
    u8 icon[0x20000];              ///< JPEG
} NsApplicationControlData;

/// ApplicationOccupiedSize
typedef struct {
    u8 unk_x0[0x80];               ///< Unknown.
} NsApplicationOccupiedSize;

/// NsApplicationContentMetaStatus
typedef struct {
    u8 meta_type;                  ///< \ref NcmContentMetaType
    u8 storageID;                  ///< \ref NcmStorageId
    u8 rights_check;               ///< RightsCheck.
    u8 reserved;                   ///< Reserved.
    u32 version;                   ///< Application version.
    u64 application_id;            ///< ApplicationId.
} NsApplicationContentMetaStatus;

/// ApplicationRecord
typedef struct {
    u64 application_id;            ///< ApplicationId.
    u8 last_event;                 ///< LastEvent.
    u8 attributes;                 ///< Attributes.
    u8 reserved[6];                ///< Reserved.
    u64 last_updated;              ///< LastUpdated.
} NsApplicationRecord;

/// ProgressForDeleteUserSaveDataAll
typedef struct {
    u8 unk_x0[0x28];                       ///< Unknown.
} NsProgressForDeleteUserSaveDataAll;

/// ApplicationViewDeprecated. The below comments are for the \ref NsApplicationView to NsApplicationViewDeprecated conversion done by \ref nsGetApplicationViewDeprecated on newer system-versions.
typedef struct {
    u64 application_id;                    ///< Same as NsApplicationView::application_id.
    u8 unk_x8[0x4];                        ///< Same as NsApplicationView::unk_x8.
    u32 flags;                             ///< Same as NsApplicationView::flags.
    u8 unk_x10[0x10];                      ///< Same as NsApplicationView::unk_x10.
    u32 unk_x20;                           ///< Same as NsApplicationView::unk_x20.
    u16 unk_x24;                           ///< Same as NsApplicationView::unk_x24.
    u8 unk_x26[0x2];                       ///< Cleared to zero.
    u8 unk_x28[0x10];                      ///< Same as NsApplicationView::unk_x30.
    u32 unk_x38;                           ///< Same as NsApplicationView::unk_x40.
    u8 unk_x3c;                            ///< Same as NsApplicationView::unk_x44.
    u8 unk_x3d[3];                         ///< Cleared to zero.
} NsApplicationViewDeprecated;

/// ApplicationView
typedef struct {
    u64 application_id;                    ///< ApplicationId.
    u8 unk_x8[0x4];                        ///< Unknown.
    u32 flags;                             ///< Flags.
    u8 unk_x10[0x10];                      ///< Unknown.
    u32 unk_x20;                           ///< Unknown.
    u16 unk_x24;                           ///< Unknown.
    u8 unk_x26[0x2];                       ///< Unknown.
    u8 unk_x28[0x8];                       ///< Unknown.
    u8 unk_x30[0x10];                      ///< Unknown.
    u32 unk_x40;                           ///< Unknown.
    u8 unk_x44;                            ///< Unknown.
    u8 unk_x45[0xb];                       ///< Unknown.
} NsApplicationView;

/// NsPromotionInfo
typedef struct {
    u64 start_timestamp;                   ///< POSIX timestamp for the promotion start.
    u64 end_timestamp;                     ///< POSIX timestamp for the promotion end.
    s64 remaining_time;                    ///< Remaining time until the promotion ends, in nanoseconds ({end_timestamp - current_time} converted to nanoseconds).
    u8 unk_x18[0x4];                       ///< Not set, left at zero.
    u8 flags;                              ///< Flags. Bit0: whether the PromotionInfo is valid (including bit1). Bit1 clear: remaining_time is set.
    u8 pad[3];                             ///< Padding.
} NsPromotionInfo;

/// NsApplicationViewWithPromotionInfo
typedef struct {
    NsApplicationView view;                ///< \ref NsApplicationView
    NsPromotionInfo promotion;             ///< \ref NsPromotionInfo
} NsApplicationViewWithPromotionInfo;

/// LaunchProperties
typedef struct {
    u64 program_id;                ///< program_id.
    u32 version;                   ///< Program version.
    u8 storageID;                  ///< \ref NcmStorageId
    u8 index;                      ///< Index.
    u8 is_application;             ///< Whether this is an Application.
} NsLaunchProperties;

/// ShellEventInfo
typedef struct {
    NsShellEvent event;            ///< \ref NsShellEvent
    u64 process_id;                ///< processID.
} NsShellEventInfo;

/// SystemUpdateProgress. Commands which have this as output will return 0 with the output cleared, when no task is available.
typedef struct {
    s64 current_size;              ///< Current size. This value can be larger than total_size when the async operation is finishing. When total_size is <=0, this current_size field may contain a progress value for when the total_size is not yet determined.
    s64 total_size;                ///< Total size, this field is only valid when >0.
} NsSystemUpdateProgress;

/// ReceiveApplicationProgress. Same as \ref NsSystemUpdateProgress, except cmds which return this will return actual errors on failure, instead of returning 0 with a cleared struct.
typedef NsSystemUpdateProgress NsReceiveApplicationProgress;

/// SendApplicationProgress. Same as \ref NsSystemUpdateProgress, except cmds which return this will return actual errors on failure, instead of returning 0 with a cleared struct.
typedef NsSystemUpdateProgress NsSendApplicationProgress;

/// EulaDataPath
typedef struct {
    char path[0x100];              ///< Path.
} NsEulaDataPath;

/// SystemDeliveryInfo
typedef struct {
    struct {
        u32 system_delivery_protocol_version;       ///< SystemDeliveryProtocolVersion.
        u32 application_delivery_protocol_version;  ///< ApplicationDeliveryProtocolVersion.
        u8 has_exfat;                               ///< HasExFat.
        u8 reserved[0x3];                           ///< Reserved.
        u32 system_update_version;                  ///< SystemUpdateVersion.
        u64 old_system_update_id;                   ///< OldSystemUpdateId.
        u8 firmware_variation_id;                   ///< FirmwareVariationId.
        u8 updatable_firmware_group_id;             ///< UpdatableFirmwareGroupId.
        u8 platform_region;                         ///< PlatformRegion.
        u8 system_delivery_info_platform;           ///< [20.0.0+] SystemDeliveryInfoPlatform.
        u8 system_update_id_flag;                   ///< [20.0.0+] SystemUpdateIdFlag. When non-zero, SystemUpdateId is used instead of OldSystemUpdateId.
        u8 pad[0x3];                                ///< Padding.
        u64 system_update_id;                       ///< [20.0.0+] SystemUpdateId.
        u8 reserved_x28[0xb8];                      ///< Reserved.
    } data;                                         ///< Data used with the below hmac.
    u8 hmac[0x20];                                  ///< HMAC-SHA256 over the above data.
} NsSystemDeliveryInfo;

/// ApplicationDeliveryInfo
typedef struct {
    struct {
        u32 application_delivery_protocol_version;  ///< ApplicationDeliveryProtocolVersion.
        u8 pad[0x4];                                ///< Padding.
        u64 application_id;                         ///< ApplicationId.
        u32 application_version;                    ///< ApplicationVersion.
        u32 required_application_version;           ///< RequiredApplicationVersion.
        u32 required_system_version;                ///< RequiredSystemVersion.
        u32 attributes;                             ///< Bitfield of ApplicationDeliveryAttributeTag.
        u8 platform;                                ///< [20.0.0+] \ref NcmContentMetaPlatform
        u8 proper_program_exists;                   ///< [20.0.0+] ProperProgramExists.
        u8 reserved[0xbe];                          ///< Reserved.
    } data;                                         ///< Data used with the below hmac.
    u8 hmac[0x20];                                  ///< HMAC-SHA256 over the above data.
} NsApplicationDeliveryInfo;

/// NsApplicationRightsOnClient
typedef struct {
    u64 application_id;                             ///< ApplicationId.
    AccountUid uid;                                 ///< \ref AccountUid
    u8 flags_x18;                                   ///< qlaunch uses bit0-bit4 and bit7 from here.
    u8 flags_x19;                                   ///< qlaunch uses bit0 from here.
    u8 unk_x1a[0x6];                                ///< Unknown.
} NsApplicationRightsOnClient;

/// DownloadTaskStatus
typedef struct {
    u8 unk_x0[0x20];                                ///< Unknown.
} NsDownloadTaskStatus;

/// Default size for \ref nssuControlSetupCardUpdate / \ref nssuControlSetupCardUpdateViaSystemUpdater. This is the size used by qlaunch for SetupCardUpdate.
#define NSSU_CARDUPDATE_TMEM_SIZE_DEFAULT 0x100000

///@name ns
///@{

/// Initialize ns services. Uses ns:am on pre-3.0.0, ns:am2 on [3.0.0+].
Result nsInitialize(void);

/// Exit ns services.
void nsExit(void);

/// Gets the Service object for the actual ns:* service session. Only initialized on [3.0.0+], on pre-3.0.0 see \ref nsGetServiceSession_ApplicationManagerInterface.
Service* nsGetServiceSession_GetterInterface(void);

/// Gets the Service object for IApplicationManagerInterface. Only initialized on pre-3.0.0, on [3.0.0+] use \ref nsGetApplicationManagerInterface.
Service* nsGetServiceSession_ApplicationManagerInterface(void);

/// Gets the Service object for IDynamicRightsInterface via the cmd for that.
/// Only available on [6.0.0+].
Result nsGetDynamicRightsInterface(Service* srv_out);

/// Gets the Service object for IReadOnlyApplicationControlDataInterface via the cmd for that.
/// Only available on [5.1.0+].
Result nsGetReadOnlyApplicationControlDataInterface(Service* srv_out);

/// Gets the Service object for IReadOnlyApplicationRecordInterface via the cmd for that.
/// Only available on [5.0.0+].
Result nsGetReadOnlyApplicationRecordInterface(Service* srv_out);

/// Gets the Service object for IECommerceInterface via the cmd for that.
/// Only available on [4.0.0+].
Result nsGetECommerceInterface(Service* srv_out);

/// Gets the Service object for IApplicationVersionInterface via the cmd for that.
/// Only available on [4.0.0+].
Result nsGetApplicationVersionInterface(Service* srv_out);

/// Gets the Service object for IFactoryResetInterface via the cmd for that.
/// Only available on [3.0.0+].
Result nsGetFactoryResetInterface(Service* srv_out);

/// Gets the Service object for IAccountProxyInterface via the cmd for that.
/// Only available on [3.0.0+].
Result nsGetAccountProxyInterface(Service* srv_out);

/// Gets the Service object for IApplicationManagerInterface via the cmd for that.
/// Only available on [3.0.0+], on prior sysvers use \ref nsGetServiceSession_ApplicationManagerInterface.
Result nsGetApplicationManagerInterface(Service* srv_out);

/// Gets the Service object for IDownloadTaskInterface via the cmd for that.
/// Only available on [3.0.0+].
Result nsGetDownloadTaskInterface(Service* srv_out);

/// Gets the Service object for IContentManagementInterface via the cmd for that.
/// Only available on [3.0.0+].
Result nsGetContentManagementInterface(Service* srv_out);

/// Gets the Service object for IDocumentInterface via the cmd for that.
/// Only available on [3.0.0+].
Result nsGetDocumentInterface(Service* srv_out);

///@}

///@name IReadOnlyApplicationControlDataInterface
///@{

/**
 * @brief Gets the \ref NsApplicationControlData for the specified application.
 * @note Uses \ref nsGetReadOnlyApplicationControlDataInterface on [5.1.0+], otherwise IApplicationManagerInterface is used.
 * @param[in] source Source, official sw uses ::NsApplicationControlSource_Storage.
 * @param[in] application_id ApplicationId.
 * @param[out] buffer \ref NsApplicationControlData
 * @param[in] size Size of the buffer.
 * @param[out] actual_size Actual output size.
 */
Result nsGetApplicationControlData(NsApplicationControlSource source, u64 application_id, NsApplicationControlData* buffer, size_t size, u64* actual_size);

/**
 * @brief Gets the \ref NsApplicationControlData for the specified application.
 * @note Only available on [19.0.0+].
 * @param[in] source Source, official sw uses ::NsApplicationControlSource_Storage.
 * @param[in] application_id ApplicationId.
 * @param[out] buffer \ref NsApplicationControlData
 * @param[in] size Size of the buffer.
 * @param[in] flag1 Default is 0. 0xFF speeds up execution.
 * @param[in] flag2 Default is 0.
 * @param[out] actual_size Actual output size.
 * @param[out] unk Returned with size, always 0.
 */
Result nsGetApplicationControlData2(NsApplicationControlSource source, u64 application_id, NsApplicationControlData* buffer, size_t size, u8 flag1, u8 flag2, u64* actual_size, u32* unk);

/**
 * @brief GetApplicationDesiredLanguage. Selects a \ref NacpLanguageEntry to use from the specified \ref NacpStruct.
 * @note Uses \ref nsGetReadOnlyApplicationControlDataInterface on [5.1.0+], otherwise IApplicationManagerInterface is used.
 * @param[in] nacp \ref NacpStruct
 * @param[out] langentry \ref NacpLanguageEntry
 */
Result nsGetApplicationDesiredLanguage(NacpStruct *nacp, NacpLanguageEntry **langentry);

///@}

///@name IECommerceInterface
///@{

/**
 * @brief RequestLinkDevice
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] uid \ref AccountUid
 */
Result nsRequestLinkDevice(AsyncResult *a, AccountUid uid);

/**
 * @brief RequestSyncRights
 * @note Only available on [6.0.0+].
 * @param[out] a \ref AsyncResult
 */
Result nsRequestSyncRights(AsyncResult *a);

/**
 * @brief RequestUnlinkDevice
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [6.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] uid \ref AccountUid
 */
Result nsRequestUnlinkDevice(AsyncResult *a, AccountUid uid);

///@}

///@name IFactoryResetInterface
///@{

/**
 * @brief ResetToFactorySettings
 * @note Uses \ref nsGetFactoryResetInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 */
Result nsResetToFactorySettings(void);

/**
 * @brief ResetToFactorySettingsWithoutUserSaveData
 * @note Uses \ref nsGetFactoryResetInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 */
Result nsResetToFactorySettingsWithoutUserSaveData(void);

/**
 * @brief ResetToFactorySettingsForRefurbishment
 * @note Uses \ref nsGetFactoryResetInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 */
Result nsResetToFactorySettingsForRefurbishment(void);

/**
 * @brief ResetToFactorySettingsWithPlatformRegion
 * @note Only available on [9.1.0+].
 */
Result nsResetToFactorySettingsWithPlatformRegion(void);

/**
 * @brief ResetToFactorySettingsWithPlatformRegionAuthentication
 * @note Only available on [9.1.0+].
 */
Result nsResetToFactorySettingsWithPlatformRegionAuthentication(void);

///@}

///@name IApplicationManagerInterface
///@{

/**
 * @brief Gets an listing of \ref NsApplicationRecord.
 * @param[out] records Output array of \ref NsApplicationRecord.
 * @param[in] count Size of the records array in entries.
 * @param[in] entry_offset Starting entry offset.
 * @param[out] out_entrycount Total output entries.
 */
Result nsListApplicationRecord(NsApplicationRecord* records, s32 count, s32 entry_offset, s32* out_entrycount);

/**
 * @brief GetApplicationRecordUpdateSystemEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result nsGetApplicationRecordUpdateSystemEvent(Event* out_event);

/**
 * @brief GetApplicationViewDeprecated
 * @note On [3.0.0+] you should generally use \ref nsGetApplicationView instead.
 * @param[out] out Output array of \ref NsApplicationViewDeprecated.
 * @param[in] application_ids Input array of ApplicationIds.
 * @param[in] count Size of the input/output arrays in entries.
 */
Result nsGetApplicationViewDeprecated(NsApplicationViewDeprecated *views, const u64 *application_ids, s32 count);

/**
 * @brief DeleteApplicationEntity
 * @param[in] application_id ApplicationId.
 */
Result nsDeleteApplicationEntity(u64 application_id);

/**
 * @brief DeleteApplicationCompletely
 * @param[in] application_id ApplicationId.
 */
Result nsDeleteApplicationCompletely(u64 application_id);

/**
 * @brief DeleteRedundantApplicationEntity
 */
Result nsDeleteRedundantApplicationEntity(void);

/**
 * @brief IsApplicationEntityMovable
 * @param[in] application_id ApplicationId.
 * @param[in] storage_id \ref NcmStorageId
 * @param[out] out Output flag.
 */
Result nsIsApplicationEntityMovable(u64 application_id, NcmStorageId storage_id, bool *out);

/**
 * @brief MoveApplicationEntity
 * @note Only available on [1.0.0-9.2.0].
 * @param[in] application_id ApplicationId.
 * @param[in] storage_id \ref NcmStorageId
 */
Result nsMoveApplicationEntity(u64 application_id, NcmStorageId storage_id);

/**
 * @brief RequestApplicationUpdateInfo
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @param[out] a \ref AsyncValue. The data that can be read from this is u8 ApplicationUpdateInfo. qlaunch just checks whether this is 0.
 * @param application_id ApplicationId.
 */
Result nsRequestApplicationUpdateInfo(AsyncValue *a, u64 application_id);

/**
 * @brief CancelApplicationDownload
 * @param[in] application_id ApplicationId.
 */
Result nsCancelApplicationDownload(u64 application_id);

/**
 * @brief ResumeApplicationDownload
 * @param[in] application_id ApplicationId.
 */
Result nsResumeApplicationDownload(u64 application_id);

/**
 * @brief CheckApplicationLaunchVersion
 * @param[in] application_id ApplicationId.
 */
Result nsCheckApplicationLaunchVersion(u64 application_id);

/**
 * @brief CalculateApplicationApplyDeltaRequiredSize
 * @param[in] application_id ApplicationId.
 * @param[out] storage_id Output \ref NcmStorageId.
 * @param[out] size Output size.
 */
Result nsCalculateApplicationDownloadRequiredSize(u64 application_id, NcmStorageId *storage_id, s64 *size);

/**
 * @brief CleanupSdCard
 */
Result nsCleanupSdCard(void);

/**
 * @brief GetSdCardMountStatusChangedEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result nsGetSdCardMountStatusChangedEvent(Event* out_event);

/**
 * @brief GetGameCardUpdateDetectionEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result nsGetGameCardUpdateDetectionEvent(Event* out_event);

/**
 * @brief DisableApplicationAutoDelete
 * @param[in] application_id ApplicationId.
 */
Result nsDisableApplicationAutoDelete(u64 application_id);

/**
 * @brief EnableApplicationAutoDelete
 * @param[in] application_id ApplicationId.
 */
Result nsEnableApplicationAutoDelete(u64 application_id);

/**
 * @brief SetApplicationTerminateResult
 * @param[in] application_id ApplicationId.
 * @param[in] res Result.
 */
Result nsSetApplicationTerminateResult(u64 application_id, Result res);

/**
 * @brief ClearApplicationTerminateResult
 * @param[in] application_id ApplicationId.
 */
Result nsClearApplicationTerminateResult(u64 application_id);

/**
 * @brief GetLastSdCardMountUnexpectedResult
 */
Result nsGetLastSdCardMountUnexpectedResult(void);

/**
 * @brief Opens a \ref NsRequestServerStopper.
 * @note Only available on [2.0.0+].
 * @param[out] r \ref NsRequestServerStopper
 */
Result nsGetRequestServerStopper(NsRequestServerStopper *r);

/**
 * @brief CancelApplicationApplyDelta
 * @note Only available on [3.0.0+].
 * @param[in] application_id ApplicationId.
 */
Result nsCancelApplicationApplyDelta(u64 application_id);

/**
 * @brief ResumeApplicationApplyDelta
 * @note Only available on [3.0.0+].
 * @param[in] application_id ApplicationId.
 */
Result nsResumeApplicationApplyDelta(u64 application_id);

/**
 * @brief CalculateApplicationApplyDeltaRequiredSize
 * @note Only available on [3.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[out] storage_id Output \ref NcmStorageId.
 * @param[out] size Output size.
 */
Result nsCalculateApplicationApplyDeltaRequiredSize(u64 application_id, NcmStorageId *storage_id, s64 *size);

/**
 * @brief ResumeAll
 * @note Only available on [3.0.0+].
 */
Result nsResumeAll(void);

/**
 * @brief Temporarily mounts the specified fs ContentStorage, then uses fs GetTotalSpaceSize/GetFreeSpaceSize with that mounted ContentStorage.
 * @note Only available on [3.0.0+].
 * @param[in] storage_id \ref NcmStorageId, must be ::NcmStorageId_BuiltInUser or ::NcmStorageId_SdCard.
 * @param[out] total_space_size Output from GetTotalSpaceSize.
 * @param[out] free_space_size Output from GetFreeSpaceSize.
 */
Result nsGetStorageSize(NcmStorageId storage_id, s64 *total_space_size, s64 *free_space_size);

/**
 * @brief RequestUpdateApplication2
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] application_id ApplicationId.
 */
Result nsRequestUpdateApplication2(AsyncResult *a, u64 application_id);

/**
 * @brief DeleteUserSaveDataAll
 * @param[in] p \ref NsProgressMonitorForDeleteUserSaveDataAll
 * @param[in] uid \ref AccountUid
 */
Result nsDeleteUserSaveDataAll(NsProgressMonitorForDeleteUserSaveDataAll *p, AccountUid uid);

/**
 * @brief DeleteUserSystemSaveData
 * @param[in] uid \ref AccountUid
 * @param[in] system_save_data_id SystemSaveDataId
 */
Result nsDeleteUserSystemSaveData(AccountUid uid, u64 system_save_data_id);

/**
 * @brief DeleteSaveData
 * @note Only available on [6.0.0+].
 * @param[in] save_data_space_id \ref FsSaveDataSpaceId
 * @param[in] save_data_id SaveDataId
 */
Result nsDeleteSaveData(FsSaveDataSpaceId save_data_space_id, u64 save_data_id);

/**
 * @brief UnregisterNetworkServiceAccount
 * @param[in] uid \ref AccountUid
 */
Result nsUnregisterNetworkServiceAccount(AccountUid uid);

/**
 * @brief UnregisterNetworkServiceAccountWithUserSaveDataDeletion
 * @note Only available on [6.0.0+].
 * @param[in] uid \ref AccountUid
 */
Result nsUnregisterNetworkServiceAccountWithUserSaveDataDeletion(AccountUid uid);

/**
 * @brief RequestDownloadApplicationControlData
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @param[out] a \ref AsyncResult
 * @param[in] application_id ApplicationId.
 */
Result nsRequestDownloadApplicationControlData(AsyncResult *a, u64 application_id);

/**
 * @brief ListApplicationTitle
 * @note The data available with \ref asyncValueGet is a s32 for the offset within the buffer where the output data is located, \ref asyncValueGetSize returns the total byte-size of the data located here. The data located here is the \ref NacpLanguageEntry for each specified ApplicationId.
 * @note Only available on [8.0.0+].
 * @param[out] a \ref AsyncValue
 * @param[in] source Source, qlaunch uses ::NsApplicationControlSource_Storage.
 * @param[in] application_ids Input array of ApplicationIds.
 * @param[in] count Size of the application_ids array in entries.
 * @param buffer 0x1000-byte aligned buffer for TransferMemory. This buffer must not be accessed until the async operation finishes.
 * @param[in] size 0x1000-byte aligned buffer size for TransferMemory. This must be at least: count*sizeof(\ref NacpLanguageEntry) + count*sizeof(u64) + count*sizeof(\ref NsApplicationControlData).
 */
Result nsListApplicationTitle(AsyncValue *a, NsApplicationControlSource source, const u64 *application_ids, s32 count, void* buffer, size_t size);

/**
 * @brief ListApplicationIcon
 * @note The data available with \ref asyncValueGet is a s32 for the offset within the buffer where the output data is located, \ref asyncValueGetSize returns the total byte-size of the data located here. This data is: an u64 for total entries, an array of u64s for each icon size, then the icon JPEGs for the specified ApplicationIds.
 * @note Only available on [8.0.0+].
 * @param[out] a \ref AsyncValue
 * @param[in] source Source.
 * @param[in] application_ids Input array of ApplicationIds.
 * @param[in] count Size of the application_ids array in entries.
 * @param buffer 0x1000-byte aligned buffer for TransferMemory. This buffer must not be accessed until the async operation finishes.
 * @param[in] size 0x1000-byte aligned buffer size for TransferMemory. This must be at least: 0x4 + count*sizeof(u64) + count*sizeof(\ref NsApplicationControlData::icon) + count*sizeof(u64) + sizeof(\ref NsApplicationControlData).
 */
Result nsListApplicationIcon(AsyncValue *a, NsApplicationControlSource source, const u64 *application_ids, s32 count, void* buffer, size_t size);

/**
 * @brief RequestCheckGameCardRegistration
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [2.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] application_id ApplicationId.
 */
Result nsRequestCheckGameCardRegistration(AsyncResult *a, u64 application_id);

/**
 * @brief RequestGameCardRegistrationGoldPoint
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [2.0.0+].
 * @param[out] a \ref AsyncValue. The data that can be read from this is 4-bytes.
 * @param[in] uid \ref AccountUid
 * @param[in] application_id ApplicationId.
 */
Result nsRequestGameCardRegistrationGoldPoint(AsyncValue *a, AccountUid uid, u64 application_id);

/**
 * @brief RequestRegisterGameCard
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [2.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] uid \ref AccountUid
 * @param[in] application_id ApplicationId.
 * @param[in] inval Input value.
 */
Result nsRequestRegisterGameCard(AsyncResult *a, AccountUid uid, u64 application_id, s32 inval);

/**
 * @brief GetGameCardMountFailureEvent
 * @note The Event must be closed by the user once finished with it.
 * @note Only available on [3.0.0+].
 * @param[out] out_event Output Event with autoclear=false.
 */
Result nsGetGameCardMountFailureEvent(Event* out_event);

/**
 * @brief IsGameCardInserted
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result nsIsGameCardInserted(bool *out);

/**
 * @brief EnsureGameCardAccess
 * @note Only available on [3.0.0+].
 */
Result nsEnsureGameCardAccess(void);

/**
 * @brief GetLastGameCardMountFailureResult
 * @note Only available on [3.0.0+].
 */
Result nsGetLastGameCardMountFailureResult(void);

/**
 * @brief ListApplicationIdOnGameCard
 * @note Only available on [5.0.0+].
 * @param[out] application_ids Output array of ApplicationIds.
 * @param[in] count Size of the application_ids array in entries.
 * @param[out] total_out Total output entries.
 */
Result nsListApplicationIdOnGameCard(u64 *application_ids, s32 count, s32 *total_out);

/**
 * @brief TouchApplication
 * @note Only available on [2.0.0+].
 * @param[in] application_id ApplicationId.
 */
Result nsTouchApplication(u64 application_id);

/**
 * @brief IsApplicationUpdateRequested
 * @note Only available on [2.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[out] flag Output flag, indicating whether out is valid.
 * @param[out] out Output value.
 */
Result nsIsApplicationUpdateRequested(u64 application_id, bool *flag, u32 *out);

/**
 * @brief WithdrawApplicationUpdateRequest
 * @note Only available on [2.0.0+].
 * @param[in] application_id ApplicationId.
 */
Result nsWithdrawApplicationUpdateRequest(u64 application_id);

/**
 * @brief RequestVerifyAddOnContentsRights
 * @note Only available on [3.0.0-9.2.0].
 * @param[out] a \ref NsProgressAsyncResult
 * @param[in] application_id ApplicationId.
 */
Result nsRequestVerifyAddOnContentsRights(NsProgressAsyncResult *a, u64 application_id);

/**
 * @brief RequestVerifyApplication
 * @note On pre-5.0.0 this uses cmd RequestVerifyApplicationDeprecated, otherwise cmd RequestVerifyApplication is used.
 * @param[out] a \ref NsProgressAsyncResult. The data available with \ref nsProgressAsyncResultGetProgress is basically the same as \ref NsSystemUpdateProgress.
 * @param[in] application_id ApplicationId.
 * @param[in] unk Unknown. A default value of 0x7 can be used (which is what qlaunch uses). Only used on [5.0.0+].
 * @param buffer 0x1000-byte aligned buffer for TransferMemory. This buffer must not be accessed until the async operation finishes.
 * @param[in] size 0x1000-byte aligned buffer size for TransferMemory. qlaunch uses size 0x100000.
 */
Result nsRequestVerifyApplication(NsProgressAsyncResult *a, u64 application_id, u32 unk, void* buffer, size_t size);

/**
 * @brief IsAnyApplicationEntityInstalled
 * @note Only available on [2.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[out] out Output flag.
 */
Result nsIsAnyApplicationEntityInstalled(u64 application_id, bool *out);

/**
 * @brief CleanupUnavailableAddOnContents
 * @note Only available on [6.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[in] uid \ref AccountUid
 */
Result nsCleanupUnavailableAddOnContents(u64 application_id, AccountUid uid);

/**
 * @brief EstimateSizeToMove
 * @note Only available on [10.0.0+].
 * @param[in] storage_ids Array of u8 \ref NcmStorageId.
 * @param[in] count Size of the storage_ids array in entries.
 * @param[in] storage_id storage_id \ref NcmStorageId
 * @param[in] flags Flags
 * @param[in] application_id ApplicationId.
 * @param[out] Out Output value.
 */
Result nsEstimateSizeToMove(u8 *storage_ids, s32 count, NcmStorageId storage_id, u32 flags, u64 application_id, s64 *out);

/**
 * @brief FormatSdCard
 * @note Only available on [2.0.0+].
 */
Result nsFormatSdCard(void);

/**
 * @brief NeedsSystemUpdateToFormatSdCard
 * @note Only available on [2.0.0+].
 * @param[out] out Output flag.
 */
Result nsNeedsSystemUpdateToFormatSdCard(bool *out);

/**
 * @brief GetLastSdCardFormatUnexpectedResult
 * @note Only available on [2.0.0+].
 */
Result nsGetLastSdCardFormatUnexpectedResult(void);

/**
 * @brief GetApplicationView
 * @note Only available on [3.0.0+], on prior system-versions use \ref nsGetApplicationViewDeprecated instead.
 * @param[out] out Output array of \ref NsApplicationView.
 * @param[in] application_ids Input array of ApplicationIds.
 * @param[in] count Size of the input/output arrays in entries.
 */
Result nsGetApplicationView(NsApplicationView *views, const u64 *application_ids, s32 count);

/**
 * @brief GetApplicationViewDownloadErrorContext
 * @note Only available on [4.0.0+].
 * @param[in] application_id ApplicationId
 * @param[out] context \ref ErrorContext
 */
Result nsGetApplicationViewDownloadErrorContext(u64 application_id, ErrorContext *context);

/**
 * @brief GetApplicationViewWithPromotionInfo
 * @note Only available on [8.0.0+].
 * @param[out] out Output array of \ref NsApplicationViewWithPromotionInfo.
 * @param[in] application_ids Input array of ApplicationIds.
 * @param[in] count Size of the input/output arrays in entries.
 */
Result nsGetApplicationViewWithPromotionInfo(NsApplicationViewWithPromotionInfo *out, const u64 *application_ids, s32 count);

/**
 * @brief RequestDownloadApplicationPrepurchasedRights
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] application_id ApplicationId.
 */
Result nsRequestDownloadApplicationPrepurchasedRights(AsyncResult *a, u64 application_id);

/**
 * @brief Generates a \ref NsSystemDeliveryInfo using the currently installed SystemUpdate meta.
 * @note Only available on [4.0.0+].
 * @param[out] info \ref NsSystemDeliveryInfo
 */
Result nsGetSystemDeliveryInfo(NsSystemDeliveryInfo *info);

/**
 * @brief SelectLatestSystemDeliveryInfo
 * @note This selects the \ref NsSystemDeliveryInfo with the latest version from sys_list, using minimum versions determined from app_list/state and base_info, etc.
 * @note Only available on [4.0.0+].
 * @param[in] sys_list Input array of \ref NsSystemDeliveryInfo.
 * @param[in] sys_count Size of the sys_list array in entries.
 * @param[in] base_info \ref NsSystemDeliveryInfo
 * @param[in] app_list Input array of \ref NsApplicationDeliveryInfo. This can be NULL.
 * @param[in] app_count Size of the app_list array in entries. This can be 0.
 * @param[out] index Output index for the selected entry in sys_list, -1 if none found.
 */
Result nsSelectLatestSystemDeliveryInfo(const NsSystemDeliveryInfo *sys_list, s32 sys_count, const NsSystemDeliveryInfo *base_info, const NsApplicationDeliveryInfo *app_list, s32 app_count, s32 *index);

/**
 * @brief VerifyDeliveryProtocolVersion
 * @note Only available on [4.0.0+].
 * @param[in] info \ref NsSystemDeliveryInfo
 */
Result nsVerifyDeliveryProtocolVersion(const NsSystemDeliveryInfo *info);

/**
 * @brief Generates \ref NsApplicationDeliveryInfo for the specified ApplicationId.
 * @note Only available on [4.0.0+].
 * @param[out] info Output array of \ref NsApplicationDeliveryInfo.
 * @param[in] count Size of the array in entries.
 * @param[in] application_id ApplicationId
 * @param[in] attr ApplicationDeliveryAttributeTag bitmask.
 * @param[out] total_out Total output entries.
 */
Result nsGetApplicationDeliveryInfo(NsApplicationDeliveryInfo *info, s32 count, u64 application_id, u32 attr, s32 *total_out);

/**
 * @brief HasAllContentsToDeliver
 * @note Only available on [4.0.0+].
 * @param[in] info Input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count Size of the array in entries. Must be value 1.
 * @param[out] out Output flag.
 */
Result nsHasAllContentsToDeliver(const NsApplicationDeliveryInfo* info, s32 count, bool *out);

/**
 * @brief Both \ref NsApplicationDeliveryInfo are validated, then the application_version in the first/second \ref NsApplicationDeliveryInfo are compared.
 * @note Only available on [4.0.0+].
 * @param[in] info0 First input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count0 Size of the info0 array in entries. Must be value 1.
 * @param[in] info1 Second input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count1 Size of the info1 array in entries. Must be value 1.
 * @param[out] out Comparison result: -1 for less than, 0 for equal, and 1 for higher than.
 */
Result nsCompareApplicationDeliveryInfo(const NsApplicationDeliveryInfo *info0, s32 count0, const NsApplicationDeliveryInfo *info1, s32 count1, s32 *out);

/**
 * @brief CanDeliverApplication
 * @note Only available on [4.0.0+].
 * @param[in] info0 First input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count0 Size of the info0 array in entries. Must be value <=1, when 0 this will return 0 with out set to 0.
 * @param[in] info1 Second input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count1 Size of the info1 array in entries. Must be value 1.
 * @param[out] out Output flag.
 */
Result nsCanDeliverApplication(const NsApplicationDeliveryInfo *info0, s32 count0, const NsApplicationDeliveryInfo *info1, s32 count1, bool *out);

/**
 * @brief ListContentMetaKeyToDeliverApplication
 * @note Only available on [4.0.0+].
 * @param[out] meta Output array of \ref NcmContentMetaKey.
 * @param[in] meta_count Size of the meta array in entries. Must be at least 1, only 1 entry will be returned.
 * @param[in] meta_index Meta entry index. An output \ref NcmContentMetaKey will not be returned when this value is larger than 0.
 * @param[in] info Input array of \ref NsApplicationDeliveryInfo.
 * @param[in] info_count Size of the info array in entries. Must be value 1.
 * @param[out] total_out Total output entries.
 */
Result nsListContentMetaKeyToDeliverApplication(NcmContentMetaKey *meta, s32 meta_count, s32 meta_index, const NsApplicationDeliveryInfo *info, s32 info_count, s32 *total_out);

/**
 * @brief After validation, this sets the output bool by comparing system-version fields in the \ref NsSystemDeliveryInfo / info-array and with a state field, etc.
 * @note Only available on [4.0.0+].
 * @param[in] info Input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count Size of the info array in entries. Must be value 1.
 * @param[in] sys_info \ref NsSystemDeliveryInfo
 * @param[out] out Output flag.
 */
Result nsNeedsSystemUpdateToDeliverApplication(const NsApplicationDeliveryInfo *info, s32 count, const NsSystemDeliveryInfo *sys_info, bool *out);

/**
 * @brief EstimateRequiredSize
 * @note Only available on [4.0.0+].
 * @param[in] meta Input array of \ref NcmContentMetaKey.
 * @param[in] count Size of the meta array in entries. When less than 1, this will return 0 with out set to 0.
 * @param[out] out Output size.
 */
Result nsEstimateRequiredSize(const NcmContentMetaKey *meta, s32 count, s64 *out);

/**
 * @brief RequestReceiveApplication
 * @note This is the Application version of \ref nssuControlRequestReceiveSystemUpdate, see the notes for that.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] addr Server IPv4 address.
 * @param[in] port Socket port. qlaunch uses value 55556.
 * @param[in] application_id ApplicationId
 * @param[in] meta Input array of \ref NcmContentMetaKey. The ::NcmContentMetaType must match ::NcmContentMetaType_Patch.
 * @param[in] count Size of the meta array in entries.
 * @param[in] storage_id \ref NcmStorageId. qlaunch uses ::NcmStorageId_Any.
 */
Result nsRequestReceiveApplication(AsyncResult *a, u32 addr, u16 port, u64 application_id, const NcmContentMetaKey *meta, s32 count, NcmStorageId storage_id);

/**
 * @brief CommitReceiveApplication
 * @note Only available on [4.0.0+].
 * @param[in] application_id ApplicationId
 */
Result nsCommitReceiveApplication(u64 application_id);

/**
 * @brief GetReceiveApplicationProgress
 * @note Only available on [4.0.0+].
 * @param[in] application_id ApplicationId
 * @param[out] out \ref NsReceiveApplicationProgress
 */
Result nsGetReceiveApplicationProgress(u64 application_id, NsReceiveApplicationProgress *out);

/**
 * @brief RequestSendApplication
 * @note This is the Application version of \ref nssuRequestSendSystemUpdate, see the notes for that.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] addr Client IPv4 address.
 * @param[in] port Socket port. qlaunch uses value 55556.
 * @param[in] application_id ApplicationId
 * @param[in] meta Input array of \ref NcmContentMetaKey. The ::NcmContentMetaType must match ::NcmContentMetaType_Patch.
 * @param[in] count Size of the meta array in entries.
 */
Result nsRequestSendApplication(AsyncResult *a, u32 addr, u16 port, u64 application_id, const NcmContentMetaKey *meta, s32 count);

/**
 * @brief GetSendApplicationProgress
 * @note Only available on [4.0.0+].
 * @param[in] application_id ApplicationId
 * @param[out] out \ref NsSendApplicationProgress
 */
Result nsGetSendApplicationProgress(u64 application_id, NsSendApplicationProgress *out);

/**
 * @brief Both \ref NsSystemDeliveryInfo are validated, then the system_update_version in the first/second \ref NsSystemDeliveryInfo are compared.
 * @note Only available on [4.0.0+].
 * @param[in] info0 First \ref NsSystemDeliveryInfo.
 * @param[in] info1 Second \ref NsSystemDeliveryInfo.
 * @param[out] out Comparison result: -1 for less than, 0 for equal, and 1 for higher than.
 */
Result nsCompareSystemDeliveryInfo(const NsSystemDeliveryInfo *info0, const NsSystemDeliveryInfo *info1, s32 *out);

/**
 * @brief ListNotCommittedContentMeta
 * @note Only available on [4.0.0+].
 * @param[out] meta Output array of \ref NcmContentMetaKey.
 * @param[in] count Size of the meta array in entries.
 * @param[in] application_id ApplicationId
 * @param[in] unk Unknown.
 * @param[out] total_out Total output entries.
 */
Result nsListNotCommittedContentMeta(NcmContentMetaKey *meta, s32 count, u64 application_id, s32 unk, s32 *total_out);

/**
 * @brief This extracts data from the input array for hashing with SHA256, with validation being done when handling each entry.
 * @note Only available on [5.0.0+].
 * @param[in] info Input array of \ref NsApplicationDeliveryInfo.
 * @param[in] count Size of the array in entries.
 * @param[out] out_hash Output 0x20-byte SHA256 hash.
 */
Result nsGetApplicationDeliveryInfoHash(const NsApplicationDeliveryInfo *info, s32 count, u8 *out_hash);

/**
 * @brief GetApplicationTerminateResult
 * @note Only available on [6.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[out] res Output Result.
 */
Result nsGetApplicationTerminateResult(u64 application_id, Result *res);

/**
 * @brief GetApplicationRightsOnClient
 * @note Only available on [6.0.0+].
 * @param[out] rights Output array of \ref NsApplicationRightsOnClient.
 * @param[in] count Size of the rights array in entries. qlaunch uses value 3 for this.
 * @param[in] application_id ApplicationId
 * @param[in] uid \ref AccountUid, can optionally be all-zero.
 * @param[in] flags Flags. Official sw hard-codes this to value 0x3.
 * @param[out] total_out Total output entries.
 */
Result nsGetApplicationRightsOnClient(NsApplicationRightsOnClient *rights, s32 count, u64 application_id, AccountUid uid, u32 flags, s32 *total_out);

/**
 * @brief RequestNoDownloadRightsErrorResolution
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [9.0.0+].
 * @param[out] a \ref AsyncValue. The data that can be read from this is u8 NoDownloadRightsErrorResolution.
 * @param application_id ApplicationId.
 */
Result nsRequestNoDownloadRightsErrorResolution(AsyncValue *a, u64 application_id);

/**
 * @brief RequestResolveNoDownloadRightsError
 * @note \ref nifmInitialize must be used prior to this. Before using the cmd, this calls \ref nifmIsAnyInternetRequestAccepted with the output from \ref nifmGetClientId, an error is returned when that returns false.
 * @note Only available on [9.0.0+].
 * @param[out] a \ref AsyncValue. The data that can be read from this is u8 NoDownloadRightsErrorResolution.
 * @param application_id ApplicationId.
 */
Result nsRequestResolveNoDownloadRightsError(AsyncValue *a, u64 application_id);

/**
 * @brief GetPromotionInfo
 * @note Only available on [8.0.0+].
 * @param[out] promotion \ref NsPromotionInfo
 * @param application_id ApplicationId.
 * @param[in] uid \ref AccountUid
 */
Result nsGetPromotionInfo(NsPromotionInfo *promotion, u64 application_id, AccountUid uid);

///@}

///@name IDownloadTaskInterface
///@{

/**
 * @brief ClearTaskStatusList
 * @note Uses \ref nsGetDownloadTaskInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 */
Result nsClearTaskStatusList(void);

/**
 * @brief RequestDownloadTaskList
 * @note Uses \ref nsGetDownloadTaskInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 */
Result nsRequestDownloadTaskList(void);

/**
 * @brief RequestEnsureDownloadTask
 * @note Uses \ref nsGetDownloadTaskInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 * @param[out] a \ref AsyncResult
 */
Result nsRequestEnsureDownloadTask(AsyncResult *a);

/**
 * @brief ListDownloadTaskStatus
 * @note Uses \ref nsGetDownloadTaskInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 * @param[out] tasks Output array of \ref NsDownloadTaskStatus.
 * @param[in] count Size of the tasks array in entries. A maximum of 0x100 tasks can be stored in state.
 * @param[out] total_out Total output entries.
 */
Result nsListDownloadTaskStatus(NsDownloadTaskStatus* tasks, s32 count, s32 *total_out);

/**
 * @brief RequestDownloadTaskListData
 * @note Uses \ref nsGetDownloadTaskInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 * @param[out] a \ref AsyncValue
 */
Result nsRequestDownloadTaskListData(AsyncValue *a);

/**
 * @brief TryCommitCurrentApplicationDownloadTask
 * @note Only available on [4.0.0+].
 */
Result nsTryCommitCurrentApplicationDownloadTask(void);

/**
 * @brief EnableAutoCommit
 * @note Only available on [4.0.0+].
 */
Result nsEnableAutoCommit(void);

/**
 * @brief DisableAutoCommit
 * @note Only available on [4.0.0+].
 */
Result nsDisableAutoCommit(void);

/**
 * @brief TriggerDynamicCommitEvent
 * @note Only available on [4.0.0+].
 */
Result nsTriggerDynamicCommitEvent(void);

///@}

///@name IContentManagementInterface
///@{

/**
 * @brief CalculateApplicationOccupiedSize
 * @note Uses \ref nsGetContentManagementInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @param[in] application_id ApplicationId.
 * @param[out] out \ref NsApplicationOccupiedSize
 */
Result nsCalculateApplicationOccupiedSize(u64 application_id, NsApplicationOccupiedSize *out);

/**
 * @brief CheckSdCardMountStatus
 * @note Uses \ref nsGetContentManagementInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 */
Result nsCheckSdCardMountStatus(void);

/**
 * @brief Returns the total storage capacity (used + free) from content manager services.
 * @note Uses \ref nsGetContentManagementInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @param[in] storage_id \ref NcmStorageId. Must be ::NcmStorageId_SdCard.
 * @param[out] size Pointer to output the total storage size to.
 */
Result nsGetTotalSpaceSize(NcmStorageId storage_id, s64 *size);

/**
 * @brief Returns the available storage capacity from content manager services.
 * @note Uses \ref nsGetContentManagementInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @param[in] storage_id \ref NcmStorageId. Must be ::NcmStorageId_SdCard.
 * @param[out] size Pointer to output the free storage size to.
 */
Result nsGetFreeSpaceSize(NcmStorageId storage_id, s64 *size);

/**
 * @brief CountApplicationContentMeta
 * @note Uses \ref nsGetContentManagementInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[out] out Output count.
 */
Result nsCountApplicationContentMeta(u64 application_id, s32 *out);

/**
 * @brief Gets an listing of \ref NsApplicationContentMetaStatus.
 * @note Uses \ref nsGetContentManagementInterface on [3.0.0+], otherwise IApplicationManagerInterface is used.
 * @note Only available on [2.0.0+].
 * @param[in] application_id ApplicationId.
 * @param[in] index Starting entry index.
 * @param[out] list Output array of \ref NsApplicationContentMetaStatus.
 * @param[in] count Size of the list array in entries.
 * @param[out] out_entrycount Total output entries.
 */
Result nsListApplicationContentMetaStatus(u64 application_id, s32 index, NsApplicationContentMetaStatus* list, s32 count, s32* out_entrycount);

/**
 * @brief IsAnyApplicationRunning
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result nsIsAnyApplicationRunning(bool *out);

///@}

///@name IRequestServerStopper
///@{

/**
 * @brief Close a \ref NsRequestServerStopper.
 * @param r \ref NsRequestServerStopper
 */
void nsRequestServerStopperClose(NsRequestServerStopper *r);

///@}

///@name IProgressMonitorForDeleteUserSaveDataAll
///@{

/**
 * @brief Close a \ref NsProgressMonitorForDeleteUserSaveDataAll. When initialized this will use \ref nsProgressMonitorForDeleteUserSaveDataAllIsFinished, throwing errors on failure / when the operation isn't finished (without closing the object).
 * @note Cancelling the operation before it's finished is not supported by \ref NsProgressMonitorForDeleteUserSaveDataAll.
 * @param p \ref NsProgressMonitorForDeleteUserSaveDataAll
 */
Result nsProgressMonitorForDeleteUserSaveDataAllClose(NsProgressMonitorForDeleteUserSaveDataAll *p);

/**
 * @brief GetSystemEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result nsProgressMonitorForDeleteUserSaveDataAllGetSystemEvent(NsProgressMonitorForDeleteUserSaveDataAll *p, Event* out_event);

/**
 * @brief IsFinished
 * @param p \ref NsProgressMonitorForDeleteUserSaveDataAll
 * @param[out] out Whether the operation finished.
 */
Result nsProgressMonitorForDeleteUserSaveDataAllIsFinished(NsProgressMonitorForDeleteUserSaveDataAll *p, bool *out);

/**
 * @brief GetResult
 * @param p \ref NsProgressMonitorForDeleteUserSaveDataAll
 */
Result nsProgressMonitorForDeleteUserSaveDataAllGetResult(NsProgressMonitorForDeleteUserSaveDataAll *p);

/**
 * @brief GetProgress
 * @param p \ref NsProgressMonitorForDeleteUserSaveDataAll
 * @param[out] progress Output \ref NsProgressForDeleteUserSaveDataAll.
 */
Result nsProgressMonitorForDeleteUserSaveDataAllGetProgress(NsProgressMonitorForDeleteUserSaveDataAll *p, NsProgressForDeleteUserSaveDataAll *progress);

///@}

///@name IProgressAsyncResult
///@{

/**
 * @brief Close a \ref NsProgressAsyncResult.
 * @note When the object is initialized, this uses \ref nsProgressAsyncResultCancel then \ref nsProgressAsyncResultWait with timeout=UINT64_MAX.
 * @param a \ref NsProgressAsyncResult
 */
void nsProgressAsyncResultClose(NsProgressAsyncResult *a);

/**
 * @brief Waits for the async operation to finish using the specified timeout.
 * @param a \ref NsProgressAsyncResult
 * @param[in] timeout Timeout in nanoseconds. UINT64_MAX for no timeout.
 */
Result nsProgressAsyncResultWait(NsProgressAsyncResult *a, u64 timeout);

/**
 * @brief Gets the Result.
 * @note Prior to using the cmd, this uses \ref nsProgressAsyncResultWait with timeout=UINT64_MAX.
 * @param a \ref NsProgressAsyncResult
 */
Result nsProgressAsyncResultGet(NsProgressAsyncResult *a);

/**
 * @brief Cancels the async operation.
 * @note Used automatically by \ref nsProgressAsyncResultClose.
 * @param a \ref NsProgressAsyncResult
 */
Result nsProgressAsyncResultCancel(NsProgressAsyncResult *a);

/**
 * @brief Gets the progress.
 * @param a \ref NsProgressAsyncResult
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size.
 */
Result nsProgressAsyncResultGetProgress(NsProgressAsyncResult *a, void* buffer, size_t size);

/**
 * @brief GetDetailResult
 * @param a \ref NsProgressAsyncResult
 */
Result nsProgressAsyncResultGetDetailResult(NsProgressAsyncResult *a);

/**
 * @brief Gets the \ref ErrorContext.
 * @note Only available on [4.0.0+].
 * @param a \ref NsProgressAsyncResult
 * @param[out] context \ref ErrorContext
 */
Result nsProgressAsyncResultGetErrorContext(NsProgressAsyncResult *a, ErrorContext *context);

///@}

///@name ns:vm
///@{

/// Initialize ns:vm. On pre-3.0.0 this must be used with \ref nsInitialize.
Result nsvmInitialize(void);

/// Exit ns:vm.
void nsvmExit(void);

/// Gets the Service object for ns:vm. This is only initialized on [3.0.0+].
Service* nsvmGetServiceSession(void);

Result nsvmNeedsUpdateVulnerability(bool *out);
Result nsvmGetSafeSystemVersion(NcmContentMetaKey *out); ///< [4.0.0+]

///@}

///@name ns:dev
///@{

/// Initialize ns:dev.
Result nsdevInitialize(void);

/// Initialize ns:dev.
void nsdevExit(void);

/// Gets the Service object for ns:dev.
Service* nsdevGetServiceSession(void);

Result nsdevLaunchProgram(u64* out_pid, const NsLaunchProperties* properties, u32 flags); ///< [1.0.0-9.2.0]
Result nsdevTerminateProcess(u64 pid);
Result nsdevTerminateProgram(u64 tid); ///< [1.0.0-9.2.0]
Result nsdevGetShellEvent(Event* out_event); ///< Autoclear for nsdevShellEvent is always true. [1.0.0-9.2.0]
Result nsdevGetShellEventInfo(NsShellEventInfo* out); ///< [1.0.0-9.2.0]
Result nsdevTerminateApplication(void);
Result nsdevPrepareLaunchProgramFromHost(NsLaunchProperties* out, const char* path, size_t path_len); ///< [1.0.0-9.2.0]
Result nsdevLaunchApplicationForDevelop(u64* out_pid, u64 application_id, u32 flags); ///< [1.0.0-9.2.0]
Result nsdevLaunchApplicationFromHost(u64* out_pid, const char* path, size_t path_len, u32 flags); ///< [10.0.0-17.0.1]
Result nsdevLaunchApplicationWithStorageIdForDevelop(u64* out_pid, u64 application_id, u32 flags, u8 app_storage_id, u8 patch_storage_id); ///< [1.0.0-17.0.1]
Result nsdevIsSystemMemoryResourceLimitBoosted(bool* out); ///< [6.0.0-8.1.0]
Result nsdevGetRunningApplicationProcessIdForDevelop(u64* out_pid); ///< [6.0.0+]
Result nsdevSetCurrentApplicationRightsEnvironmentCanBeActiveForDevelop(bool can_be_active); ///< [6.0.0+]

///@}

///@name ns:su
///@{

/// Initialize ns:su.
Result nssuInitialize(void);

/// Exit ns:su.
void nssuExit(void);

/// Gets the Service object for ns:su.
Service* nssuGetServiceSession(void);

/**
 * @brief Gets the \ref NsBackgroundNetworkUpdateState.
 * @note Internally this uses nim commands ListSystemUpdateTask and GetSystemUpdateTaskInfo to determine the output state.
 * @param[out] out \ref NsBackgroundNetworkUpdateState
 */
Result nssuGetBackgroundNetworkUpdateState(NsBackgroundNetworkUpdateState *out);

/**
 * @brief Opens a \ref NsSystemUpdateControl.
 * @note Only 1 \ref NsSystemUpdateControl can be open at a time.
 * @param[out] c \ref NsSystemUpdateControl
 */
Result nssuOpenSystemUpdateControl(NsSystemUpdateControl *c);

/**
 * @brief Uses nim ListSystemUpdateTask, then uses the task with DestroySystemUpdateTask if it exists. Then this runs ExFat handling, updates state, and sets the same state flag as \ref nssuRequestBackgroundNetworkUpdate.
 * @note Only usable when a \ref NsSystemUpdateControl isn't open.
 */
Result nssuNotifyExFatDriverRequired(void);

/**
 * @brief ClearExFatDriverStatusForDebug
 */
Result nssuClearExFatDriverStatusForDebug(void);

/**
 * @brief RequestBackgroundNetworkUpdate
 * @note Only usable when a \ref NsSystemUpdateControl isn't open.
 */
Result nssuRequestBackgroundNetworkUpdate(void);

/**
 * @brief This checks whether a sysupdate is needed with the input \ref NcmContentMetaKey using NCM commands, if not this will just return 0. Otherwise, this will then run code which is identical to \ref nssuRequestBackgroundNetworkUpdate.
 * @note Only usable when a \ref NsSystemUpdateControl isn't open.
 * @param[in] key \ref NcmContentMetaKey
 */
Result nssuNotifyBackgroundNetworkUpdate(const NcmContentMetaKey *key);

/**
 * @brief NotifyExFatDriverDownloadedForDebug
 */
Result nssuNotifyExFatDriverDownloadedForDebug(void);

/**
 * @brief Gets an Event which can be signaled by \ref nssuNotifySystemUpdateForContentDelivery.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result nssuGetSystemUpdateNotificationEventForContentDelivery(Event* out_event);

/**
 * @brief Signals the event returned by \ref nssuGetSystemUpdateNotificationEventForContentDelivery.
 */
Result nssuNotifySystemUpdateForContentDelivery(void);

/**
 * @brief This does shutdown preparation.
 * @note This is used by am-sysmodule, so generally there's no need to use this.
 * @note Only available on [3.0.0+].
 */
Result nssuPrepareShutdown(void);

/**
 * @brief This uses nim ListSystemUpdateTask, then when a task is returned uses it with DestroySystemUpdateTask.
 * @note Only available on [4.0.0+].
 */
Result nssuDestroySystemUpdateTask(void);

/**
 * @brief RequestSendSystemUpdate
 * @note addr/port are little-endian.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] addr Client IPv4 address.
 * @param[in] port Socket port. qlaunch uses value 55556.
 * @param[in] info \ref NsSystemDeliveryInfo
 */
Result nssuRequestSendSystemUpdate(AsyncResult *a, u32 addr, u16 port, NsSystemDeliveryInfo *info);

/**
 * @brief GetSendSystemUpdateProgress
 * @note Only available on [4.0.0+].
 * @param[out] out \ref NsSystemUpdateProgress
 */
Result nssuGetSendSystemUpdateProgress(NsSystemUpdateProgress *out);

///@}

///@name ISystemUpdateControl
///@{

/**
 * @brief Close a \ref NsSystemUpdateControl.
 * @param c \ref NsSystemUpdateControl
 */
void nssuControlClose(NsSystemUpdateControl *c);

/**
 * @brief Gets whether a network sysupdate was downloaded, with install pending.
 * @param c \ref NsSystemUpdateControl
 * @param[out] out Output flag.
 */
Result nssuControlHasDownloaded(NsSystemUpdateControl *c, bool* out);

/**
 * @brief RequestCheckLatestUpdate
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncValue. The data that can be read from this is u8 \ref NsLatestSystemUpdate.
 */
Result nssuControlRequestCheckLatestUpdate(NsSystemUpdateControl *c, AsyncValue *a);

/**
 * @brief RequestDownloadLatestUpdate
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncResult
 */
Result nssuControlRequestDownloadLatestUpdate(NsSystemUpdateControl *c, AsyncResult *a);

/**
 * @brief GetDownloadProgress
 * @param c \ref NsSystemUpdateControl
 * @param[out] out \ref NsSystemUpdateProgress
 */
Result nssuControlGetDownloadProgress(NsSystemUpdateControl *c, NsSystemUpdateProgress *out);

/**
 * @brief ApplyDownloadedUpdate
 * @param c \ref NsSystemUpdateControl
 */
Result nssuControlApplyDownloadedUpdate(NsSystemUpdateControl *c);

/**
 * @brief RequestPrepareCardUpdate
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncResult
 */
Result nssuControlRequestPrepareCardUpdate(NsSystemUpdateControl *c, AsyncResult *a);

/**
 * @brief GetPrepareCardUpdateProgress
 * @note \ref nssuControlSetupCardUpdate / \ref nssuControlSetupCardUpdateViaSystemUpdater must have been used at some point prior to using this.
 * @param c \ref NsSystemUpdateControl
 * @param[out] out \ref NsSystemUpdateProgress
 */
Result nssuControlGetPrepareCardUpdateProgress(NsSystemUpdateControl *c, NsSystemUpdateProgress *out);

/**
 * @brief HasPreparedCardUpdate
 * @note \ref nssuControlSetupCardUpdate / \ref nssuControlSetupCardUpdateViaSystemUpdater must have been used at some point prior to using this.
 * @param c \ref NsSystemUpdateControl
 * @param[out] out Output flag.
 */
Result nssuControlHasPreparedCardUpdate(NsSystemUpdateControl *c, bool* out);

/**
 * @brief ApplyCardUpdate
 * @note \ref nssuControlSetupCardUpdate / \ref nssuControlSetupCardUpdateViaSystemUpdater must have been used at some point prior to using this.
 * @param c \ref NsSystemUpdateControl
 */
Result nssuControlApplyCardUpdate(NsSystemUpdateControl *c);

/**
 * @brief Gets the filesize for the specified DownloadedEulaData.
 * @note This mounts the Eula SystemData, then uses the file "<mountname>:/<input path>".
 * @param c \ref NsSystemUpdateControl
 * @param[in] path EulaData path.
 * @param[out] filesize Output filesize.
 */
Result nssuControlGetDownloadedEulaDataSize(NsSystemUpdateControl *c, const char* path, u64 *filesize);

/**
 * @brief Gets the specified DownloadedEulaData.
 * @note See the note for \ref nssuControlGetDownloadedEulaDataSize.
 * @param c \ref NsSystemUpdateControl
 * @param[in] path EulaData path.
 * @param[out] buffer Output buffer.
 * @param[in] size Size of the output buffer, must be at least the output size from \ref nssuControlGetDownloadedEulaDataSize.
 * @param[out] filesize Output filesize.
 */
Result nssuControlGetDownloadedEulaData(NsSystemUpdateControl *c, const char* path, void* buffer, size_t size, u64 *filesize);

/**
 * @brief SetupCardUpdate
 * @param c \ref NsSystemUpdateControl
 * @param[in] buffer TransferMemory buffer, when NULL this is automatically allocated.
 * @param[in] size TransferMemory buffer size, see \ref NSSU_CARDUPDATE_TMEM_SIZE_DEFAULT.
 */
Result nssuControlSetupCardUpdate(NsSystemUpdateControl *c, void* buffer, size_t size);

/**
 * @brief Gets the filesize for the specified PreparedCardUpdateEulaData.
 * @note See the note for \ref nssuControlGetDownloadedEulaDataSize.
 * @param c \ref NsSystemUpdateControl
 * @param[in] path EulaData path.
 * @param[out] filesize Output filesize.
 */
Result nssuControlGetPreparedCardUpdateEulaDataSize(NsSystemUpdateControl *c, const char* path, u64 *filesize);

/**
 * @brief Gets the specified PreparedCardUpdateEulaData.
 * @note See the note for \ref nssuControlGetDownloadedEulaDataSize.
 * @param c \ref NsSystemUpdateControl
 * @param[in] path EulaData path.
 * @param[out] buffer Output buffer.
 * @param[in] size Size of the output buffer, must be at least the output size from \ref nssuControlGetPreparedCardUpdateEulaDataSize.
 * @param[out] filesize Output filesize.
 */
Result nssuControlGetPreparedCardUpdateEulaData(NsSystemUpdateControl *c, const char* path, void* buffer, size_t size, u64 *filesize);

/**
 * @brief SetupCardUpdateViaSystemUpdater
 * @note Same as \ref nssuControlSetupCardUpdate, except this doesn't run the code for fs cmds GetGameCardHandle/GetGameCardUpdatePartitionInfo, and uses fs OpenRegisteredUpdatePartition instead of OpenGameCardFileSystem.
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[in] buffer TransferMemory buffer, when NULL this is automatically allocated.
 * @param[in] size TransferMemory buffer size, see \ref NSSU_CARDUPDATE_TMEM_SIZE_DEFAULT.
 */
Result nssuControlSetupCardUpdateViaSystemUpdater(NsSystemUpdateControl *c, void* buffer, size_t size);

/**
 * @brief HasReceived
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[out] out Output flag.
 */
Result nssuControlHasReceived(NsSystemUpdateControl *c, bool* out);

/**
 * @brief RequestReceiveSystemUpdate
 * @note addr/port are little-endian.
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncResult
 * @param[in] addr Server IPv4 address.
 * @param[in] port Socket port. qlaunch uses value 55556.
 * @param[in] info \ref NsSystemDeliveryInfo
 */
Result nssuControlRequestReceiveSystemUpdate(NsSystemUpdateControl *c, AsyncResult *a, u32 addr, u16 port, NsSystemDeliveryInfo *info);

/**
 * @brief GetReceiveProgress
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[out] out \ref NsSystemUpdateProgress
 */
Result nssuControlGetReceiveProgress(NsSystemUpdateControl *c, NsSystemUpdateProgress *out);

/**
 * @brief ApplyReceivedUpdate
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 */
Result nssuControlApplyReceivedUpdate(NsSystemUpdateControl *c);

/**
 * @brief Gets the filesize for the specified ReceivedEulaData.
 * @note See the note for \ref nssuControlGetDownloadedEulaDataSize.
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[in] path EulaData path.
 * @param[out] filesize Output filesize.
 */
Result nssuControlGetReceivedEulaDataSize(NsSystemUpdateControl *c, const char* path, u64 *filesize);

/**
 * @brief Gets the specified ReceivedEulaData.
 * @note See the note for \ref nssuControlGetDownloadedEulaDataSize.
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[in] path EulaData path.
 * @param[out] buffer Output buffer.
 * @param[in] size Size of the output buffer, must be at least the output size from \ref nssuControlGetReceivedEulaDataSize.
 * @param[out] filesize Output filesize.
 */
Result nssuControlGetReceivedEulaData(NsSystemUpdateControl *c, const char* path, void* buffer, size_t size, u64 *filesize);

/**
 * @brief Does setup for ReceiveSystemUpdate by using the same nim cmds as \ref nssuDestroySystemUpdateTask.
 * @note qlaunch uses this before \ref nssuControlRequestReceiveSystemUpdate.
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 */
Result nssuControlSetupToReceiveSystemUpdate(NsSystemUpdateControl *c);

/**
 * @brief RequestCheckLatestUpdateIncludesRebootlessUpdate
 * @note Only available on [6.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncValue
 */
Result nssuControlRequestCheckLatestUpdateIncludesRebootlessUpdate(NsSystemUpdateControl *c, AsyncValue *a);

///@}

