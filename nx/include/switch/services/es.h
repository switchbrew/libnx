/**
 * @file es.h
 * @brief ETicket service IPC wrapper.
 * @author simontime
 * @copyright libnx Authors
 */

#pragma once

#include "../kernel/ipc.h"

#include "../services/sm.h"

Result esInitialize(void);
void esExit(void);

Result esCountCommmonTicket(u32* out);
Result esCountPersonalizedTicket(u32* out);
