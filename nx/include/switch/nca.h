/**
 * @file nca.h
 * @brief NCA structure.
 * @copyright libnx Authors
 */

#pragma once

#define PFS0HEADER_MAGIC 0x30534650

/// Offset 0x0 in the NSP file.
typedef struct {
    u32 magic;
    u32 number_of_files;
    u32 size_of_string_table;
    u32 reserved;
} Pfs0Header;

/// These follow after the Pfs0Header, based on the number of files in the header.
typedef struct {
    u64 offset;
    u64 size;
    u32 name_offset;
    u32 reserved;
} Pfs0FileEntry;
