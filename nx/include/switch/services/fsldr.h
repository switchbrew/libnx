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
#include "../crypto/sha256.h"
#include "../services/ncm_types.h"

typedef struct {
    u8 signature[0x100];
    u8 hash[SHA256_HASH_SIZE];
    bool is_signed;
    u8 reserved[3];
} FsCodeInfo;

/// Initialize fsp-ldr.
Result fsldrInitialize(void);

/// Exit fsp-ldr.
void fsldrExit(void);

/// Gets the Service object for the actual fsp-ldr service session.
Service* fsldrGetServiceSession(void);

Result fsldrOpenCodeFileSystem(FsCodeInfo* out_code_info, u64 tid, NcmStorageId storage_id, const char *path, FsContentAttributes attr, FsFileSystem* out);
Result fsldrIsArchivedProgram(u64 pid, bool *out);
