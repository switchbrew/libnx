/**
 * @file caps.h
 * @brief Common caps (caps:*) service IPC header.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
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
    AlbumReportOption_Disable = 0,             ///< Don't display the screenshot-taken Overlay-applet notification.
    AlbumReportOption_Enable = 1,              ///< Display the screenshot-taken Overlay notification.
} AlbumReportOption;

typedef enum {
    CapsAlbumStorage_Nand = 0,        ///< Nand
    CapsAlbumStorage_Sd   = 1,        ///< Sd
} CapsAlbumStorage;

/// ContentType
typedef enum {
    CapsContentType_Screenshot = 0,             ///< Album screenshots.
    CapsContentType_Movie      = 1,             ///< Album videos.
    CapsContentType_ExtraMovie = 3,             ///< Videos recorded by the current host Application via \ref grcCreateMovieMaker.
} CapsContentType;

/// ScreenShotAttribute
typedef struct {
    u32 unk_x0;                                 ///< Always set to 0 by official sw.
    u32 orientation;                            ///< \ref AlbumImageOrientation
    u32 unk_x8;                                 ///< Always set to 0 by official sw.
    u32 unk_xc;                                 ///< Always set to 1 by official sw.
    u8 unk_x10[0x30];                           ///< Always set to 0 by official sw.
} CapsScreenShotAttribute;

/// ScreenShotAttributeForApplication. Only unk_x0 is used by official sw.
typedef struct {
    u32 unk_x0;                                 ///< Unknown.
    u8 unk_x4;                                  ///< Unknown.
    u8 unk_x5;                                  ///< Unknown.
    u8 unk_x6;                                  ///< Unknown.
    u8 pad;                                     ///< Padding.
    u32 unk_x8;                                 ///< Unknown.
    u32 unk_xc;                                 ///< Unknown.
    u32 unk_x10;                                ///< Unknown.
    u32 unk_x14;                                ///< Unknown.
    u32 unk_x18;                                ///< Unknown.
    u32 unk_x1c;                                ///< Unknown.
    u16 unk_x20;                                ///< Unknown.
    u16 unk_x22;                                ///< Unknown.
    u16 unk_x24;                                ///< Unknown.
    u16 unk_x26;                                ///< Unknown.
    u8 reserved[0x18];                          ///< Always zero.
} CapsScreenShotAttributeForApplication;

/// ScreenShotDecodeOption
typedef struct {
    u8 unk_x0[0x20];                            ///< Unknown. Set to all-zero by official sw.
} CapsScreenShotDecodeOption;

/// AlbumFileDateTime. This corresponds to each field in the Album entry filename, prior to the "-": "YYYYMMDDHHMMSSII".
typedef struct {
    u16 year;                                    ///< Year.
    u8 month;                                    ///< Month.
    u8 day;                                      ///< Day of the month.
    u8 hour;                                     ///< Hour.
    u8 minute;                                   ///< Minute.
    u8 second;                                   ///< Second.
    u8 id;                                       ///< Unique ID for when there's multiple Album files with the same timestamp.
} CapsAlbumFileDateTime;

/// AlbumEntryId
typedef struct {
    u64 application_id;                          ///< ApplicationId
    CapsAlbumFileDateTime datetime;              ///< \ref CapsAlbumFileDateTime
    u8 storage;                                  ///< \ref CapsAlbumStorage
    u8 content;                                  ///< \ref CapsAlbumFileContents
    u32 pad_x12;                                 ///< Set to 0 by official software
    u16 pad_x16;                                 ///< Set to 0 by official software
} PACKED CapsAlbumFileId;

/// AlbumEntry
typedef struct {
    u64 size;                                    ///< Size.
    CapsAlbumFileId file_id;                     ///< \ref CapsAlbumFileId
} CapsAlbumEntry;

/// ApplicationAlbumEntry
typedef struct {
    union {
        u8 data[0x20];                           ///< Data.

        struct {
            u8 unk_x0[0x20];                     ///< Unknown.
        } v0; ///< Pre-7.0.0

        struct {
            u64 size;                           ///< size of the entry
            u64 application_id;                 ///< ApplicationId
            CapsAlbumFileDateTime datetime;     ///< \ref CapsAlbumFileDateTime
            u8 unk_x18[0x8];                    ///< Unknown.
        } v1; ///< [7.0.0+]
    };
} CapsApplicationAlbumEntry;

/// ApplicationAlbumFileEntry
typedef struct {
    CapsApplicationAlbumEntry entry;             ///< \ref CapsApplicationAlbumEntry
    CapsAlbumFileDateTime datetime;              ///< \ref CapsAlbumFileDateTime
    u64 unk_x28;                                 ///< Unknown.
} CapsApplicationAlbumFileEntry;

/// ApplicationData
typedef struct {
    u8 userdata[0x400];                          ///< UserData.
    u32 size;                                    ///< UserData size.
} CapsApplicationData;

/// AlbumFileContents
typedef enum {
    CapsAlbumFileContents_ScreenShot      = 0,
    CapsAlbumFileContents_Movie           = 1,
    CapsAlbumFileContents_ExtraScreenShot = 2,
    CapsAlbumFileContents_ExtraMovie      = 3,
} CapsAlbumFileContents;

typedef enum {
    CapsAlbumContentsUsageFlag_HasGreaterUsage   = BIT(0), ///< Indicates that there are additional files not captured by the count/size fields of CapsAlbumContentsUsage
    CapsAlbumContentsUsageFlag_IsUnknownContents = BIT(1), ///< Indicates that the file is not a known content type
} CapsAlbumContentsUsageFlag;

typedef struct {
    s64 count;
    s64 size;
    u32 flags;
    u8 file_contents;
    u8 pad_x15[0x3];
} CapsAlbumContentsUsage;

typedef struct {
    CapsAlbumContentsUsage usages[2];
} CapsAlbumUsage2;

typedef struct {
    CapsAlbumContentsUsage usages[3];
} CapsAlbumUsage3;

typedef struct {
    CapsAlbumContentsUsage usages[16];
} CapsAlbumUsage16;

/// UserIdList
typedef struct {
    AccountUid uids[ACC_USER_LIST_SIZE];                  ///< \ref AccountUid
    u8 count;                                             ///< Total userIDs.
    u8 pad[7];                                            ///< Padding.
} CapsUserIdList;

/// LoadAlbumScreenShotImageOutputForApplication
typedef struct {
    s64 width;                                            ///< Width. Official sw copies this to a s32 output field.
    s64 height;                                           ///< Height. Official sw copies this to a s32 output field.
    CapsScreenShotAttributeForApplication attr;           ///< \ref CapsScreenShotAttributeForApplication
    CapsApplicationData appdata;                          ///< \ref CapsApplicationData
    u8 reserved[0xac];                                    ///< Unused.
} CapsLoadAlbumScreenShotImageOutputForApplication;

/// Gets the ShimLibraryVersion.
u64 capsGetShimLibraryVersion(void);

/// Gets the default start_datetime.
static inline CapsAlbumFileDateTime capsGetDefaultStartDateTime(void) {
    return (CapsAlbumFileDateTime){.year = 1970, .month = 1, .day = 1};
}

/// Gets the default end_datetime.
static inline CapsAlbumFileDateTime capsGetDefaultEndDateTime(void) {
    return (CapsAlbumFileDateTime){.year = 3000, .month = 1, .day = 1};
}

/// Convert a \ref CapsApplicationAlbumFileEntry to \ref CapsApplicationAlbumEntry.
static inline void capsConvertApplicationAlbumFileEntryToApplicationAlbumEntry(CapsApplicationAlbumEntry *out, CapsApplicationAlbumFileEntry *in) {
    *out = in->entry;
}

/// Convert a \ref CapsApplicationAlbumEntry to \ref CapsApplicationAlbumFileEntry. Should only be used on [7.0.0+].
static inline void capsConvertApplicationAlbumEntryToApplicationAlbumFileEntry(CapsApplicationAlbumFileEntry *out, CapsApplicationAlbumEntry *in) {
    out->entry = *in;
    out->datetime = in->v1.datetime;
    out->unk_x28 = 0;
}

