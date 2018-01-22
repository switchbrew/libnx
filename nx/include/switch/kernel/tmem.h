#pragma once
#include "../types.h"

typedef struct {
    Handle      handle;
    size_t      size;
    Permission  perm;
    void*       src_addr;
    void*       map_addr;
} TransferMemory;

Result tmemCreate(TransferMemory* t, size_t size, Permission perm);
void   tmemLoadRemote(TransferMemory* t, Handle handle, size_t size, Permission perm);

Result tmemMap(TransferMemory* t);
Result tmemUnmap(TransferMemory* t);
void*  tmemGetAddr(TransferMemory* t);

Result tmemClose(TransferMemory* t);
