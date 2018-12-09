/**
 * @file ro.h
 * @brief Relocatable Objects (ro) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/ldr.h"

Result ldrRoInitialize(void);
void ldrRoExit(void);

Result roDmntInitialize(void);
void roDmntExit(void);

Result ldrRoLoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
Result ldrRoUnloadNro(u64 nro_address);
Result ldrRoLoadNrr(u64 nrr_address, u64 nrr_size);
Result ldrRoUnloadNrr(u64 nrr_address);

Result roDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t out_size, u32 *num_out);
