#pragma once
#include "../kernel/mutex.h"
#include "../kernel/event.h"
#include "../services/vi.h"
#include "../nvidia/graphic_buffer.h"
#include "types.h"
#include "binder.h"
#include "buffer_producer.h"

typedef struct NWindow {
    u32 magic;
    Binder bq;
    Event event;
    Mutex mutex;
    u64 slots_configured;
    u64 slots_requested;
    s32 cur_slot;
    u32 width;
    u32 height;
    u32 format;
    u32 usage;
    BqRect crop;
    u32 scaling_mode;
    u32 transform;
    u32 sticky_transform;
    u32 default_width;
    u32 default_height;
    u32 swap_interval;
    bool is_connected;
    bool consumer_running_behind;
} NWindow;

bool nwindowIsValid(NWindow* nw);
Result nwindowCreate(NWindow* nw, s32 binder_id, bool producer_controlled_by_app);
Result nwindowCreateFromLayer(NWindow* nw, const ViLayer* layer);
void nwindowClose(NWindow* nw);

Result nwindowGetDimensions(NWindow* nw, u32* out_width, u32* out_height);
Result nwindowSetDimensions(NWindow* nw, u32 width, u32 height);
Result nwindowSetCrop(NWindow* nw, s32 left, s32 top, s32 right, s32 bottom);
Result nwindowSetTransform(NWindow* nw, u32 transform);
Result nwindowSetSwapInterval(NWindow* nw, u32 swap_interval);

Result nwindowConfigureBuffer(NWindow* nw, s32 slot, NvGraphicBuffer* buf);
Result nwindowDequeueBuffer(NWindow* nw, s32* out_slot, NvMultiFence* out_fence);
Result nwindowCancelBuffer(NWindow* nw, s32 slot, const NvMultiFence* fence);
Result nwindowQueueBuffer(NWindow* nw, s32 slot, const NvMultiFence* fence);
void nwindowReleaseBuffers(NWindow* nw);

static inline bool nwindowIsConsumerRunningBehind(NWindow* nw)
{
    return nw->consumer_running_behind;
}
