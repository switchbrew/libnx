/**
 * @file fspr.h
 * @brief FilesystemProxy-ProgramRegistry (fsp-pr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/ncm_types.h"

/// Initialize fsp-pr.
Result fsprInitialize(void);

/// Exit fsp-pr.
void fsprExit(void);

/// Gets the Service object for the actual fsp-pr service session.
Service* fsprGetServiceSession(void);

Result fsprRegisterProgram(u64 pid, u64 tid, NcmStorageId sid, const void *fs_access_header, size_t fah_size, const void *fs_access_control, size_t fac_size);
Result fsprUnregisterProgram(u64 pid);
Result fsprSetCurrentProcess(void);
Result fsprSetEnabledProgramVerification(bool enabled);
