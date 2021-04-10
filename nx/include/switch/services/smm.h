/**
 * @file smm.h
 * @brief ServiceManager-IManager (sm:m) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../sf/tipc.h"

/// Initialize sm:m.
Result smManagerInitialize(void);

/// Exit sm:m.
void smManagerExit(void);

Result smManagerRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size);
Result smManagerUnregisterProcess(u64 pid);

/// Initialize sm:m exclusively for tipc (requires <12.0.0 and non-Atmosphere).
Result smManagerCmifInitialize(void);

/// Exit sm:m exclusively for tipc (requires <12.0.0 and non-Atmosphere).
void smManagerCmifExit(void);

/// Gets the Service object for the actual sm:m service session (requires <12.0.0 and non-Atmosphere).
Service* smManagerCmifGetServiceSession(void);

Result smManagerCmifRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size);
Result smManagerCmifUnregisterProcess(u64 pid);

/// Initialize sm:m exclusively for tipc (requires 12.0.0+ or Atmosphere).
Result smManagerTipcInitialize(void);

/// Exit sm:m exclusively for tipc (requires 12.0.0+ or Atmosphere).
void smManagerTipcExit(void);

/// Gets the TipcService object for the actual sm:m service session (requires 12.0.0+ or Atmosphere).
TipcService* smManagerTipcGetServiceSession(void);

Result smManagerTipcRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size);
Result smManagerTipcUnregisterProcess(u64 pid);
