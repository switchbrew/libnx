/**
 * @file pl.h
 * @brief pl:u service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// SharedFontType
typedef enum {
    PlSharedFontType_Standard             = 0,  ///< Japan, US and Europe
    PlSharedFontType_ChineseSimplified    = 1,  ///< Chinese Simplified
    PlSharedFontType_ExtChineseSimplified = 2,  ///< Extended Chinese Simplified
    PlSharedFontType_ChineseTraditional   = 3,  ///< Chinese Traditional
    PlSharedFontType_KO                   = 4,  ///< Korean (Hangul)
    PlSharedFontType_NintendoExt          = 5,  ///< Nintendo Extended. This font only has the special Nintendo-specific characters, which aren't available with the other fonts.
    PlSharedFontType_Total,                     ///< Total fonts supported by this enum.
} PlSharedFontType;

/// FontData
typedef struct {
    u32 type;                                   ///< \ref PlSharedFontType
    u32 offset;                                 ///< Offset of the font in sharedmem.
    u32 size;                                   ///< Size of the font.
    void* address;                              ///< Address of the actual font.
} PlFontData;

/// Initialize pl.
Result plInitialize(void);

/// Exit pl.
void plExit(void);

/// Gets the Service object for the actual pl service session.
Service* plGetServiceSession(void);

/// Gets the address of the SharedMemory.
void* plGetSharedmemAddr(void);

///< Gets a specific shared-font via \ref PlSharedFontType.
Result plGetSharedFontByType(PlFontData* font, PlSharedFontType SharedFontType);

///< Gets shared font(s).
Result plGetSharedFont(u64 LanguageCode, PlFontData* fonts, s32 max_fonts, s32* total_fonts);

