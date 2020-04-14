/**
 * @file pgl.h
 * @brief PGL service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize pgl.
Result pglInitialize(void);

/// Exit pgl.
void pglExit(void);

/// Gets the Service object for the actual pgl service session.
Service* pglGetServiceSession(void);

Result pglTerminateProcess(u64 pid);
