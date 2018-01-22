// Copyright 2017 plutoo
#include <malloc.h>
#include <switch/types.h>
#include <switch/result.h>
#include <switch/kernel/svc.h>
#include <switch/kernel/tmem.h>
#include <switch/kernel/virtmem.h>

Result tmemCreate(TransferMemory* t, size_t size, Permission perm)
{
    Result rc = 0;

    t->handle = INVALID_HANDLE;
    t->size = size;
    t->perm = perm;
    t->map_addr = NULL;
    t->src_addr = memalign(0x1000, size);

    if (t->src_addr == NULL) {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }

    if (R_SUCCEEDED(rc)) {
        rc = svcCreateTransferMemory(&t->handle, t->src_addr, size, perm);
    }

    return rc;
}

void tmemLoadRemote(TransferMemory* t, Handle handle, size_t size, Permission perm)
{
    t->handle = handle;
    t->size = size;
    t->perm = perm;
    t->map_addr = NULL;
    t->src_addr = NULL;
}

Result tmemMap(TransferMemory* t)
{
    Result rc = 0;

    if (t->map_addr == NULL)
    {
        void* addr = virtmemReserve(t->size);

        rc = svcMapTransferMemory(t->handle, addr, t->size, t->perm);

        if (R_SUCCEEDED(rc)) {
            t->map_addr = addr;
        }
        else {
            virtmemFree(addr, t->size);
        }
    }
    else {
        rc = LibnxError_AlreadyMapped;
    }

    return rc;
}

Result tmemUnmap(TransferMemory* t)
{
    Result rc;

    rc = svcUnmapTransferMemory(t->handle, t->map_addr, t->size);

    if (R_SUCCEEDED(rc)) {
        virtmemFree(t->map_addr, t->size);
        t->map_addr = NULL;
    }

    return rc;
}

void* tmemGetAddr(TransferMemory* t) {
    return t->map_addr;
}

Result tmemClose(TransferMemory* t)
{
    Result rc = 0;

    if (t->map_addr != NULL) {
        rc = tmemUnmap(t);
    }

    if (R_SUCCEEDED(rc)) {
        if (t->handle != INVALID_HANDLE) {
            rc = svcCloseHandle(t->handle);
        }

        if (t->src_addr != NULL) {
            free(t->src_addr);
        }

        t->src_addr = NULL;
        t->handle = INVALID_HANDLE;
    }

    return rc;
}
