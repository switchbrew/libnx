#pragma once
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

// From Android window.h.
/* attributes queriable with query() */
enum {
    NATIVE_WINDOW_WIDTH     = 0,
    NATIVE_WINDOW_HEIGHT    = 1,
    NATIVE_WINDOW_FORMAT    = 2,
//...
//    NATIVE_WINDOW_DEFAULT_WIDTH = 6, //These two return invalid data.
//    NATIVE_WINDOW_DEFAULT_HEIGHT = 7,
};

// From Android window.h.
/* parameter for NATIVE_WINDOW_[API_][DIS]CONNECT */
enum {
    //...
    /* Buffers will be queued after being filled using the CPU
     */
    NATIVE_WINDOW_API_CPU = 2,
    //...
};

// From Android hardware.h.

/**
 * Transformation definitions
 *
 * IMPORTANT NOTE:
 * HAL_TRANSFORM_ROT_90 is applied CLOCKWISE and AFTER HAL_TRANSFORM_FLIP_{H|V}.
 *
 */

enum {
    /* flip source image horizontally (around the vertical axis) */
    HAL_TRANSFORM_FLIP_H    = 0x01,
    /* flip source image vertically (around the horizontal axis)*/
    HAL_TRANSFORM_FLIP_V    = 0x02,
    /* rotate source image 90 degrees clockwise */
    HAL_TRANSFORM_ROT_90    = 0x04,
    /* rotate source image 180 degrees */
    HAL_TRANSFORM_ROT_180   = 0x03,
    /* rotate source image 270 degrees clockwise */
    HAL_TRANSFORM_ROT_270   = 0x07,
};

// From Android window.h.
/* parameter for NATIVE_WINDOW_SET_BUFFERS_TRANSFORM */
enum {
    /* flip source image horizontally */
    NATIVE_WINDOW_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    /* flip source image vertically */
    NATIVE_WINDOW_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    /* rotate source image 90 degrees clock-wise */
    NATIVE_WINDOW_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    /* rotate source image 180 degrees */
    NATIVE_WINDOW_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    /* rotate source image 270 degrees clock-wise */
    NATIVE_WINDOW_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
};

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
