/**
 * @file idlesys.h
 * @author MasaGratoR
 * @copyright libnx Authors
 */

#pragma once

#include "../types.h"

/// Initialize idle:sys.
Result idlesysInitialize(void);

/// Exit idle:sys.
void idlesysExit(void);

/// Gets the Service for idle:sys.
Service* idlesysGetServiceSession(void);

/// Resets sleep counter.
Result idlesysReportUserIsActive(void);
