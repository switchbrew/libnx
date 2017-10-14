// Copyright 2017 plutoo
#include <switch.h>
#include <malloc.h>

Result shmemCreate(SharedMemory* s, size_t size, Permission local_perm, Permission remote_perm)
{
    Result rc;

    s->MemHandle = INVALID_HANDLE;
    s->Size = size;
    s->MappedAddr = NULL;
    s->Perm = local_perm;

    rc = svcCreateSharedMemory(&s->MemHandle, size, local_perm, remote_perm);

    return rc;
}

void shmemLoadRemote(SharedMemory* s, Handle handle, size_t size, Permission perm)
{
    s->MemHandle = handle;
    s->Size = size;
    s->MappedAddr = NULL;
    s->Perm = perm;
}

Result shmemMap(SharedMemory* s)
{
    Result rc = 0;

    if (s->MappedAddr == NULL)
    {
        void* addr = virtmemReserve(s->Size);

        rc = svcMapSharedMemory(s->MemHandle, addr, s->Size, s->Perm);

        if (R_SUCCEEDED(rc)) {
            s->MappedAddr = addr;
        }
        else {
            virtmemFree(addr, s->Size);
        }
    }
    else {
        rc = LIBNX_ALREADYMAPPED;
    }

    return rc;
}

Result shmemUnmap(SharedMemory* s)
{
    Result rc;

    rc = svcUnmapSharedMemory(s->MemHandle, s->MappedAddr, s->Size);

    if (R_SUCCEEDED(rc)) {
        s->MappedAddr = NULL;
    }

    return rc;
}

void* shmemGetAddr(SharedMemory* s) {
    return s->MappedAddr;
}

Result shmemClose(SharedMemory* s)
{
    Result rc = 0;

    if (s->MappedAddr != NULL) {
        rc = shmemUnmap(s);
    }

    if (R_SUCCEEDED(rc)) {
        if (s->MemHandle != INVALID_HANDLE) {
            rc = svcCloseHandle(s->MemHandle);
        }

        s->MemHandle = INVALID_HANDLE;
    }

    return rc;
}
