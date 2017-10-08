// Copyright 2017 plutoo
#include <switch.h>
#include <malloc.h>

Result tmemCreate(TransferMemory* t, size_t size, Permission perm)
{
    Result rc = 0;

    t->MemHandle = INVALID_HANDLE;
    t->Size = size;
    t->Perm = perm;
    t->MappedAddr = NULL;
    t->SourceAddr = memalign(0x1000, size);

    if (t->SourceAddr == NULL) {
        rc = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
    }

    if (R_SUCCEEDED(rc)) {
        rc = svcCreateTransferMemory(&t->MemHandle, t->SourceAddr, size, perm);
    }

    return rc;
}

void tmemLoadRemote(TransferMemory* t, Handle handle, size_t size, Permission perm)
{
    t->MemHandle = handle;
    t->Size = size;
    t->Perm = perm;
    t->MappedAddr = NULL;
    t->SourceAddr = NULL;
}

Result tmemMap(TransferMemory* t)
{
    Result rc = 0;

    if (t->MappedAddr == NULL)
    {
        void* addr = virtmemReserve(t->Size);

        rc = svcMapTransferMemory(t->MemHandle, addr, t->Size, t->Perm);

        if (R_SUCCEEDED(rc)) {
            t->MappedAddr = addr;
        }
        else {
            virtmemFree(addr, t->Size);
        }
    }
    else {
        rc = LIBNX_ALREADYMAPPED;
    }

    return rc;
}

Result tmemUnmap(TransferMemory* t)
{
    Result rc;

    rc = svcUnmapTransferMemory(t->MemHandle, t->MappedAddr, t->Size);

    if (R_SUCCEEDED(rc)) {
        t->MappedAddr = NULL;
    }

    return rc;
}

void* tmemGetAddr(TransferMemory* t) {
    return t->MappedAddr;
}

Result tmemClose(TransferMemory* t)
{
    Result rc = 0;

    if (t->SourceAddr != NULL) {
        free(t->SourceAddr);
    }

    if (t->MappedAddr != NULL) {
        rc = tmemUnmap(t);
    }

    if (R_SUCCEEDED(rc)) {
        if (t->MemHandle != INVALID_HANDLE) {
            rc = svcCloseHandle(t->MemHandle);
        }

        t->SourceAddr = NULL;
        t->MemHandle = INVALID_HANDLE;
    }

    return rc;
}
