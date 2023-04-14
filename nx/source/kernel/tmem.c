// Copyright 2017 plutoo
#include <string.h>
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/tmem.h"
#include "kernel/virtmem.h"
#include "runtime/diag.h"
#include "../runtime/alloc.h"

Result tmemCreate(TransferMemory* t, size_t size, Permission perm)
{
    Result rc = 0;

    t->handle = INVALID_HANDLE;
    t->size = size;
    t->perm = perm;
    t->map_addr = NULL;
    t->src_addr = __libnx_aligned_alloc(0x1000, size);

    if (t->src_addr == NULL) {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }
    else {
        memset(t->src_addr, 0, size);
    }

    if (R_SUCCEEDED(rc)) {
        rc = svcCreateTransferMemory(&t->handle, t->src_addr, size, perm);
    }

    if (R_FAILED(rc)) {
        __libnx_free(t->src_addr);
        t->src_addr = NULL;
    }

    return rc;
}

Result tmemCreateFromMemory(TransferMemory* t, void* buf, size_t size, Permission perm)
{
    Result rc = 0;

    if (buf == NULL || ((uintptr_t)buf & 0xFFF)) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    t->handle = INVALID_HANDLE;
    t->size = size;
    t->perm = perm;
    t->map_addr = NULL;
    t->src_addr = NULL;

    rc = svcCreateTransferMemory(&t->handle, buf, size, perm);

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
        virtmemLock();
        void* addr = virtmemFindAslr(t->size, 0x1000);
        rc = svcMapTransferMemory(t->handle, addr, t->size, t->perm);
        virtmemUnlock();

        if (R_SUCCEEDED(rc)) {
            t->map_addr = addr;
        }
    }
    else {
        rc = MAKERESULT(Module_Libnx, LibnxError_AlreadyMapped);
    }

    return rc;
}

Result tmemUnmap(TransferMemory* t)
{
    Result rc;

    rc = svcUnmapTransferMemory(t->handle, t->map_addr, t->size);

    if (R_SUCCEEDED(rc)) {
        t->map_addr = NULL;
    }

    return rc;
}

Result tmemCloseHandle(TransferMemory* t)
{
    Result rc = 0;

    if (t->handle != INVALID_HANDLE) {
        rc = svcCloseHandle(t->handle);
        t->handle = INVALID_HANDLE;
    }

    return rc;
}

Result tmemWaitForPermission(TransferMemory* t, Permission perm)
{
    Result rc = 0;

    if ((t->perm & perm) != perm) {
        MemoryInfo m = {0};
        u32 p = 0;
        rc = svcQueryMemory(&m, &p, (u64)(t->src_addr));

        if (R_FAILED(rc)) {
            return rc;
        }

        while ((m.perm & perm) != perm) {
            rc = svcQueryMemory(&m, &p, (u64)(t->src_addr));

            if (R_FAILED(rc)) {
                return rc;
            }

            svcSleepThread(100000);
        }
    }

    return rc;
}

Result tmemClose(TransferMemory* t)
{
    Result rc = 0;

    if (t->map_addr != NULL) {
        rc = tmemUnmap(t);
    }

    if (R_SUCCEEDED(rc)) {
        rc = tmemCloseHandle(t);

        if (t->src_addr != NULL) {
            __libnx_free(t->src_addr);
        }

        t->src_addr = NULL;
    }

    return rc;
}
