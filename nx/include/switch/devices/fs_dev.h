/**
 * @file fs_dev.h
 * @brief FS driver.
 */
#pragma once

//NOTE: This is currently not usable.

/// Initializes the FS driver.
Result fsdevInit(void);

/// Enable/disable copy in fsdev_write. This is disabled by default.
/// If it is disabled, you will be unable to write from the following memory-regions(http://switchbrew.org/index.php?title=SVC#MemoryState):
/// MemoryType_Io
/// MemoryType_Normal
/// MemoryType_*SharedMemory
/// MemoryType_ThreadLocal
/// MemoryType_IpcBuffer3
/// MemoryType_KernelStack (This is not mapped in userland anyway)
/// This same restriction applies with reading into the above memory, a file-read version of fsdevWriteSafe() is not available.
void fsdevWriteSafe(bool enable);

/// Exits the FS driver.
Result fsdevExit(void);

