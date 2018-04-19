/**
 * @file nacp.h
 * @brief Control.nacp structure / related code for nacp.
 * @copyright libnx Authors
 */

#pragma once

/// Language entry. These strings are UTF-8.
typedef struct {
    char name[0x200];
    char author[0x100];
} NacpLanguageEntry;

typedef struct {
    NacpLanguageEntry lang[16];

    u8  x3000_unk[0x24];////Normally all-zero?
    u32 x3024_unk;
    u32 x3028_unk;
    u32 x302C_unk;
    u32 x3030_unk;
    u32 x3034_unk;
    u64 titleID0;

    u8 x3040_unk[0x20];
    char version[0x10];

    u64 titleID_DlcBase;
    u64 titleID1;

    u32 x3080_unk;
    u32 x3084_unk;
    u32 x3088_unk;
    u8 x308C_unk[0x24];//zeros?

    u64 titleID2;
    u64 titleIDs[7];//"Array of application titleIDs, normally the same as the above app-titleIDs. Only set for game-updates?"

    u32 x30F0_unk;
    u32 x30F4_unk;

    u64 titleID3;//"Application titleID. Only set for game-updates?"

    char bcatPassphrase[0x40];
    u8 x3140_unk[0xEC0];//Normally all-zero?
} NacpStruct;

/// Get the NacpLanguageEntry from the input nacp corresponding to the current system language (this may fallback to other languages when needed). Output langentry is NULL if none found / content of entry is empty.
Result nacpGetLanguageEntry(NacpStruct* nacp, NacpLanguageEntry** langentry);

