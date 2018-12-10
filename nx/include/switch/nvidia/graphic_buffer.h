#pragma once
#include "../display/types.h"
#include "types.h"

typedef struct {
    u32 width;
    u32 height;
    NvColorFormat color_format;
    NvLayout layout;
    u32 pitch;
    u32 unused; // usually this field contains the nvmap handle, but it's completely unused/overwritten during marshalling
    u32 offset;
    NvKind kind;
    u32 block_height_log2;
    NvDisplayScanFormat scan;
    u32 second_field_offset;
    u64 flags;
    u64 size;
    u32 unk[6]; // compression related
} NvSurface;

typedef struct {
    NativeHandle header;
    s32 unk0;       // -1
    s32 nvmap_id;   // nvmap object id
    u32 unk2;       // 0
    u32 magic;      // 0xDAFFCAFF
    u32 pid;        // 42
    u32 type;       // ?
    u32 usage;      // GRALLOC_USAGE_* bitmask
    u32 format;     // PIXEL_FORMAT_*
    u32 ext_format; // copy of the above (in most cases)
    u32 stride;     // in pixels!
    u32 total_size; // in bytes
    u32 num_planes; // usually 1
    u32 unk12;      // 0
    NvSurface planes[3];
    u64 unused;     // official sw writes a pointer to bookkeeping data here, but it's otherwise completely unused/overwritten during marshalling
} NvGraphicBuffer;
