/**
 * @file fsldr.h
 * @brief FilesystemProxy-ForLoader (fsp-ldr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/fs.h"

/// Initialize fsp-ldr.
Result fsldrInitialize(void);

/// Exit fsp-ldr.
void fsldrExit(void);

/// Gets the Service object for the actual fsp-ldr service session.
Service* fsldrGetServiceSession(void);

Result fsldrOpenCodeFileSystem(u64 tid, const char *path, FsFileSystem* out);
Result fsldrIsArchivedProgram(u64 pid, bool *out);
Result fsldrSetCurrentProcess(void);
