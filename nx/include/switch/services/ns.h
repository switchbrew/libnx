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
} nsApplicationControlData;

Result nsInitialize(void);
void nsExit(void);

Result nsGetApplicationControlData(u8 flag, u64 titleID, nsApplicationControlData* buffer, size_t size, size_t* actual_size);
