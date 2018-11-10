/**
 * @file spsm.h
 * @brief SPSM service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result spsmInitialize(void);
void spsmExit(void);

Result spsmShutdown(bool reboot);
Result spsmPutErrorState(void);
