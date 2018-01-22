// Copyright 2018 plutoo
#include <malloc.h>

#include <switch/types.h>
#include <switch/result.h>
#include <switch/runtime/env.h>
#include <switch/kernel/svc.h>
#include <switch/kernel/detect.h>
#include <switch/kernel/virtmem.h>
#include <switch/kernel/jit.h>

Result jitCreate(Jit* j, size_t size)
{
    JitType type;

    // Use new jit primitive introduced in 4.0.0, if available.
    if (kernelAbove400() && envIsSyscallHinted(0x4B) && envIsSyscallHinted(0x4C)) {
        type = JitType_JitMemory;
    }
    // Fall back to MapProcessCodeMemory if available.
    else if (envIsSyscallHinted(0x73) && envIsSyscallHinted(0x77) && envIsSyscallHinted(0x78)
             && (envGetOwnProcessHandle() != INVALID_HANDLE)) {
        type = JitType_CodeMemory;
    }
    else {
        // Jit is unavailable. :(
        return MAKERESULT(Module_Libnx, LibnxError_JitUnavailable);
    }

    size = (size + 0xFFF) &~ 0xFFF;

    void* src_addr = memalign(size, 0x1000);

    if (src_addr == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    j->type = type;
    j->size = size;
    j->src_addr = src_addr;
    j->rx_addr = virtmemReserve(j->size);
    j->handle = INVALID_HANDLE;

    Result rc = 0;

    switch (j->type)
    {
    case JitType_CodeMemory:
        j->rw_addr = j->src_addr;
        break;

    case JitType_JitMemory:
        j->rw_addr = virtmemReserve(j->size);

        rc = svcCreateJitMemory(&j->handle, j->src_addr, j->size);
        if (R_SUCCEEDED(rc))
        {
            rc = svcMapJitMemory(j->handle, JitMapOperation_MapOwner, j->rw_addr, j->size, PERM_RW);
            if (R_SUCCEEDED(rc))
            {
                rc = svcMapJitMemory(j->handle, JitMapOperation_MapSlave, j->rx_addr, j->size, PERM_RX);

                if (R_FAILED(rc)) {
                    svcMapJitMemory(j->handle, JitMapOperation_UnmapOwner, j->rw_addr, j->size, 0);
                }
            }

            if (R_FAILED(rc)) {
                svcCloseHandle(j->handle);
                j->handle = INVALID_HANDLE;
            }
        }

        if (R_FAILED(rc)) {
            virtmemFree(j->rw_addr, j->size);
            j->rw_addr = NULL;
        }

        break;
    }

    if (R_FAILED(rc)) {
        virtmemFree(j->rx_addr, j->size);
        free(j->src_addr);
        j->src_addr = NULL;
    }

    return rc;
}

Result jitTransitionToWritable(Jit* j)
{
    Result rc = 0;

    switch (j->type) {
    case JitType_CodeMemory:
        rc = svcUnmapProcessCodeMemory(envGetOwnProcessHandle(), (u64) j->rx_addr, (u64) j->src_addr, j->size);
        break;

    case JitType_JitMemory:
        // No need to do anything.
        break;
    }

    return rc;
}

Result jitTransitionToExecutable(Jit* j)
{
    Result rc = 0;

    switch (j->type) {
    case JitType_CodeMemory:
        rc = svcMapProcessCodeMemory(envGetOwnProcessHandle(), (u64) j->rx_addr, (u64) j->src_addr, j->size);

        if (R_SUCCEEDED(rc)) {
            rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64) j->rx_addr, j->size, PERM_RX);

            if (R_FAILED(rc)) {
                jitTransitionToWritable(j);
            }
        }
        break;

    case JitType_JitMemory:
        // todo: Clean dcache, invalidate icache.
        break;
    }

    return rc;
}

Result jitClose(Jit* j)
{
    Result rc = 0;

    switch (j->type)
    {
    case JitType_CodeMemory:
        rc = jitTransitionToWritable(j);

        if (R_SUCCEEDED(rc)) {
            virtmemFree(j->rx_addr, j->size);
        }
        break;

    case JitType_JitMemory:
        rc = svcMapJitMemory(j->handle, JitMapOperation_UnmapOwner, j->rw_addr, j->size, 0);

        if (R_SUCCEEDED(rc)) {
            virtmemFree(j->rw_addr, j->size);

            rc = svcMapJitMemory(j->handle, JitMapOperation_UnmapSlave, j->rx_addr, j->size, 0);

            if (R_SUCCEEDED(rc)) {
                virtmemFree(j->rw_addr, j->size);
                svcCloseHandle(j->handle);
            }
        }
        break;
    }

    if (R_SUCCEEDED(rc)) {
        if (j->src_addr != NULL) {
            free(j->src_addr);
            j->src_addr = NULL;
        }
    }
    return rc;
}

void* jitGetRwAddr(Jit* j) {
    return j->rw_addr;
}

void* jitGetRxAddr(Jit* j) {
    return j->rx_addr;
}
