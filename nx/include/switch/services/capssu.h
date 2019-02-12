/**
 * @file capssu.h
 * @brief Application screenshot saving (caps:su) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/caps.h"

/// Initialize caps:su. Only available on 4.0.0+.
Result capssuInitialize(void);
void capssuExit(void);

/// Same as \ref capssuSaveScreenShotEx0, except this uses an all-zero CapsScreenShotAttribute where the first u32 is set to attr_val. attr_val can be zero.
Result capssuSaveScreenShot(const void* buffer, size_t size, u32 unk, u32 attr_val, CapsApplicationAlbumEntry *out);

/// Saves an Album screenshot using the specified gfx data in buffer (1280x720 RGBA8), size must be at least 0x384000. unk can be zero.
Result capssuSaveScreenShotEx0(const void* buffer, size_t size, CapsScreenShotAttribute *attr, u32 unk, CapsApplicationAlbumEntry *out);

