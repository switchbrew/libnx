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
#include "../../services/ncm_types.h"

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

/**
 * @brief Mounts the Application's RomFS.
 * @param name Device mount name.
 * @remark This function is intended to be used to access one's own RomFS.
 *         If the application is running as NRO, it mounts the embedded RomFS section inside the NRO.
 *         If on the other hand it's an NSO, it behaves identically to \ref romfsMountFromCurrentProcess.
 */
Result romfsMountSelf(const char *name);

/**
 * @brief Mounts RomFS from an open file.
 * @param file FsFile of the RomFS image.
 * @param offset Offset of the RomFS within the file.
 * @param name Device mount name.
 */
Result romfsMountFromFile(FsFile file, u64 offset, const char *name);

/**
 * @brief Mounts RomFS from an open storage.
 * @param storage FsStorage of the RomFS image.
 * @param offset Offset of the RomFS within the storage.
 * @param name Device mount name.
 */
Result romfsMountFromStorage(FsStorage storage, u64 offset, const char *name);

/**
 * @brief Mounts RomFS using the current process host program RomFS.
 * @param name Device mount name.
 */
Result romfsMountFromCurrentProcess(const char *name);

/**
 * @brief Mounts RomFS of a running program.
 * @note Permission needs to be set in the NPDM.
 * @param program_id ProgramId to mount.
 * @param name Device mount name.
 */
Result romfsMountDataStorageFromProgram(u64 program_id, const char *name);

/**
 * @brief Mounts RomFS from a file path in a mounted fsdev device.
 * @param path File path.
 * @param offset Offset of the RomFS within the file.
 * @param name Device mount name.
 */
Result romfsMountFromFsdev(const char *path, u64 offset, const char *name);

/**
 * @brief Mounts RomFS from SystemData.
 * @param dataId SystemDataId to mount.
 * @param storageId Storage ID to mount from.
 * @param name Device mount name.
 */
Result romfsMountFromDataArchive(u64 dataId, NcmStorageId storageId, const char *name);

/// Unmounts the RomFS device.
Result romfsUnmount(const char *name);

/// Wrapper for \ref romfsMountSelf with the default "romfs" device name.
static inline Result romfsInit(void)
{
    return romfsMountSelf("romfs");
}

/// Wrapper for \ref romfsUnmount with the default "romfs" device name.
static inline Result romfsExit(void)
{
    return romfsUnmount("romfs");
}

/// Opaque handle to a RomFS mount
typedef struct romfs_mount romfs_mount;

/// Object for interacting with a romfs_file
typedef struct
{
    romfs_mount *mount; ///< The RomFS mount the file is associated with.
    romfs_file  *file;  ///< Detail about the actual RomFS file.

    u64 offset; ///< The starting offset in RomFS for file data.
    u64 pos;    ///< Current read position into the file.

    int err;
} romfs_fileobj;

/**
 * @brief Finds a RomFS mount by the given name, the default mount name is "romfs"
 * @param name The name of the mount to search for
 * @param mount A pointer to a romfs_mount pointer to fill out with the mount information
 */
Result romfsFindMount(const char *name, romfs_mount **mount);

/**
 * @brief Finds a file in RomFS automatically determining the mount
 * @param path The path to the file
 * @param file A pointer to a romfs_fileobj structure to fill out
 * @remark The path structure should follow <mount name>:<path> (i.e. romfs:/data/file.txt)
 *         If no mount name is provided the default of "romfs" will be used.
 */
Result romfsFindFile(const char *path, romfs_fileobj *file);

/**
 * @brief Finds a file in a specific RomFS mount
 * @param mount The mount to search in
 * @param path The path to the file
 * @param file A pointer to a romfs_fileobj structure to fill out
 * @remark The mount name prefix is not required and is ignored if provided
 */
Result romfsFindFileInMount(romfs_mount *mount, const char *path, romfs_fileobj *file);

/**
 * @brief Wrapper function for turning a romfs_file into a romfs_fileobj to be operated on, this
 *        is useful for creating readable files from a romfs_direntry
 * @param mount The mount the file came from
 * @param file The file information
 */
romfs_fileobj romfsFileObj(romfs_mount *mount, romfs_file *file);

/**
 * @brief Reads data from the specified RomFS file.
 * @param file The RomFS file to read from.
 * @param buffer The buffer to read data into.
 * @param size The number of bytes to read.
 * @param offset The offset in bytes from the beginning of the file to start reading from.
 * @param nread A pointer in which the number of total bytes read will be written to.
 * @remark The file's position pointer is not updated by this function.
 */
Result romfsReadFile(romfs_fileobj *file, void *buffer, u64 size, u64 offset, u64 *nread);

typedef struct
{
    romfs_mount *mount;    ///< The RomFS mount associated with the directory.
    romfs_dir   *dir;      ///< Information about the directory being searched.
    int         state;     ///< Current iteration count or error code
    u32         childDir;  ///< Next child directory of the directory.
    u32         childFile; ///< Next child file of the directory.
} romfs_diriter;

typedef enum
{
    RomfsDirEntryType_File = 0,
    RomfsDirEntryType_Dir
} RomfsDirEntryType;

typedef struct
{
    RomfsDirEntryType type; ///< Type of this entry.
    union
    {
        romfs_file *file;  ///< Entry information if type is RomfsDirEntryType_File.
        romfs_dir  *dir;   ///< Entry information if type is RomfsDirEntryType_Dir.
    };

    const char *name; ///< Basename of the entry, not null-terminated, UTF-8 coded.
    u32 name_len;     ///< Length in bytes of the basename.
} romfs_direntry;

/**
 * @brief Initialises the directory iterator for seaching at the given path, automatically determines mount.
 * @param path The directory path to search.
 * @param iter The directory iterator to fill out with found information.
 * @remark The path structure should follow <mount name>:<path> (i.e. romfs:/data/folder).
 *         If no mount name is provided the default of "romfs" will be used.
 */
Result romfsDirOpen(const char *path, romfs_diriter *iter);

/**
 * @brief Initialises the directory iterator for seaching at the given path in the supplied mount.
 * @param mount The RomFS mount to search in.
 * @param path The directory path to search.
 * @param iter The directory iterator to fill out with found information.
 * @remark The mount does not need to be specified in the path and is ignored if provided.
 */
Result romfsDirOpenWithMount(romfs_mount *mount, const char *path, romfs_diriter *iter);

/**
 * @brief Gets the next entry in the directory, returns false on error or when no more entries are found.
 * @param iter The directory iterator.
 * @param entry The entry to fill out with information.
 */
bool romfsDirNext(romfs_diriter *iter, romfs_direntry *entry);
