/**
 * @file ns.h
 * @brief NS service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../nacp.h"
#include "../services/fs.h"
#include "../kernel/event.h"

typedef struct {
    NacpStruct nacp;
    u8 icon[0x20000];//JPEG
} NsApplicationControlData;

typedef struct
{
    u8 title_type;
    u8 storageID;
    u8 unk_x02;
    u8 padding;
    u32 title_version;
    u64 titleID;
} NsApplicationContentMetaStatus;

typedef struct
{
    u64 titleID;
    u8 type;
    u8 unk_x09;
    u8 unk_x0A[6];
    u8 unk_x10;
    u8 unk_x11[7];
} NsApplicationRecord;

typedef struct {
    u64 titleID;
    u32 version;
    u8 storageID;
    u8 index;
    u8 is_application;
} NsLaunchProperties;

typedef enum {
    NsShellEvent_None = 0,
    NsShellEvent_Exit = 1,
    NsShellEvent_Start = 2,
    NsShellEvent_Crash = 3,
    NsShellEvent_Debug = 4,
} NsShellEvent;

typedef struct {
    NsShellEvent event;
    u64 process_id;
} NsShellEventInfo;

Result nsInitialize(void);
void nsExit(void);

Result nsListApplicationRecord(NsApplicationRecord* buffer, size_t size, size_t entry_offset, size_t* out_entrycount);
Result nsListApplicationContentMetaStatus(u64 titleID, u32 index, NsApplicationContentMetaStatus* buffer, size_t size, size_t* out_entrycount);
Result nsGetApplicationControlData(u8 flag, u64 titleID, NsApplicationControlData* buffer, size_t size, size_t* actual_size);

/**
 * @brief Returns the total storage capacity (used + free) from content manager services.
 * @param storage_id Specified FsStorageId. (Must be FsStorageId_SdCard)
 * @param size Pointer to output the total storage size to.
 */
Result nsGetTotalSpaceSize(FsStorageId storage_id, u64 *size);

/**
 * @brief Returns the available storage capacity from content manager services.
 * @param storage_id Specified FsStorageId. (Must be FsStorageId_SdCard)
 * @param size Pointer to output the free storage size to.
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
Result nsdevGetShellEvent(Event* out); // Autoclear for nsdevShellEvent is always true.
Result nsdevGetShellEventInfo(NsShellEventInfo* out);
Result nsdevTerminateApplication(void);
Result nsdevPrepareLaunchProgramFromHost(NsLaunchProperties* out, const char* path, size_t path_len);
Result nsdevLaunchApplication(u64* out_pid, u64 app_title_id, u32 flags);
Result nsdevLaunchApplicationWithStorageId(u64* out_pid, u64 app_title_id, u32 flags, u8 app_storage_id, u8 patch_storage_id);
Result nsdevIsSystemMemoryResourceLimitBoosted(bool* out);
Result nsdevGetRunningApplicationProcessId(u64* out_pid);
Result nsdevSetCurrentApplicationRightsEnvironmentCanBeActive(bool can_be_active);
