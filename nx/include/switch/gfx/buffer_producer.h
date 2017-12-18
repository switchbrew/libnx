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

Result bufferProducerRequestBuffer(s32 bufferIdx);
Result bufferProducerDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, bufferProducerFence *fence);
Result bufferProducerDetachBuffer(s32 slot);
Result bufferProducerQueueBuffer(s32 buf, bufferProducerQueueBufferInput *input, bufferProducerQueueBufferOutput *output);
Result bufferProducerQuery(s32 what, s32* value);
Result bufferProducerConnect(s32 api, bool producerControlledByApp, bufferProducerQueueBufferOutput *output);
Result bufferProducerDisconnect(s32 api);
Result bufferProducerTegraBufferInit(s32 buf, u8 input[0x178]);
