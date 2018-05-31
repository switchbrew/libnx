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

Result nsvmInitialize(void);
void nsvmExit(void);

Result nsvmNeedsUpdateVulnerability(u8 *out);
Result nsvmGetSafeSystemVersion(u16 *out);
