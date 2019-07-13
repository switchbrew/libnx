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
Service* ldrRoGetServiceSession(void);

Result ro1Initialize(void);
void ro1Exit(void);
Service* ro1GetServiceSession(void);

Result roDmntInitialize(void);
void roDmntExit(void);
Service* roDmntGetServiceSession(void);

Result ldrRoLoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
Result ldrRoUnloadNro(u64 nro_address);
Result ldrRoLoadNrr(u64 nrr_address, u64 nrr_size);
Result ldrRoUnloadNrr(u64 nrr_address);
Result ldrRoLoadNrrEx(u64 nrr_address, u64 nrr_size);

Result ro1LoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
Result ro1UnloadNro(u64 nro_address);
Result ro1LoadNrr(u64 nrr_address, u64 nrr_size);
Result ro1UnloadNrr(u64 nrr_address);
Result ro1LoadNrrEx(u64 nrr_address, u64 nrr_size);

Result roDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, u32 *num_out);
