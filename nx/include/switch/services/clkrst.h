/**
 * @file clkrst.h
 * @brief Clkrst service IPC wrapper.
 * @author p-sam
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/pcv.h"

typedef struct {
    Service  s;
} ClkrstSession;

Result clkrstInitialize(void);
void clkrstExit(void);
Result clkrstOpenSession(ClkrstSession* session_out, PcvModule module);
void clkrstCloseSession(ClkrstSession* session);
Result clkrstSetClockRate(ClkrstSession* session, u32 hz);
Result clkrstGetClockRate(ClkrstSession* session, u32 *out_hz);
