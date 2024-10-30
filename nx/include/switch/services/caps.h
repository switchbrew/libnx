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
    AlbumReportOption_Disable = 0,              ///< Don't display the screenshot-taken Overlay-applet notification.
    AlbumReportOption_Enable = 1,               ///< Display the screenshot-taken Overlay notification.
} AlbumReportOption;

typedef enum {
    CapsAlbumStorage_Nand = 0,                  ///< Nand
    CapsAlbumStorage_Sd   = 1,                  ///< Sd
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

/// ScreenShotDecoderFlag
typedef enum {
    CapsScreenShotDecoderFlag_None                  = 0,      ///< No special processing.
    CapsScreenShotDecoderFlag_EnableFancyUpsampling = BIT(0), ///< See libjpeg-turbo do_fancy_upsampling.
    CapsScreenShotDecoderFlag_EnableBlockSmoothing  = BIT(1), ///< See libjpeg-turbo do_block_smoothing.
} CapsScreenShotDecoderFlag;

/// ScreenShotDecodeOption
typedef struct {
    u64 flags;          ///< Bitflags, see \ref CapsScreenShotDecoderFlag.
    u64 reserved[0x3];  ///< Reserved. Unused by official sw.
} CapsScreenShotDecodeOption;

/// AlbumFileDateTime. This corresponds to each field in the Album entry filename, prior to the "-": "YYYYMMDDHHMMSSII".
typedef struct {
    u16 year;                                   ///< Year.
    u8 month;                                   ///< Month.
    u8 day;                                     ///< Day of the month.
    u8 hour;                                    ///< Hour.
    u8 minute;                                  ///< Minute.
    u8 second;                                  ///< Second.
    u8 id;                                      ///< Unique ID for when there's multiple Album files with the same timestamp.
} CapsAlbumFileDateTime;

/// AlbumEntryId
typedef struct {
    u64 application_id;                         ///< ApplicationId
    CapsAlbumFileDateTime datetime;             ///< \ref CapsAlbumFileDateTime
    u8 storage;                                 ///< \ref CapsAlbumStorage
    u8 content;                                 ///< \ref CapsAlbumFileContents
    u8 unknown_12;                              ///< [19.0.0+]
    u8 unknown_13;                              ///< [19.0.0+]
    u8 pad_x14[0x4];                            ///< padding
} CapsAlbumFileId;

/// AlbumEntry
typedef struct {
    u64 size;                                   ///< Size.
    CapsAlbumFileId file_id;                    ///< \ref CapsAlbumFileId
} CapsAlbumEntry;

/// ApplicationAlbumEntry
typedef struct {
    union {
        u8 data[0x20];                          ///< Data.

        struct {
            u8 unk_x0[0x20];                    ///< aes256 with random key over \ref AlbumEntry.
        } v0; ///< Pre-7.0.0

        struct {
            u64 size;                           ///< size of the entry
            u64 hash;                           ///< aes256 with hardcoded key over \ref AlbumEntry.
            CapsAlbumFileDateTime datetime;     ///< \ref CapsAlbumFileDateTime
            u8 storage;                         ///< \ref CapsAlbumStorage
            u8 content;                         ///< \ref CapsAlbumFileContents
            u8 pad_x1a[0x5];                    ///< padding
            u8 unk_x1f;                         ///< Set to 1 by official software
        } v1; ///< [7.0.0+]
    };
} CapsApplicationAlbumEntry;

/// ApplicationAlbumFileEntry
typedef struct {
    CapsApplicationAlbumEntry entry;            ///< \ref CapsApplicationAlbumEntry
    CapsAlbumFileDateTime datetime;             ///< \ref CapsAlbumFileDateTime
    u64 unk_x28;                                ///< Unknown.
} CapsApplicationAlbumFileEntry;

/// ApplicationData
typedef struct {
    u8 userdata[0x400];                         ///< UserData.
    u32 size;                                   ///< UserData size.
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
    s64 count;                                            ///< Count.
    s64 size;                                             ///< Size. Used storage space.
    u32 flags;                                            ///< \ref CapsAlbumContentsUsageFlag
    u8 file_contents;                                     ///< \ref CapsAlbumFileContents
    u8 pad_x15[0x3];                                      ///< Unused
} CapsAlbumContentsUsage;

typedef struct {
    CapsAlbumContentsUsage usages[2];                     ///< \ref CapsAlbumContentsUsage
} CapsAlbumUsage2;

typedef struct {
    CapsAlbumContentsUsage usages[3];                     ///< \ref CapsAlbumContentsUsage
} CapsAlbumUsage3;

typedef struct {
    CapsAlbumContentsUsage usages[16];                    ///< \ref CapsAlbumContentsUsage
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

/// LoadAlbumScreenShotImageOutput
typedef struct {
    s64 width;                                            ///< Width. Official sw copies this to a s32 output field.
    s64 height;                                           ///< Height. Official sw copies this to a s32 output field.
    CapsScreenShotAttribute attr;                         ///< \ref CapsScreenShotAttribute
    u8 unk_x50[0x400];                                    ///< Unused.
} CapsLoadAlbumScreenShotImageOutput;

/// AlbumFileContentsFlag
typedef enum {
    CapsAlbumFileContentsFlag_ScreenShot = BIT(0),        ///< Query for ScreenShot files.
    CapsAlbumFileContentsFlag_Movie      = BIT(1),        ///< Query for Movie files.
} CapsAlbumFileContentsFlag;

/// AlbumCache
typedef struct {
    u64 count;                                            ///< Count
    u64 unk_x8;                                           ///< Unknown
} CapsAlbumCache;

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

