#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu* parent;
    Event  event;
    bool   has_init;
} NvErrorNotifier;

Result nverrCreate(NvErrorNotifier* t, NvGpu* parent);
void   nverrClose(NvErrorNotifier* t);
