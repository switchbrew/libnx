#pragma once
#include "../nvidia/map.h"
#include "native_window.h"

typedef struct Framebuffer
{
    NWindow *win;
    NvMap map;
    void* buf;
    u32 stride;
    u32 width_aligned;
    u32 height_aligned;
    u32 num_fbs;
    u32 fb_size;
    bool has_init;
} Framebuffer;

Result framebufferCreate(Framebuffer* fb, NWindow *win, u32 width, u32 height, u32 format, u32 num_fbs);
void framebufferClose(Framebuffer* fb);

void* framebufferBegin(Framebuffer* fb, u32* out_stride);
void framebufferEnd(Framebuffer* fb);
