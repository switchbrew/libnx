/**
 * @file fspr.h
 * @brief FilesystemProxy-ProgramRegistry (fsp-pr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/fs.h"

Result fsprInitialize(void);
void fsprExit(void);

Result fsprRegisterProgram(u64 pid, u64 titleID, FsStorageId storageID, const void *fs_access_header, size_t fah_size, const void *fs_access_control, size_t fac_size);
Result fsprUnregisterProgram(u64 pid);
Result fsprSetCurrentProcess(void);
Result fsprSetEnabledProgramVerification(bool enabled);
