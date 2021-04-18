/**
 * @file mmu.h
 * @brief Multimedia (mm) IPC wrapper.
 * @author averne
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    MmuModuleId_Ram   = 2,
    MmuModuleId_Nvenc = 5,
    MmuModuleId_Nvdec = 6,
    MmuModuleId_Nvjpg = 7,
} MmuModuleId;

typedef struct {
    MmuModuleId module;
    u32         id;
} MmuRequest;

Result mmuInitialize(void);
void mmuExit(void);
Service* mmuGetServiceSession(void);

Result mmuRequestInitialize(MmuRequest *request, MmuModuleId module, u32 unk, bool autoclear); ///< unk is ignored by official software
Result mmuRequestFinalize(const MmuRequest *request);
Result mmuRequestGet(const MmuRequest *request, u32 *out_freq_hz);
Result mmuRequestSetAndWait(const MmuRequest *request, u32 freq_hz, s32 timeout);
