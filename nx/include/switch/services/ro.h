/**
 * @file ro.h
 * @brief Relocatable Objects (ro) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/ldr.h"

/// Initialize ldr:ro.
Result ldrRoInitialize(void);

/// Exit ldr:ro.
void ldrRoExit(void);

/// Gets the Service object for the actual ldr:ro service session.
Service* ldrRoGetServiceSession(void);

/// Initialize ro:1. Only available with [7.0.0+].
Result ro1Initialize(void);

/// Exit ro:1.
void ro1Exit(void);

/// Gets the Service object for the actual ro:1 service session.
Service* ro1GetServiceSession(void);

/// Initialize ro:dmnt. Only available with [3.0.0+].
Result roDmntInitialize(void);

/// Exit ro:dmnt.
void roDmntExit(void);

/// Gets the Service object for the actual ro:dmnt service session.
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

Result roDmntGetProcessModuleInfo(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, s32 *num_out);
