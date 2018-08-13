#include "driver_internal.h"

int audrvMemPoolAdd(AudioDriver* d, void* buffer, size_t size)
{
    if (!buffer || ((uintptr_t)buffer & (AUDREN_MEMPOOL_ALIGNMENT-1)))
        return -1;
    if (size & (AUDREN_MEMPOOL_ALIGNMENT-1))
        return -1;

    int id = d->etc->first_free_mempool;
    if (id < 0)
        return -1;

    d->etc->first_free_mempool = d->etc->mempools[id].next_free;
    d->in_mempools[id].address = buffer;
    d->in_mempools[id].size = size;
    d->in_mempools[id].state = AudioRendererMemPoolState_New;

    return id;
}

bool audrvMemPoolRemove(AudioDriver* d, int id)
{
    if (id < 0)
        return false;
    if (d->in_mempools[id].state == AudioRendererMemPoolState_Attached || d->in_mempools[id].state == AudioRendererMemPoolState_RequestDetach)
        return false;

    d->etc->mempools[id].next_free = d->etc->first_free_mempool;
    d->etc->first_free_mempool = id;
    d->in_mempools[id].address = NULL;
    d->in_mempools[id].size = 0;
    d->in_mempools[id].state = AudioRendererMemPoolState_Released;

    return true;
}

bool audrvMemPoolAttach(AudioDriver* d, int id)
{
    if (id < 0 || id >= d->etc->mempool_count)
        return false;

    switch (d->in_mempools[id].state) {
        case AudioRendererMemPoolState_New:
        case AudioRendererMemPoolState_RequestAttach:
        case AudioRendererMemPoolState_Detached:
            d->in_mempools[id].state = AudioRendererMemPoolState_RequestAttach;
            return true;
        case AudioRendererMemPoolState_RequestDetach:
        case AudioRendererMemPoolState_Attached:
            d->in_mempools[id].state = AudioRendererMemPoolState_Attached;
            return true;
        default:
            return false;
    }
}

bool audrvMemPoolDetach(AudioDriver* d, int id)
{
    if (id < 0 || id >= d->etc->mempool_count)
        return false;

    switch (d->in_mempools[id].state) {
        case AudioRendererMemPoolState_New:
            return true;
        case AudioRendererMemPoolState_RequestAttach:
        case AudioRendererMemPoolState_Detached:
            d->in_mempools[id].state = AudioRendererMemPoolState_Detached;
            return true;
        case AudioRendererMemPoolState_RequestDetach:
        case AudioRendererMemPoolState_Attached:
            d->in_mempools[id].state = AudioRendererMemPoolState_RequestDetach;
            return true;
        default:
            return false;
    }
}
