/**
 * @file caps.h
 * @brief Common caps (caps:*) service IPC header.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/acc.h"

/// ImageOrientation
typedef enum {
    AlbumImageOrientation_Unknown0 = 0,         ///< Unknown. Default.
    AlbumImageOrientation_Unknown1 = 1,         ///< Unknown.
    AlbumImageOrientation_Unknown2 = 2,         ///< Unknown.
    AlbumImageOrientation_Unknown3 = 3,         ///< Unknown.
} AlbumImageOrientation;

/// AlbumReportOption
typedef enum {
    AlbumReportOption_Unknown0 = 0,             ///< Unknown.
    AlbumReportOption_Unknown1 = 1,             ///< Unknown.
    AlbumReportOption_Unknown2 = 2,             ///< Unknown.
    AlbumReportOption_Unknown3 = 3,             ///< Unknown.
} AlbumReportOption;

typedef struct {
    u32 unk_x0;                                 ///< Always set to 0 by official sw.
    u32 orientation;                            ///< \ref AlbumImageOrientation
    u32 unk_x8;                                 ///< Always set to 0 by official sw.
    u32 unk_xc;                                 ///< Always set to 1 by official sw.
    u8 unk_x10[0x30];                           ///< Always set to 0 by official sw.
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

/// ApplicationAlbumEntry
typedef struct {
    u8 data[0x20];
} CapsApplicationAlbumEntry;

/// ApplicationData
typedef struct {
    u8 userdata[0x400];                          ///< UserData.
    u32 size;                                    ///< UserData size.
} CapsApplicationData;

/// UserIdList
typedef struct {
    union { u128 userIDs[ACC_USER_LIST_SIZE]; } PACKED;   ///< userIDs.
    u8 count;                                             ///< Total userIDs.
    u8 pad[7];                                            ///< Padding.
} CapsUserIdList;

// Get the ShimLibraryVersion.
u64 capsGetShimLibraryVersion(void);

