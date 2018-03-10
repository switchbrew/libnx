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

Result nvOpen(u32 *fd, const char *devicepath);
Result nvIoctl(u32 fd, u32 request, void* argp);
Result nvClose(u32 fd);
Result nvQueryEvent(u32 fd, u32 event_id, Handle *handle_out);

Result nvConvertError(int rc);
