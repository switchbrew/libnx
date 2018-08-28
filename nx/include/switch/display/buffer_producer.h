#pragma once
#include "types.h"
#include "../nvidia/ioctl.h"

typedef struct {
    u32 is_valid;
    nvioctl_fence nv_fences[4];
} PACKED BqFence;

typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
} PACKED BqRect;

typedef struct {
    s64 timestamp;
    s32 isAutoTimestamp;
    BqRect crop;
    s32 scalingMode;
    u32 transform; // See the NATIVE_WINDOW_TRANSFORM_* enums.
    u32 stickyTransform;
    u32 unk[2];
    BqFence fence;
} PACKED BqQueueBufferInput;

typedef struct {
    u32 width;
    u32 height;
    u32 transformHint;
    u32 numPendingBuffers;
} PACKED BqQueueBufferOutput;

typedef struct {
    u32 magic;
    u32 width;
    u32 height;
    u32 stride;
    u32 format;
    u32 usage;

    u32 pid;
    u32 refcount;

    u32 numFds;
    u32 numInts;

    struct { // Actual size is numFds*4 + numInts*4.
        u32 unk_x0;
        u32 nvmap_handle0;
        u32 unk_x8;
        u32 unk_xc;
        u32 unk_x10;
        u32 unk_x14;
        u32 unk_x18;
        u32 unk_x1c;
        u32 unk_x20;
        u32 width_unk0;
        u32 buffer_size0;
        u32 unk_x2c;
        u32 unk_x30;
        u32 width_unk1;
        u32 height_unk;
        u32 flags;
        u32 unk_x40;
        u32 unk_x44;
        u32 byte_stride;
        u32 nvmap_handle1;
        u32 buffer_offset;
        u32 unk_x54;
        u32 unk_x58;
        u32 unk_x5c;
        u32 unk_x60;
        u32 unk_x64;
        u32 unk_x68;
        u32 buffer_size1;
        u32 unk_x70[0x33]; // Normally all-zero.
        u64 timestamp;
    } PACKED data;
} PACKED BqGraphicBuffer;

Result bqInitialize(Binder *session);
void bqExit(void);

Result bqRequestBuffer(s32 bufferIdx, BqGraphicBuffer *buf);
Result bqDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, BqFence *fence);
Result bqDetachBuffer(s32 slot);
Result bqQueueBuffer(s32 buf, BqQueueBufferInput *input, BqQueueBufferOutput *output);
Result bqQuery(s32 what, s32* value);
Result bqConnect(s32 api, bool producerControlledByApp, BqQueueBufferOutput *output);
Result bqDisconnect(s32 api);
Result bqGraphicBufferInit(s32 buf, BqGraphicBuffer *input);
