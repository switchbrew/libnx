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

/// ImageOrientation
typedef enum {
    AlbumImageOrientation_Unknown0 = 0,          ///< Unknown.
    AlbumImageOrientation_Unknown1 = 1,          ///< Unknown.
    AlbumImageOrientation_Unknown2 = 2,          ///< Unknown.
    AlbumImageOrientation_Unknown3 = 3,          ///< Unknown.
} AlbumImageOrientation;

