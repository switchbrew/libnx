/**
 * @file fs_dev.h
 * @brief FS driver.
 */
#pragma once

//NOTE: This is currently not usable.

#define FSDEV_DIRITER_MAGIC 0x66736476 /* "fsdv" */

/*! Open directory struct */
typedef struct
{
  u32               magic;          /*! "fsdv" */
  FsDir             fd;
  ssize_t           index;          /*! Current entry index */
  size_t            size;           /*! Current batch size */
  FsDirectoryEntry entry_data[32]; /*! Temporary storage for reading entries */
} fsdev_dir_t;

/// Initializes the FS driver.
Result fsdevInit(void);

/// Exits the FS driver.
Result fsdevExit(void);

/// Mounts the input fs with the specified device name. fsdev will handle closing the fs when required, including when fsdevMountDevice() fails.
int fsdevMountDevice(const char *name, FsFileSystem fs);

/// Unmounts the specified device.
int fsdevUnmountDevice(const char *name);

