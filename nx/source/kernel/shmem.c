// Copyright 2017 plutoo
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/virtmem.h"
#include "kernel/shmem.h"

Result shmemCreate(SharedMemory* s, size_t size, Permission local_perm, Permission remote_perm)
{
    Result rc;

    s->handle = INVALID_HANDLE;
    s->size = size;
    s->map_addr = NULL;
    s->perm = local_perm;

    rc = svcCreateSharedMemory(&s->handle, size, local_perm, remote_perm);

    return rc;
}

void shmemLoadRemote(SharedMemory* s, Handle handle, size_t size, Permission perm)
{
    s->handle = handle;
    s->size = size;
    s->map_addr = NULL;
    s->perm = perm;
}

Result shmemMap(SharedMemory* s)
{
    Result rc = 0;

    if (s->map_addr == NULL)
    {
        void* addr = virtmemReserve(s->size);

        rc = svcMapSharedMemory(s->handle, addr, s->size, s->perm);

        if (R_SUCCEEDED(rc)) {
            s->map_addr = addr;
        }
        else {
            virtmemFree(addr, s->size);
        }
    }
    else {
        rc = LibnxError_AlreadyMapped;
    }

    return rc;
}

Result shmemUnmap(SharedMemory* s)
{
    Result rc;

    rc = svcUnmapSharedMemory(s->handle, s->map_addr, s->size);

    if (R_SUCCEEDED(rc)) {
        virtmemFree(s->map_addr, s->size);
        s->map_addr = NULL;
    }

    return rc;
}

Result shmemClose(SharedMemory* s)
{
    Result rc = 0;

    if (s->map_addr != NULL) {
        rc = shmemUnmap(s);
    }

    if (R_SUCCEEDED(rc)) {
        if (s->handle != INVALID_HANDLE) {
            rc = svcCloseHandle(s->handle);
        }

        s->handle = INVALID_HANDLE;
    }

    return rc;
}
