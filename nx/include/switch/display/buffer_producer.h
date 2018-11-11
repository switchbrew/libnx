#pragma once
#include "types.h"
#include "binder.h"
#include "../nvidia/fence.h"

typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
} BqRect;

typedef struct {
    struct { s64 timestamp; } PACKED;
    s32 isAutoTimestamp;
    BqRect crop;
    s32 scalingMode;
    u32 transform; // See the NATIVE_WINDOW_TRANSFORM_* enums.
    u32 stickyTransform;
    u32 unk;
    u32 swapInterval;
    NvMultiFence fence;
} BqBufferInput;

typedef struct {
    u32 width;
    u32 height;
    u32 transformHint;
    u32 numPendingBuffers;
} BqBufferOutput;

typedef struct {
    u32 width;
    u32 height;
    u32 stride;
    u32 format;
    u32 usage;
    NativeHandle* native_handle;
} BqGraphicBuffer;

Result bqRequestBuffer(Binder *b, s32 bufferIdx, BqGraphicBuffer *buf);
Result bqDequeueBuffer(Binder *b, bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, NvMultiFence *fence);
Result bqDetachBuffer(Binder *b, s32 slot);
Result bqQueueBuffer(Binder *b, s32 buf, const BqBufferInput *input, BqBufferOutput *output);
Result bqCancelBuffer(Binder *b, s32 buf, const NvMultiFence *fence);
Result bqQuery(Binder *b, s32 what, s32* value);
Result bqConnect(Binder *b, s32 api, bool producerControlledByApp, BqBufferOutput *output);
Result bqDisconnect(Binder *b, s32 api);
Result bqSetPreallocatedBuffer(Binder *b, s32 buf, const BqGraphicBuffer *input);
