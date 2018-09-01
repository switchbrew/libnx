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

typedef struct {
    NacpStruct nacp;
    u8 icon[0x20000];//JPEG
} NsApplicationControlData;

typedef struct
{
    u8 title_type;
    u8 storage_id;
    u8 unk_02;
    u8 padding;
    u32 title_version;
    u64 title_id;
} NsApplicationContentMetaStatus;

typedef struct
{
    u64 title_id;
    u8 type;
    u8 unk_09;
    u8 unk_0A[6];
    u8 unk_10;
    u8 unk_11[7];
} NsApplicationRecord;

Result nsInitialize(void);
void nsExit(void);

Result nsListApplicationRecord(NsApplicationRecord* buffer, size_t size, int* out_entrycount);
Result nsListApplicationContentMetaStatus(u64 title_id, u32 index, NsApplicationContentMetaStatus* buffer, size_t size, int* out_entrycount);
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
Result nsdevInitialize();
void nsdevExit();

Result nsdevTerminateProcess(u64 pid);
Result nsdevTerminateProgram(u64 tid);
