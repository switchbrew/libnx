#pragma once
#include "types.h"
#include "ioctl.h"

typedef struct NvChannel {
    u32  fd;
    bool has_init;
} NvChannel;

Result nvChannelCreate(NvChannel* c, const char* dev);
void   nvChannelClose(NvChannel* c);

Result nvChannelSetPriority(NvChannel* c, NvChannelPriority prio);
Result nvChannelSetTimeout(NvChannel* c, u32 timeout);
