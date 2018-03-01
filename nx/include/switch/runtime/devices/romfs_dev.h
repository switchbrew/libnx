/**
 * @file romfs_dev.h
 * @brief RomFS driver.
 * @author yellows8
 * @author mtheall
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once

#include "../../types.h"
#include "../../services/fs.h"

/// RomFS header.
typedef struct
{
    u64 headerSize;        ///< Size of the header.
    u64 dirHashTableOff;   ///< Offset of the directory hash table.
    u64 dirHashTableSize;  ///< Size of the directory hash table.
    u64 dirTableOff;       ///< Offset of the directory table.
    u64 dirTableSize;      ///< Size of the directory table.
    u64 fileHashTableOff;  ///< Offset of the file hash table.
    u64 fileHashTableSize; ///< Size of the file hash table.
    u64 fileTableOff;      ///< Offset of the file table.
    u64 fileTableSize;     ///< Size of the file table.
    u64 fileDataOff;       ///< Offset of the file data.
} romfs_header;

/// RomFS directory.
typedef struct
{
    u32 parent; ///< Offset of the parent directory.
    u32 sibling;   ///< Offset of the next sibling directory.
    u32 childDir;  ///< Offset of the first child directory.
    u32 childFile; ///< Offset of the first file.
    u32 nextHash;  ///< Directory hash table pointer.
    u32 nameLen;   ///< Name length.
    uint8_t name[];    ///< Name. (UTF-8)
} romfs_dir;

/// RomFS file.
typedef struct
{
    u32 parent;   ///< Offset of the parent directory.
    u32 sibling;  ///< Offset of the next sibling file.
    u64 dataOff;  ///< Offset of the file's data.
    u64 dataSize; ///< Length of the file's data.
    u32 nextHash; ///< File hash table pointer.
    u32 nameLen;  ///< Name length.
    uint8_t name[];   ///< Name. (UTF-8)
} romfs_file;

struct romfs_mount;

/**
 * @brief Mounts the Application's RomFS.
 * @param mount Output mount handle
 */
Result romfsMount(struct romfs_mount **mount);
static inline Result romfsInit(void)
{
    return romfsMount(NULL);
}

/**
 * @brief Mounts RomFS from an open file.
 * @param file FsFile of the RomFS image.
 * @param offset Offset of the RomFS within the file.
 * @param mount Output mount handle
 */
Result romfsMountFromFile(FsFile file, u64 offset, struct romfs_mount **mount);
static inline Result romfsInitFromFile(FsFile file, u64 offset)
{
    return romfsMountFromFile(file, offset, NULL);
}

/**
 * @brief Mounts RomFS from an open storage.
 * @param storage FsStorage of the RomFS image.
 * @param offset Offset of the RomFS within the storage.
 * @param mount Output mount handle
 */
Result romfsMountFromStorage(FsStorage storage, u64 offset, struct romfs_mount **mount);
static inline Result romfsInitFromStorage(FsStorage storage, u64 offset)
{
    return romfsMountFromStorage(storage, offset, NULL);
}

/// Bind the RomFS mount
Result romfsBind(struct romfs_mount *mount);

/// Unmounts the RomFS device.
Result romfsUnmount(struct romfs_mount *mount);
static inline Result romfsExit(void)
{
    return romfsUnmount(NULL);
}

