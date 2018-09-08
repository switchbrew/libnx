#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu* parent;
    Event  event;
    bool   has_init;
} NvErrorNotifier;

Result nvErrorNotifierCreate(NvErrorNotifier* t, NvGpu* parent);
void   nvErrorNotifierClose(NvErrorNotifier* t);
Result nvErrorNotifierWait(NvErrorNotifier* t, u64 timeout);
Result nvErrorNotifierGetError(NvErrorNotifier* t, NvError* out);
