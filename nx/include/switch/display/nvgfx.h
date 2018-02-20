#pragma once
#include "../types.h"

Result nvgfxInitialize(void);
void nvgfxExit(void);
Result nvgfxEventWait(u32 syncpt_id, u32 threshold, s32 timeout);
Result nvgfxSubmitGpfifo(void);
Result nvgfxGetFramebuffer(u8 **buffer, size_t *size);
