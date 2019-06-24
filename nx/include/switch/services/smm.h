/**
 * @file smm.h
 * @brief ServiceManager-IManager (sm:m) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/fs.h"

Result smManagerInitialize(void);
void smManagerExit(void);

Service* smManagerGetServiceSession(void);

Result smManagerRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size);
Result smManagerUnregisterProcess(u64 pid);
