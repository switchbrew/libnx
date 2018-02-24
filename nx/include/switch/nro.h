/**
 * @file nro.h
 * @brief NRO headers.
 * @copyright libnx Authors
 */

#pragma once

#define NROHEADER_MAGICNUM 0x304f524e

#define ASSETHEADER_MAGICNUM 0x54455341
#define ASSETHEADER_VERSION 0

/// Entry for each segment in the codebin.
typedef struct {
    u32 FileOff;
    u32 Size;
} NsoSegment;

/// Offset 0x0 in the NRO.
typedef struct {
    u32 unused;
    u32 modOffset;
    u8 Padding[8];
} NroStart;

/// This follows NroStart, the actual nro-header.
typedef struct {
    u32 Magic;
    u32 Unk1;
    u32 size;
    u32 Unk2;
    NsoSegment Segments[3];
    u32 bssSize;
    u32 Unk3;
    u8  BuildId[0x20];
    u8  Padding[0x20];
} NroHeader;

/// Custom asset section.
typedef struct {
    u64 offset;
    u64 size;
} AssetSection;

/// Custom asset header.
typedef struct {
    u32 magic;
    u32 version;
    AssetSection icon;
    AssetSection nacp;
    AssetSection romfs;
} AssetHeader;

