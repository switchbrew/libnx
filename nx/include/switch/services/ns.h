/**
 * @file ns.h
 * @brief NS service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../nacp.h"

typedef struct {
    NacpStruct nacp;
    u8 icon[0x20000];//JPEG
} NsApplicationControlData;

Result nsInitialize(void);
void nsExit(void);

Result nsGetApplicationControlData(u8 flag, u64 titleID, NsApplicationControlData* buffer, size_t size, size_t* actual_size);

/**
 * @brief Returns the total storage from content manager services.
 * @param size Pointer to output the total storage size to.
 */
Result nsGetTotalSpaceSize(u64 *size);

/**
 * @brief Returns the available storage from content manager services.
 * @param size Pointer to output the free storage size to.
 */
Result nsGetFreeSpaceSize(u64 *size);

Result nsvmInitialize(void);
void nsvmExit(void);

Result nsvmNeedsUpdateVulnerability(bool *out);
Result nsvmGetSafeSystemVersion(u16 *out);
