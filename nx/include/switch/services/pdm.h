/**
 * @file pdm.h
 * @brief PDM (pdm:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"

/// ApplicationPlayStatistics
typedef struct {
    u64 titleID;                      ///< titleID
    u64 totalPlayTime;                ///< Total play-time in nanoseconds.
    u64 totalLaunches;                ///< Total times the application title was launched.
} PdmApplicationPlayStatistics;

