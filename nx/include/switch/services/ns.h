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
    NsLatestSystemUpdate_Unknown0 = 0,                ///< Unknown.
    NsLatestSystemUpdate_Unknown1 = 1,                ///< Unknown.
    NsLatestSystemUpdate_Unknown2 = 2,                ///< Unknown.
} NsLatestSystemUpdate;

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

/// NsApplicationContentMetaStatus
typedef struct {
    u8 meta_type;                  ///< \ref NcmContentMetaType
    u8 storageID;                  ///< \ref NcmStorageId
    u8 unk_x02;                    ///< Unknown.
    u8 padding;                    ///< Padding.
    u32 version;                   ///< Application version.
    u64 application_id;            ///< ApplicationId.
} NsApplicationContentMetaStatus;

/// ApplicationRecord
typedef struct {
    u64 application_id;            ///< ApplicationId.
    u8 type;                       ///< Type.
    u8 unk_x09;                    ///< Unknown.
    u8 unk_x0a[6];                 ///< Unknown.
    u8 unk_x10;                    ///< Unknown.
    u8 unk_x11[7];                 ///< Unknown.
} NsApplicationRecord;

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
        u32 system_delivery_protocol_version;       ///< Must match a system-setting.
        u32 application_delivery_protocol_version;  ///< Loaded from a system-setting. Unused by \ref nssuRequestSendSystemUpdate / \ref nssuControlRequestReceiveSystemUpdate, besides HMAC validation.
        u32 includes_exfat;                         ///< Whether ExFat is included. Unused by \ref nssuRequestSendSystemUpdate / \ref nssuControlRequestReceiveSystemUpdate, besides HMAC validation.
        u32 system_update_meta_version;             ///< SystemUpdate meta version.
        u64 system_update_meta_id;                  ///< SystemUpdate meta Id.
        u8 unk_x18;                                 ///< Copied into state by \ref nssuRequestSendSystemUpdate.
        u8 unk_x19;                                 ///< Unused by \ref nssuRequestSendSystemUpdate / \ref nssuControlRequestReceiveSystemUpdate, besides HMAC validation.
        u8 unk_x1a;                                 ///< Unknown.
        u8 unk_x1b[0xc5];                           ///< Unused by \ref nssuRequestSendSystemUpdate / \ref nssuControlRequestReceiveSystemUpdate, besides HMAC validation.
    } data;                                         ///< Data used with the below hmac.
    u8 hmac[0x20];                                  ///< HMAC-SHA256 over the above data.
} NsSystemDeliveryInfo;

/// ApplicationDeliveryInfo
typedef struct {
    struct {
        u8 unk_x0[0x10];                            ///< Unknown.
        u32 application_version;                    ///< Application version.
        u32 unk_x14;                                ///< Unknown.
        u32 required_system_version;                ///< Required system version, see NsSystemDeliveryInfo::system_update_meta_version.
        u32 unk_x1c;                                ///< Unknown.
        u8 unk_x20[0xc0];                           ///< Unknown.
    } data;                                         ///< Data used with the below hmac.
    u8 hmac[0x20];                                  ///< HMAC-SHA256 over the above data.
} NsApplicationDeliveryInfo;

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

/// Gets the Service object for IApplicationManagerInterface.
Service* nsGetServiceSession_ApplicationManagerInterface(void);

/**
 * @brief Gets an listing of \ref NsApplicationRecord.
 * @param[out] records Output array of \ref NsApplicationRecord.
 * @param[in] count Size of the records array in entries.
 * @param[in] entry_offset Starting entry offset.
 * @param[out] out_entrycount Total output entries.
 */
Result nsListApplicationRecord(NsApplicationRecord* records, s32 count, s32 entry_offset, s32* out_entrycount);

/**
 * @brief Gets an listing of \ref NsApplicationContentMetaStatus.
 * @param[in] application_id ApplicationId.
 * @param[in] index Starting entry index.
 * @param[out] list Output array of \ref NsApplicationContentMetaStatus.
 * @param[in] count Size of the list array in entries.
 * @param[out] out_entrycount Total output entries.
 */
Result nsListApplicationContentMetaStatus(u64 application_id, s32 index, NsApplicationContentMetaStatus* list, s32 count, s32* out_entrycount);

/**
 * @brief Gets the \ref NsApplicationControlData for the specified application.
 * @param[in] source Source, official sw uses ::NsApplicationControlSource_Storage.
 * @param[in] application_id ApplicationId.
 * @param[out] buffer \ref NsApplicationControlData
 * @param[in] size Size of the buffer.
 * @param[out] actual_size Actual output size.
 */
Result nsGetApplicationControlData(NsApplicationControlSource source, u64 application_id, NsApplicationControlData* buffer, size_t size, u64* actual_size);

/**
 * @brief Returns the total storage capacity (used + free) from content manager services.
 * @param[in] storage_id \ref NcmStorageId. Must be ::NcmStorageId_SdCard.
 * @param[out] size Pointer to output the total storage size to.
 */
Result nsGetTotalSpaceSize(NcmStorageId storage_id, u64 *size);

/**
 * @brief Returns the available storage capacity from content manager services.
 * @param[in] storage_id \ref NcmStorageId. Must be ::NcmStorageId_SdCard.
 * @param[out] size Pointer to output the free storage size to.
 */
Result nsGetFreeSpaceSize(NcmStorageId storage_id, u64 *size);

/**
 * @brief Generates a \ref NsSystemDeliveryInfo using the currently installed SystemUpdate meta.
 * @note Only available on [4.0.0+].
 * @param[out] info \ref NsSystemDeliveryInfo
 */
Result nsGetSystemDeliveryInfo(NsSystemDeliveryInfo *info);

/**
 * @brief SelectLatestSystemDeliveryInfo
 * @note This selects the \ref NsSystemDeliveryInfo with the latest version from sys_list, using minimum versions determined from app_list/state and base_info. This also does various validation, etc.
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
 * @brief After validation etc, this sets the output bool by comparing system-version fields in the \ref NsSystemDeliveryInfo / info-array and with a state field. 
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
 * @brief Both \ref NsSystemDeliveryInfo are validated, then the system_update_meta_version in the first/second \ref NsSystemDeliveryInfo are compared.
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

Result nsdevLaunchProgram(u64* out_pid, const NsLaunchProperties* properties, u32 flags);
Result nsdevTerminateProcess(u64 pid);
Result nsdevTerminateProgram(u64 tid);
Result nsdevGetShellEvent(Event* out_event); ///< Autoclear for nsdevShellEvent is always true.
Result nsdevGetShellEventInfo(NsShellEventInfo* out);
Result nsdevTerminateApplication(void);
Result nsdevPrepareLaunchProgramFromHost(NsLaunchProperties* out, const char* path, size_t path_len);
Result nsdevLaunchApplicationForDevelop(u64* out_pid, u64 application_id, u32 flags);
Result nsdevLaunchApplicationWithStorageIdForDevelop(u64* out_pid, u64 application_id, u32 flags, u8 app_storage_id, u8 patch_storage_id);
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
 * @note The system will use the input addr/port with bind(), the input addr will eventually be validated with the addr from accept(). addr/port are little-endian.
 * @note After the system accepts a connection etc, an error will be thrown if the system is Internet-connected.
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] addr Client IPv4 address. qlaunch uses a local-WLAN addr.
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
 * @note The system will use the input addr/port with connect(). addr/port are little-endian.
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncResult
 * @param[in] addr Server IPv4 address. qlaunch uses a local-WLAN addr, however this can be any addr.
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

