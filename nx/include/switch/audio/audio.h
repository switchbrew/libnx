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
    PcmFormat_Int8 = 1,
    PcmFormat_Int16 = 2,
    PcmFormat_Int24 = 3,
    PcmFormat_Int32 = 4,
    PcmFormat_Float = 5,
    PcmFormat_Adpcm = 6,
} PcmFormat;

typedef struct {
    char name[0x100];
} AudioDeviceName;
