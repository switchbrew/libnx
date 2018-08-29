#pragma once
#include "../types.h"

Result nvgfxInitialize(void);
void nvgfxExit(void);
Result nvgfxSubmitGpfifo(void);
Result nvgfxGetFramebuffer(u8 **buffer, size_t *size, u32 *handle);
