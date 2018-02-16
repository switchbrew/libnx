/**
 * @file audout.h
 * @brief Audio output service.
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

typedef enum {
    AudioOutState_Started = 0,
    AudioOutState_Stopped = 1,
} AudioOutState;

/// Audio output buffer format
typedef struct AudioOutBuffer AudioOutBuffer;

struct AudioOutBuffer
{
    AudioOutBuffer* next;       ///< Next buffer.
    void* buffer;               ///< Sample buffer.
    u64 buffer_size;            ///< Sample buffer size.
    u64 data_size;              ///< Size of data inside the buffer.
    u64 data_offset;            ///< Offset of data inside the buffer.
};

Result audoutInitialize(void);
void audoutExit(void);

Result audoutListAudioOuts(char *DeviceNames, u32 *DeviceNamesCount);
Result audoutOpenAudioOut(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioOutState *State);
Result audoutGetAudioOutState(AudioOutState *State);
Result audoutStartAudioOut(void);
Result audoutStopAudioOut(void);
Result audoutAppendAudioOutBuffer(AudioOutBuffer *Buffer);
Result audoutRegisterBufferEvent(Handle *BufferEvent);
Result audoutGetReleasedAudioOutBuffer(AudioOutBuffer *Buffer, u32 *ReleasedBuffersCount);
Result audoutContainsAudioOutBuffer(AudioOutBuffer *Buffer, bool *ContainsBuffer);

/**
 * @brief Submits an audio sample data buffer for playing.
 * @param source AudioOutBuffer containing the source sample data to be played.
 * @param released AudioOutBuffer to receive the last played buffer.
 */
void audoutPlayBuffer(AudioOutBuffer *source, AudioOutBuffer *released);

/// These return the state associated with the currently active audio output device.
u32 audoutGetSampleRate(void);
u32 audoutGetChannelCount(void);
PcmFormat audoutGetPcmFormat(void);
AudioOutState audoutGetDeviceState(void);
