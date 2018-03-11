/**
 * @file nv.h
 * @brief NVIDIA low level driver (nvdrv*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result nvInitialize(void);
void nvExit(void);

typedef enum {
    NvEventId_Gpu_SmException_BptIntReport=1,
    NvEventId_Gpu_SmException_BptPauseReport=2,
    NvEventId_Gpu_ErrorNotifier=3,

    NvEventId_CtrlGpu_ErrorEventHandle=1,
    NvEventId_CtrlGpu_Unknown=2,
} NvEventId;

#define NV_EVENT_ID_CTRL__SYNCPT(slot, syncpt) \
    ((1u<<28) | ((syncpt) << 16) | (slot))

Result nvOpen(u32 *fd, const char *devicepath);
Result nvIoctl(u32 fd, u32 request, void* argp);
Result nvClose(u32 fd);
Result nvQueryEvent(u32 fd, u32 event_id, Handle *handle_out);

Result nvConvertError(int rc);
