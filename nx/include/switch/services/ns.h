/**
 * @file ns.h
 * @brief NS services IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../nacp.h"
#include "../services/fs.h"
#include "../services/ncm.h"
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
    u8 title_type;                 ///< \ref NcmContentMetaType
    u8 storageID;                  ///< \ref FsStorageId
    u8 unk_x02;                    ///< Unknown.
    u8 padding;                    ///< Padding.
    u32 title_version;             ///< Title version.
    u64 titleID;                   ///< titleID.
} NsApplicationContentMetaStatus;

/// ApplicationRecord
typedef struct {
    u64 titleID;                   ///< titleID.
    u8 type;                       ///< Type.
    u8 unk_x09;                    ///< Unknown.
    u8 unk_x0A[6];                 ///< Unknown.
    u8 unk_x10;                    ///< Unknown.
    u8 unk_x11[7];                 ///< Unknown.
} NsApplicationRecord;

/// LaunchProperties
typedef struct {
    u64 titleID;                   ///< titleID.
    u32 version;                   ///< Title version.
    u8 storageID;                  ///< \ref FsStorageId
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

/// EulaDataPath
typedef struct {
    char path[0x100];              ///< Path.
} NsEulaDataPath;

/// SystemDeliveryInfo
typedef struct {
    u32 protocol_version;           ///< Must be <= to and match a system-setting.
    u8 unk_x4[0x8];                 ///< Unused by \ref nssuRequestSendSystemUpdate / \ref nssuControlRequestReceiveSystemUpdate, besides HMAC validation.
    u32 systemupdate_meta_version;  ///< SystemUpdate meta version.
    u64 systemupdate_meta_titleid;  ///< SystemUpdate meta titleID.
    u8 unk_x18;                     ///< Copied into state by \ref nssuRequestSendSystemUpdate.
    u8 unk_x19[0xc7];               ///< Unused by \ref nssuRequestSendSystemUpdate / \ref nssuControlRequestReceiveSystemUpdate, besides HMAC validation.
    u8 hmac[0x20];                  ///< HMAC-SHA256 over the previous 0xe0-bytes.
} NsSystemDeliveryInfo;

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
 * @param[in] titleID titleID.
 * @param[in] index Starting entry index.
 * @param[out] list Output array of \ref NsApplicationContentMetaStatus.
 * @param[in] count Size of the list array in entries.
 * @param[out] out_entrycount Total output entries.
 */
Result nsListApplicationContentMetaStatus(u64 titleID, s32 index, NsApplicationContentMetaStatus* list, s32 count, s32* out_entrycount);

/**
 * @brief Gets the \ref NsApplicationControlData for the specified title.
 * @param[in] flag Flag, official sw uses value 1.
 * @param[in] titleID titleID.
 * @param[out] buffer \ref NsApplicationControlData
 * @param[in] size Size of the buffer.
 * @param[out] actual_size Actual output size.
 */
Result nsGetApplicationControlData(NsApplicationControlSource source, u64 titleID, NsApplicationControlData* buffer, size_t size, u64* actual_size);

/**
 * @brief Returns the total storage capacity (used + free) from content manager services.
 * @param[in] storage_id Specified FsStorageId. (Must be FsStorageId_SdCard)
 * @param[out] size Pointer to output the total storage size to.
 */
Result nsGetTotalSpaceSize(FsStorageId storage_id, u64 *size);

/**
 * @brief Returns the available storage capacity from content manager services.
 * @param[in] storage_id Specified FsStorageId. (Must be FsStorageId_SdCard)
 * @param[out] size Pointer to output the free storage size to.
 */
Result nsGetFreeSpaceSize(FsStorageId storage_id, u64 *size);

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
Result nsdevLaunchApplicationForDevelop(u64* out_pid, u64 app_title_id, u32 flags);
Result nsdevLaunchApplicationWithStorageIdForDevelop(u64* out_pid, u64 app_title_id, u32 flags, u8 app_storage_id, u8 patch_storage_id);
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
 * @note Only available on [4.0.0+].
 * @param[out] a \ref AsyncResult
 * @param[in] inval0 Unknown input value.
 * @param[in] inval1 Unknown input value. qlaunch uses value 0xD904 (55556).
 * @param[in] info \ref NsSystemDeliveryInfo
 */
Result nssuRequestSendSystemUpdate(AsyncResult *a, u32 inval0, u16 inval1, NsSystemDeliveryInfo *info);

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
 * @note This mounts the Eula title, then uses the file "<mountname>:/<input path>".
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
 * @param[in] path EulaData path.
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
 * @param[in] path EulaData path.
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
 * @note Only available on [4.0.0+].
 * @param c \ref NsSystemUpdateControl
 * @param[out] a \ref AsyncResult
 * @param[in] inval0 Unknown input value.
 * @param[in] inval1 Unknown input value. qlaunch uses value 0xD904 (55556).
 * @param[in] info \ref NsSystemDeliveryInfo
 */
Result nssuControlRequestReceiveSystemUpdate(NsSystemUpdateControl *c, AsyncResult *a, u32 inval0, u16 inval1, NsSystemDeliveryInfo *info);

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

