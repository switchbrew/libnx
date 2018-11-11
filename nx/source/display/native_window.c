#include <string.h>
#include "types.h"
#include "result.h"
#include "services/vi.h"
#include "display/binder.h"
#include "display/buffer_producer.h"
#include "display/native_window.h"
#include "nvidia/graphic_buffer.h"

static void _nwindowUpdate(NWindow* nw, const BqBufferOutput* out)
{
    nw->default_width = out->width;
    nw->default_height = out->height;
    nw->consumer_running_behind = out->numPendingBuffers > 1;
}

Result nwindowCreate(NWindow* nw, s32 binder_id, bool producer_controlled_by_app)
{
    Result rc;

    memset(nw, 0, sizeof(*nw));
    nw->has_init = true;
    nw->swap_interval = 1;
    nw->cur_slot = -1;
    nw->format = ~0U;

    binderCreate(&nw->bq, binder_id);
    rc = binderInitSession(&nw->bq);

    if (R_SUCCEEDED(rc))
        binderGetNativeHandle(&nw->bq, 0x0f, &nw->event);

    if (R_SUCCEEDED(rc)) {
        BqBufferOutput bqoutput;
        rc = bqConnect(&nw->bq, NATIVE_WINDOW_API_CPU, producer_controlled_by_app, &bqoutput);
        if (R_SUCCEEDED(rc)) {
            nw->is_connected = true;
            _nwindowUpdate(nw, &bqoutput);
        }
    }

    if (R_FAILED(rc))
        nwindowClose(nw);

    return rc;
}

Result nwindowCreateFromLayer(NWindow* nw, const ViLayer* layer)
{
    return nwindowCreate(nw, layer->igbp_binder_obj_id, false);
}

void nwindowClose(NWindow* nw)
{
    if (!nw || !nw->has_init)
        return;

    if (nw->is_connected)
        bqDisconnect(&nw->bq, NATIVE_WINDOW_API_CPU);

    eventClose(&nw->event);
    binderClose(&nw->bq);

    memset(nw, 0, sizeof(*nw));
}

Result nwindowConfigureBuffer(NWindow* nw, s32 slot, NvGraphicBuffer* buf)
{
    if (!nw || !buf || slot < 0 || slot >= 64)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (!nw->has_init)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    mutexLock(&nw->mutex);

    if (!nw->width)
        nw->width = buf->layers[0].width;
    if (!nw->height)
        nw->height = buf->layers[0].height;
    if (nw->format == ~0U)
        nw->format = buf->format;
    if (!nw->usage)
        nw->usage = buf->usage;

    BqGraphicBuffer bqbuf;
    bqbuf.width = nw->width;
    bqbuf.height = nw->height;
    bqbuf.stride = buf->stride;
    //bqbuf.stride = buf->layers[0].pitch / (u8)(buf->layers[0].color_format >> 3); // this also works
    bqbuf.format = nw->format;
    bqbuf.usage = nw->usage;
    bqbuf.native_handle = &buf->header;

    Result rc = bqSetPreallocatedBuffer(&nw->bq, slot, &bqbuf);

    mutexUnlock(&nw->mutex);
    return rc;
}

Result nwindowDequeueBuffer(NWindow* nw, s32* out_slot, NvMultiFence* out_fence)
{
    if (!nw || !out_slot)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (!nw->has_init)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    mutexLock(&nw->mutex);

    if (nw->cur_slot >= 0) {
        mutexUnlock(&nw->mutex);
        return MAKERESULT(Module_Libnx, LibnxError_BadGfxDequeueBuffer);
    }

    NvMultiFence fence;
    s32 slot;
    Result rc;

    if (nw->event.revent != INVALID_HANDLE) {
        do {
            eventWait(&nw->event, U64_MAX);
            rc = bqDequeueBuffer(&nw->bq, true, nw->width, nw->height, nw->format, nw->usage, &slot, &fence);
        } while (rc == MAKERESULT(Module_LibnxBinder, LibnxBinderError_WouldBlock));
    }
    else
        rc = bqDequeueBuffer(&nw->bq, false, nw->width, nw->height, nw->format, nw->usage, &slot, &fence);

    if (R_SUCCEEDED(rc)) {
        if (!(nw->slots_requested & (1UL << slot))) {
            rc = bqRequestBuffer(&nw->bq, slot, NULL);
            if (R_FAILED(rc))
                bqCancelBuffer(&nw->bq, slot, &fence);
            else
                nw->slots_requested |= 1UL << slot;
        }
    }

    if (R_SUCCEEDED(rc)) {
        *out_slot = slot;
        nw->cur_slot = slot;
        if (out_fence)
            *out_fence = fence;
        else
            nvMultiFenceWait(&fence, -1);
    }

    mutexUnlock(&nw->mutex);
    return rc;
}

Result nwindowCancelBuffer(NWindow* nw, s32 slot, const NvMultiFence* fence)
{
    if (!nw || slot < 0 || slot >= 64)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (!nw->has_init)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    mutexLock(&nw->mutex);

    if (slot != nw->cur_slot) {
        mutexUnlock(&nw->mutex);
        return MAKERESULT(Module_Libnx, LibnxError_BadGfxQueueBuffer);
    }

    static const NvMultiFence s_emptyFence = {0};
    if (!fence)
        fence = &s_emptyFence;

    Result rc = bqCancelBuffer(&nw->bq, slot, fence);
    if (R_SUCCEEDED(rc))
        nw->cur_slot = -1;

    mutexUnlock(&nw->mutex);
    return rc;
}

Result nwindowQueueBuffer(NWindow* nw, s32 slot, const NvMultiFence* fence)
{
    if (!nw || slot < 0 || slot >= 64)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (!nw->has_init)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    mutexLock(&nw->mutex);

    if (slot != nw->cur_slot) {
        mutexUnlock(&nw->mutex);
        return MAKERESULT(Module_Libnx, LibnxError_BadGfxQueueBuffer);
    }

    BqBufferInput bqinput;
    memset(&bqinput, 0, sizeof(bqinput));
    bqinput.crop = nw->crop;
    bqinput.scalingMode = nw->scaling_mode;
    bqinput.transform = nw->transform;
    bqinput.stickyTransform = nw->sticky_transform;
    bqinput.swapInterval = nw->swap_interval;
    if (fence)
        bqinput.fence = *fence;

    BqBufferOutput bqoutput;
    Result rc = bqQueueBuffer(&nw->bq, slot, &bqinput, &bqoutput);
    if (R_SUCCEEDED(rc)) {
        nw->cur_slot = -1;
        _nwindowUpdate(nw, &bqoutput);
    }

    mutexUnlock(&nw->mutex);
    return rc;
}
