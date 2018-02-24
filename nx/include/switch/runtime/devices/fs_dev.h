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
  FsDirectoryEntry entry_data[32]; ///< Temporary storage for reading entries
} fsdev_dir_t;

/// Initializes the FS driver. Automatically initializes the sdmc device if accessible. If called again, sdmc mounting will be attempted again if it's not mounted.
Result fsdevInit(void);

/// Exits the FS driver. Any devices still mounted are unmounted.
Result fsdevExit(void);

/// Mounts the input fs with the specified device name. fsdev will handle closing the fs when required, including when fsdevMountDevice() fails.
/// Returns -1 when any errors occur.
int fsdevMountDevice(const char *name, FsFileSystem fs);

/// Unmounts the specified device.
int fsdevUnmountDevice(const char *name);

/// Uses fsFsCommit() with the specified device. This must be used after any savedata-write operations(not just file-write).
/// This is not used automatically at device unmount.
Result fsdevCommitDevice(const char *name);

/// Returns the FsFileSystem for the default device (SD card), if mounted. Used internally by romfs_dev.
FsFileSystem* fsdevGetDefaultFileSystem(void);
