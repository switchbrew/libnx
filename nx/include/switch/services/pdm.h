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
    u8 unk_x8[0x10];                  ///< Unknown
} PdmApplicationPlayStatistics;

