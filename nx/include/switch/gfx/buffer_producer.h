#pragma once

#include <switch/gfx/nvioctl.h>

typedef struct {
    u32 is_valid;
    nvioctl_fence nv_fences[4];
} PACKED bufferProducerFence;

typedef struct {
    s64 timestamp;
    s32 isAutoTimestamp;
    u32 crop[4];//Rect
    s32 scalingMode;
    u32 transform;
    u32 stickyTransform;
    u32 unk[2];
    bufferProducerFence fence;
} PACKED bufferProducerQueueBufferInput;

typedef struct {
    u32 width;
    u32 height;
    u32 transformHint;
    u32 numPendingBuffers;
} PACKED bufferProducerQueueBufferOutput;

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

    union {
        u32 rawdata[0x144>>2];//Actual size is numFds*4 + numInts*4.

        struct {
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
            u32 unk_x70[0x33];//Normally all-zero.
            u64 timestamp;//Unknown, some timestamp perhaps?
        } data;
    };
} PACKED bufferProducerGraphicBuffer;

//From Android window.h.
/* attributes queriable with query() */
enum {
    NATIVE_WINDOW_WIDTH     = 0,
    NATIVE_WINDOW_HEIGHT    = 1,
    NATIVE_WINDOW_FORMAT    = 2,
//...
//    NATIVE_WINDOW_DEFAULT_WIDTH = 6, //These two return invalid data.
//    NATIVE_WINDOW_DEFAULT_HEIGHT = 7,
};

//From Android window.h.
/* parameter for NATIVE_WINDOW_[API_][DIS]CONNECT */
enum {
    //...
    /* Buffers will be queued after being filled using the CPU
     */
    NATIVE_WINDOW_API_CPU = 2,
    //...
};

Result bufferProducerInitialize(binderSession *session);
void bufferProducerExit();

Result bufferProducerRequestBuffer(s32 bufferIdx, bufferProducerGraphicBuffer *buf);
Result bufferProducerDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, bufferProducerFence *fence);
Result bufferProducerDetachBuffer(s32 slot);
Result bufferProducerQueueBuffer(s32 buf, bufferProducerQueueBufferInput *input, bufferProducerQueueBufferOutput *output);
Result bufferProducerQuery(s32 what, s32* value);
Result bufferProducerConnect(s32 api, bool producerControlledByApp, bufferProducerQueueBufferOutput *output);
Result bufferProducerDisconnect(s32 api);
Result bufferProducerGraphicBufferInit(s32 buf, bufferProducerGraphicBuffer *input);
