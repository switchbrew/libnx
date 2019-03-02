/**
 * @file caps.h
 * @brief Common caps (caps:*) service IPC header.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"

typedef struct {
    u32 unk_x0;
    u8 unk_x4[0x3c];
} CapsScreenShotAttribute;

/// AlbumEntry
typedef struct {
    u8 unk_x0[0x20];
} CapsAlbumEntry;

/// ApplicationAlbumEntry
typedef struct {
    u8 unk_x0[0x20];
} CapsApplicationAlbumEntry;

