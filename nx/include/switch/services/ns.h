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
#include "../kernel/event.h"

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

/// ShellEvent
typedef enum {
    NsShellEvent_None = 0,         ///< None
    NsShellEvent_Exit = 1,         ///< Exit
    NsShellEvent_Start = 2,        ///< Start
    NsShellEvent_Crash = 3,        ///< Crash
    NsShellEvent_Debug = 4,        ///< Debug
} NsShellEvent;

/// ShellEventInfo
typedef struct {
    NsShellEvent event;            ///< \ref NsShellEvent
    u64 process_id;                ///< processID.
} NsShellEventInfo;

Result nsInitialize(void);
void nsExit(void);

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
Result nsGetApplicationControlData(u8 flag, u64 titleID, NsApplicationControlData* buffer, size_t size, u64* actual_size);

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

Result nsvmInitialize(void);
void nsvmExit(void);

Result nsvmNeedsUpdateVulnerability(bool *out);
Result nsvmGetSafeSystemVersion(u16 *out);

/* ns:dev */
Result nsdevInitialize(void);
void nsdevExit(void);

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
