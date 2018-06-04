/**
 * @file pm.h
 * @brief Process management (pm*) service IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @author mdbell
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result pmdmntInitialize(void);
void pmdmntExit(void);

Result pminfoInitialize(void);
void pminfoExit(void);

Result pmshellInitialize(void);
void pmshellExit(void);

Result pmdmntStartProcess(u64 pid);
Result pmdmntGetTitlePid(u64* pid_out, u64 title_id);
Result pmdmntEnableDebugForTitleId(Handle* handle_out, u64 title_id);
Result pmdmntGetApplicationPid(u64* pid_out);
Result pmdmntEnableDebugForApplication(Handle* handle_out);

Result pminfoGetTitleId(u64* title_id_out, u64 pid);

Result pmshellLaunchProcess(u32 launch_flags, u64 titleID, u64 storageID, u64 *pid);
Result pmshellTerminateProcessByTitleId(u64 titleID);