/**
 * @file ldr.h
 * @brief Loader (ldr*) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/ncm_types.h"

typedef struct {
    u8 main_thread_priority;
    u8 default_cpu_id;
    u16 application_type;
    u32 main_thread_stack_size;
    u64 program_id;
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

/// Initialize ldr:shel.
Result ldrShellInitialize(void);

/// Exit ldr:shel.
void ldrShellExit(void);

/// Gets the Service object for the actual ldr:shel service session.
Service* ldrShellGetServiceSession(void);

/// Initialize ldr:dmnt.
Result ldrDmntInitialize(void);

/// Exit ldr:dmnt.
void ldrDmntExit(void);

/// Gets the Service object for the actual ldr:dmnt service session.
Service* ldrDmntGetServiceSession(void);

/// Initialize ldr:pm.
Result ldrPmInitialize(void);

/// Exit ldr:pm.
void ldrPmExit(void);

/// Gets the Service object for the actual ldr:pm service session.
Service* ldrPmGetServiceSession(void);

Result ldrShellSetProgramArguments(u64 program_id, const void *args, size_t args_size);
Result ldrShellFlushArguments(void);

Result ldrDmntSetProgramArguments(u64 program_id, const void *args, size_t args_size);
Result ldrDmntFlushArguments(void);
Result ldrDmntGetProcessModuleInfo(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, s32 *num_out);

Result ldrPmCreateProcess(u64 pin_id, u32 flags, Handle reslimit_h, Handle *out_process_h);
Result ldrPmGetProgramInfo(const NcmProgramLocation *loc, LoaderProgramInfo *out_program_info);
Result ldrPmPinProgram(const NcmProgramLocation *loc, u64 *out_pin_id);
Result ldrPmUnpinProgram(u64 pin_id);
