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

/// AlbumFileDateTime. This corresponds to each field in the Album entry filename, prior to the "-".
typedef struct {
    u16 year;                                    ///< Year.
    u8 month;                                    ///< Month.
    u8 day;                                      ///< Day.
    u8 hour;                                     ///< Hour.
    u8 minute;                                   ///< Minute.
    u8 second;                                   ///< Second.
    u8 unk_x7;                                   ///< Unknown.
} CapsAlbumFileDateTime;

/// AlbumEntryId
typedef struct {
    u64 titleID;                                 ///< titleID.
    CapsAlbumFileDateTime datetime;              ///< \ref CapsAlbumFileDateTime
    u8 unk_x10;                                  ///< Unknown.
    u8 unk_x11;                                  ///< Unknown.
    u8 pad[6];                                   ///< Padding?
} CapsAlbumEntryId;

/// AlbumEntry
typedef struct {
    u8 unk_x0[0x8];
    CapsAlbumEntryId id;
} CapsAlbumEntry;

/// AlbumFileEntry
typedef struct {
    u8 unk_x0[0x20];
} AlbumFileEntry;

/// ApplicationAlbumEntry
typedef struct {
    u8 data[0x20];
} CapsApplicationAlbumEntry;

/// ImageOrientation
typedef enum {
    AlbumImageOrientation_Unknown0 = 0,          ///< Unknown.
    AlbumImageOrientation_Unknown1 = 1,          ///< Unknown.
    AlbumImageOrientation_Unknown2 = 2,          ///< Unknown.
    AlbumImageOrientation_Unknown3 = 3,          ///< Unknown.
} AlbumImageOrientation;

// Get the ShimLibraryVersion.
u64 capsGetShimLibraryVersion(void);

