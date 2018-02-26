/**
 * @file nro.h
 * @brief NRO headers.
 * @copyright libnx Authors
 */

#pragma once

#define NROHEADER_MAGIC 0x304f524e

#define NROASSETHEADER_MAGIC 0x54455341
#define NROASSETHEADER_VERSION 0

/// Entry for each segment in the codebin.
typedef struct {
    u32 file_off;
    u32 size;
} NroSegment;

/// Offset 0x0 in the NRO.
typedef struct {
    u32 unused;
    u32 mod_offset;
    u8  padding[8];
} NroStart;

/// This follows NroStart, the actual nro-header.
typedef struct {
    u32 magic;
    u32 unk1;
    u32 size;
    u32 unk2;
    NroSegment segments[3];
    u32 bss_size;
    u32 unk3;
    u8  build_id[0x20];
    u8  padding[0x20];
} NroHeader;

/// Custom asset section.
typedef struct {
    u64 offset;
    u64 size;
} NroAssetSection;

/// Custom asset header.
typedef struct {
    u32 magic;
    u32 version;
    NroAssetSection icon;
    NroAssetSection nacp;
    NroAssetSection romfs;
} NroAssetHeader;

