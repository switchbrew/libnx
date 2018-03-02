/**
 * @file audio.h
 * @brief Global audio service.
 * @author hexkyz
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"

typedef enum {
    PcmFormat_Invalid = 0,
    PcmFormat_INT8 = 1,
    PcmFormat_INT16 = 2,
    PcmFormat_INT24 = 3,
    PcmFormat_INT32 = 4,
    PcmFormat_FLOAT = 5,
    PcmFormat_ADPCM = 6,
} PcmFormat;
