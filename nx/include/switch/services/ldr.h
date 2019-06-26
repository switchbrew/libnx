/**
 * @file ldr.h
 * @brief Loader (ldr*) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/fs.h"

typedef struct {
        u8 main_thread_priority;
        u8 default_cpu_id;
        u16 application_type;
        u32 main_thread_stack_size;
        u64 title_id;
        u32 acid_sac_size;
        u32 aci0_sac_size;
        u32 acid_fac_size;
        u32 aci0_fah_size;
        u8 ac_buffer[0x3E0];
} LoaderProgramInfo;

typedef struct {
    u8 build_id[0x20];
    u64 base_address;
    u64 size;
} LoaderModuleInfo;

Result ldrShellInitialize(void);
void ldrShellExit(void);

Service* ldrShellGetServiceSession(void);

Result ldrDmntInitialize(void);
void ldrDmntExit(void);

Service* ldrDmntGetServiceSession(void);

Result ldrPmInitialize(void);
void ldrPmExit(void);

Service* ldrPmGetServiceSession(void);

Result ldrShellAddTitleToLaunchQueue(u64 tid, const void *args, size_t args_size);
Result ldrShellClearLaunchQueue(void);

Result ldrDmntAddTitleToLaunchQueue(u64 tid, const void *args, size_t args_size);
Result ldrDmntClearLaunchQueue(void);
Result ldrDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, u32 *num_out);

Result ldrPmCreateProcess(u64 flags, u64 launch_index, Handle reslimit_h, Handle *out_process_h);
Result ldrPmGetProgramInfo(u64 title_id, FsStorageId storage_id, LoaderProgramInfo *out_program_info);
Result ldrPmRegisterTitle(u64 title_id, FsStorageId storage_id, u64 *out_index);
Result ldrPmUnregisterTitle(u64 launch_index);
