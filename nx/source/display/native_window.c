#include <string.h>
#include "types.h"
#include "result.h"
#include "services/vi.h"
#include "display/binder.h"
#include "display/buffer_producer.h"
#include "display/native_window.h"
#include "nvidia/graphic_buffer.h"

#define NWINDOW_MAGIC 0x6E69574E // NWin

static void _nwindowUpdate(NWindow* nw, const BqBufferOutput* out)
{
    nw->default_width = out->width;
    nw->default_height = out->height;
    nw->consumer_running_behind = out->numPendingBuffers > 1;
}

static Result _nwindowConnect(NWindow* nw)
{
    BqBufferOutput bqoutput;
    Result rc = bqConnect(&nw->bq, NATIVE_WINDOW_API_CPU, nw->producer_controlled_by_app, &bqoutput);
    if (R_SUCCEEDED(rc)) {
        nw->is_connected = true;
        _nwindowUpdate(nw, &bqoutput);
    }
    return rc;
}

static Result _nwindowDisconnect(NWindow* nw)
{
    Result rc = bqDisconnect(&nw->bq, NATIVE_WINDOW_API_CPU);
    if (R_SUCCEEDED(rc)) {
        nw->is_connected = false;
        nw->slots_configured = 0;
        nw->slots_requested = 0;
        nw->cur_slot = -1;
        nw->width = 0;
        nw->height = 0;
        nw->format = 0;
        nw->usage = 0;
    }
    return rc;
}

bool nwindowIsValid(NWindow* nw)
{
    return nw && nw->magic == NWINDOW_MAGIC;
}

Result nwindowCreate(NWindow* nw, s32 binder_id, bool producer_controlled_by_app)
{
    Result rc;

    memset(nw, 0, sizeof(*nw));
    nw->magic = NWINDOW_MAGIC;
    nw->swap_interval = 1;
    nw->cur_slot = -1;
    nw->format = ~0U;
    nw->producer_controlled_by_app = producer_controlled_by_app;

    binderCreate(&nw->bq, binder_id);
    rc = binderInitSession(&nw->bq);

    if (R_SUCCEEDED(rc))
        binderGetNativeHandle(&nw->bq, 0x0f, &nw->event);

    if (R_SUCCEEDED(rc))
        rc = _nwindowConnect(nw);

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
    if (!nwindowIsValid(nw))
        return;

    if (nw->is_connected)
        _nwindowDisconnect(nw);

    eventClose(&nw->event);
    binderClose(&nw->bq);

    memset(nw, 0, sizeof(*nw));
}

Result nwindowGetDimensions(NWindow* nw, u32* out_width, u32* out_height)
{
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (!out_width || !out_height)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    *out_width = nw->width ? nw->width : nw->default_width;
    *out_height = nw->height ? nw->height : nw->default_height;
    return 0;
}

Result nwindowSetDimensions(NWindow* nw, u32 width, u32 height)
{
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if ((nw->width || nw->height) && nw->slots_configured)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    nw->width = width;
    nw->height = height;
    memset(&nw->crop, 0, sizeof(nw->crop));
    return 0;
}

Result nwindowSetCrop(NWindow* nw, s32 left, s32 top, s32 right, s32 bottom)
{
    if (right < left || bottom < top)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    u32 width, height;
    Result rc = nwindowGetDimensions(nw, &width, &height);
    if (R_SUCCEEDED(rc)) {
        nw->crop.left   = left < 0 ? 0 : left;
        nw->crop.top    = top < 0 ? 0 : top;
        nw->crop.right  = right > width ? width : right;
        nw->crop.bottom = bottom > height ? height : bottom;
    }
    return rc;
}

Result nwindowSetTransform(NWindow* nw, u32 transform)
{
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (transform & ~(HAL_TRANSFORM_FLIP_H|HAL_TRANSFORM_FLIP_V|HAL_TRANSFORM_ROT_90))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    nw->transform = transform;
    return 0;
}

Result nwindowSetSwapInterval(NWindow* nw, u32 swap_interval)
{
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    nw->swap_interval = swap_interval;
    return 0;
}

Result nwindowConfigureBuffer(NWindow* nw, s32 slot, NvGraphicBuffer* buf)
{
    if (!nw || !buf || slot < 0 || slot >= 64)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    mutexLock(&nw->mutex);

    if (nw->slots_configured & (1UL << slot)) {
        mutexUnlock(&nw->mutex);
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);
    }

    if (!nw->is_connected) {
        Result rc = _nwindowConnect(nw);
        if (R_FAILED(rc)) {
            mutexUnlock(&nw->mutex);
            return rc;
        }
    }

    if (!nw->width)
        nw->width = buf->planes[0].width;
    if (!nw->height)
        nw->height = buf->planes[0].height;
    if (nw->format == ~0U)
        nw->format = buf->format;
    if (!nw->usage)
        nw->usage = buf->usage;

    BqGraphicBuffer bqbuf;
    bqbuf.width = nw->width;
    bqbuf.height = nw->height;
    bqbuf.stride = buf->stride;
    //bqbuf.stride = buf->planes[0].pitch / (u8)(buf->planes[0].color_format >> 3); // this also works
    bqbuf.format = nw->format;
    bqbuf.usage = nw->usage;
    bqbuf.native_handle = &buf->header;

    Result rc = bqSetPreallocatedBuffer(&nw->bq, slot, &bqbuf);
    if (R_SUCCEEDED(rc))
        nw->slots_configured |= 1UL << slot;

    mutexUnlock(&nw->mutex);
    return rc;
}

Result nwindowDequeueBuffer(NWindow* nw, s32* out_slot, NvMultiFence* out_fence)
{
    if (!nw)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    mutexLock(&nw->mutex);

    if (!nw->slots_configured || nw->cur_slot >= 0) {
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
        nw->cur_slot = slot;
        if (out_slot)
            *out_slot = slot;
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
    if (!nwindowIsValid(nw))
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
    if (!nwindowIsValid(nw))
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

Result nwindowReleaseBuffers(NWindow* nw)
{
    if (!nwindowIsValid(nw))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc = 0;
    mutexLock(&nw->mutex);

    if (nw->cur_slot >= 0)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    else if (nw->is_connected && nw->slots_configured) {
        for (u32 i = 0; i < 64; i ++)
            if (nw->slots_configured & (1UL << i))
                bqDetachBuffer(&nw->bq, i);

        rc = _nwindowDisconnect(nw);
    }

    mutexUnlock(&nw->mutex);
    return rc;
}
