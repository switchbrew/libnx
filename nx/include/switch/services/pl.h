/**
 * @file pl.h
 * @brief pl:u service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum
{
    PlSharedFontType_Standard             = 0,  ///< Japan, US and Europe
    PlSharedFontType_ChineseSimplified    = 1,  ///< Chinese Simplified
    PlSharedFontType_ExtChineseSimplified = 2,  ///< Extended Chinese Simplified
    PlSharedFontType_ChineseTraditional   = 3,  ///< Chinese Traditional
    PlSharedFontType_KO                   = 4,  ///< Korean (Hangul)
    PlSharedFontType_NintendoExt          = 5,  ///< Nintendo Extended. This font only has the special Nintendo-specific characters, which aren't available with the other fonts.
    PlSharedFontType_Total,                     ///< Total fonts supported by this enum.
} PlSharedFontType;

typedef struct {
    u32 type;
    u32 offset;
    u32 size;
    void* address;
} PlFontData;

Result plInitialize(void);
void plExit(void);
Service* plGetServiceSession(void);
void* plGetSharedmemAddr(void);

///< Gets a specific shared-font via SharedFontType, see \ref PlSharedFontType.
Result plGetSharedFontByType(PlFontData* font, u32 SharedFontType);

///< Gets shared font(s).
Result plGetSharedFont(u64 LanguageCode, PlFontData* fonts, size_t max_fonts, size_t* total_fonts);

