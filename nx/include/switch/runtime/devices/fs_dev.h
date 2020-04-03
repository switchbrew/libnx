/**
 * @file fs_dev.h
 * @brief FS driver, using devoptab.
 * @author yellows8
 * @author mtheall
 * @copyright libnx Authors
 */
#pragma once

#include <sys/types.h>
#include "../../services/fs.h"

#define FSDEV_DIRITER_MAGIC 0x66736476 ///< "fsdv"

/// Open directory struct
typedef struct
{
  u32               magic;         ///< "fsdv"
  FsDir             fd;            ///< File descriptor
  ssize_t           index;         ///< Current entry index
  size_t            size;          ///< Current batch size
} fsdev_dir_t;

/// Retrieves a pointer to temporary stage for reading entries
NX_CONSTEXPR FsDirectoryEntry* fsdevDirGetEntries(fsdev_dir_t *dir)
{
  return (FsDirectoryEntry*)(void*)(dir+1);
}

/// Initializes and mounts the sdmc device if accessible.
Result fsdevMountSdmc(void);

/// Mounts the specified SaveData.
Result fsdevMountSaveData(const char *name, u64 application_id, AccountUid uid);

/// Mounts the specified SaveData as ReadOnly.
/// Only available on [2.0.0+].
Result fsdevMountSaveDataReadOnly(const char *name, u64 application_id, AccountUid uid);

/// Mounts the specified BcatSaveData.
Result fsdevMountBcatSaveData(const char *name, u64 application_id);

/// Mounts the specified DeviceSaveData.
Result fsdevMountDeviceSaveData(const char *name, u64 application_id);

/// Mounts the TemporaryStorage for the current process.
/// Only available on [3.0.0+].
Result fsdevMountTemporaryStorage(const char *name);

/// Mounts the specified CacheStorage.
/// Only available on [3.0.0+].
Result fsdevMountCacheStorage(const char *name, u64 application_id, u16 save_data_index);

/// Mounts the specified SystemSaveData.
Result fsdevMountSystemSaveData(const char *name, FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, AccountUid uid);

/// Mounts the specified SystemBcatSaveData.
/// Only available on [4.0.0+].
Result fsdevMountSystemBcatSaveData(const char *name, u64 system_save_data_id);

/// Mounts the input fs with the specified device name. fsdev will handle closing the fs when required, including when fsdevMountDevice() fails.
/// Returns -1 when any errors occur.
/// Input device name string shouldn't exceed 31 characters, and shouldn't have a trailing colon.
int fsdevMountDevice(const char *name, FsFileSystem fs);

/// Unmounts the specified device.
int fsdevUnmountDevice(const char *name);

/// Uses fsFsCommit() with the specified device. This must be used after any savedata-write operations(not just file-write). This should be used after each file-close where file-writing was done.
/// This is not used automatically at device unmount.
Result fsdevCommitDevice(const char *name);

/// Returns the FsFileSystem for the specified device. Returns NULL when the specified device isn't found.
FsFileSystem* fsdevGetDeviceFileSystem(const char *name);

/// Writes the FS-path to outpath (which has buffer size FS_MAX_PATH), for the input path (as used in stdio). The FsFileSystem is also written to device when not NULL.
int fsdevTranslatePath(const char *path, FsFileSystem** device, char *outpath);

/// This calls fsFsSetConcatenationFileAttribute on the filesystem specified by the input path (as used in stdio).
Result fsdevSetConcatenationFileAttribute(const char *path);

// Uses \ref fsFsIsValidSignedSystemPartitionOnSdCard with the specified device.
Result fsdevIsValidSignedSystemPartitionOnSdCard(const char *name, bool *out);

/// This calls fsFsCreateFile on the filesystem specified by the input path (as used in stdio).
Result fsdevCreateFile(const char* path, size_t size, u32 flags);

/// Recursively deletes the directory specified by the input path (as used in stdio).
Result fsdevDeleteDirectoryRecursively(const char *path);

/// Unmounts all devices and cleans up any resources used by the FS driver.
Result fsdevUnmountAll(void);

/// Retrieves the last native result code generated during a failed fsdev operation.
Result fsdevGetLastResult(void);
