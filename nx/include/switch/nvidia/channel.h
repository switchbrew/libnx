#pragma once

typedef struct NvChannel {
    u32  fd;
    bool has_init;
} NvChannel;

Result nvchannelCreate(NvChannel* c, const char* dev);
void nvchannelClose(NvChannel* c);
