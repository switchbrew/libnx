/**
 * @file nifm_la.h
 * @brief Wrapper for using the nifm LibraryApplet (the launched applet varies).
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/nifm.h"

/**
 * @brief Uses \ref nifmGetResult, then on failure launches the applet.
 * @param r \ref NifmRequest
 */
Result nifmLaHandleNetworkRequestResult(NifmRequest* r);

