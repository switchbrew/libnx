/**
 * @file hwopus.h
 * @brief Hardware Opus audio service.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../services/sm.h"
#include "../kernel/tmem.h"

typedef struct {
    Service s;
    TransferMemory tmem;
} HwopusDecoder;

Result hwopusDecoderInitialize(HwopusDecoder* decoder, s32 SampleRate, s32 ChannelCount);
void hwopusDecoderExit(HwopusDecoder* decoder);

Result hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedSampleCount, u32 *DecodedDataSize, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size);

