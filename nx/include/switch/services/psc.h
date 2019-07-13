/**
 * @file psc.h
 * @brief PSC service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/sm.h"

typedef enum {
    PscPmState_Awake = 0,               ///< Everything is awake.
    PscPmState_ReadyAwaken = 1,         ///< Preparing to transition to awake.
    PscPmState_ReadySleep = 2,          ///< Preparing to transition to sleep.
    PscPmState_ReadySleepCritical = 3,  ///< Critical services are ready to sleep.
    PscPmState_ReadyAwakenCritical = 4, ///< Critical services are ready to wake up.
    PscPmState_ReadyShutdown = 5,       ///< Preparing to transition to shutdown.
} PscPmState;

typedef struct {
    Event event;
    Service srv;
    u16 module_id;
} PscPmModule;

Result pscInitialize(void);
void pscExit(void);
Service* pscGetServiceSession(void);

Result pscGetPmModule(PscPmModule *out, u16 module_id, const u16 *dependencies, size_t dependency_count, bool autoclear);

Result pscPmModuleGetRequest(PscPmModule *module, PscPmState *out_state, u32 *out_flags);
Result pscPmModuleAcknowledge(PscPmModule *module, PscPmState state);
Result pscPmModuleFinalize(PscPmModule *module);
