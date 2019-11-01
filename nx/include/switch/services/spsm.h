/**
 * @file spsm.h
 * @brief SPSM service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize spsm.
Result spsmInitialize(void);

/// Exit spsm.
void spsmExit(void);

/// Gets the Service object for the actual spsm service session.
Service* spsmGetServiceSession(void);

Result spsmShutdown(bool reboot);
Result spsmPutErrorState(void);
