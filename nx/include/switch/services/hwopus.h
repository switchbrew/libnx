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
    bool multistream;
} HwopusDecoder;

/// This structure is the start of opusin for \ref hwopusDecodeInterleaved, with the actual opus packet following this.
/// These fields are big-endian.
typedef struct {
    u32 size;        ///< Size of the packet following this header.
    u32 final_range; ///< Indicates the final range of the codec encoder's entropy coder. This can be left at zero.
} HwopusHeader;

/// Used internally.
typedef struct {
    s32 SampleRate;
    s32 ChannelCount;
    s32 TotalStreamCount;
    s32 StereoStreamCount;
    u8 channel_mapping[256];
} HwopusMultistreamState;

Result hwopusDecoderInitialize(HwopusDecoder* decoder, s32 SampleRate, s32 ChannelCount);
void hwopusDecoderExit(HwopusDecoder* decoder);

/// Only available on 3.0.0+.
/// See libopus multistream docs.
Result hwopusDecoderMultistreamInitialize(HwopusDecoder* decoder, s32 SampleRate, s32 ChannelCount, s32 TotalStreamCount, s32 StereoStreamCount, u8 *channel_mapping);

/// Decodes opus data.
Result hwopusDecodeInterleaved(HwopusDecoder* decoder, s32 *DecodedDataSize, s32 *DecodedSampleCount, const void* opusin, size_t opusin_size, s16 *pcmbuf, size_t pcmbuf_size);

