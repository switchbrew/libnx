/**
 * @file audout.h
 * @brief Audio output service.
 * @author hexkyz
 * @copyright libnx Authors
 */
#pragma once

#include "../audio/audio.h"

typedef enum {
    AudioOutState_Started = 0,
    AudioOutState_Stopped = 1,
} AudioOutState;

/// Audio output buffer format
typedef struct AudioOutBuffer AudioOutBuffer;

struct AudioOutBuffer
{
    AudioOutBuffer* next;       ///< Next buffer. (Unused)
    void* buffer;               ///< Sample buffer (aligned to 0x1000 bytes).
    u64 buffer_size;            ///< Sample buffer size (aligned to 0x1000 bytes).
    u64 data_size;              ///< Size of data inside the buffer.
    u64 data_offset;            ///< Offset of data inside the buffer. (Unused?)
};

Result audoutInitialize(void);
void audoutExit(void);

Result audoutListAudioOuts(char *DeviceNames, u32 *DeviceNamesCount);
Result audoutOpenAudioOut(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioOutState *State);
Result audoutGetAudioOutState(AudioOutState *State);
Result audoutStartAudioOut(void);
Result audoutStopAudioOut(void);

/// Submits an \ref AudioOutBuffer for playing.
Result audoutAppendAudioOutBuffer(AudioOutBuffer *Buffer);

Result audoutGetReleasedAudioOutBuffer(AudioOutBuffer **Buffer, u32 *ReleasedBuffersCount);
Result audoutContainsAudioOutBuffer(AudioOutBuffer *Buffer, bool *ContainsBuffer);

/**
 * @brief Submits an audio sample data buffer for playing and waits for it to finish playing.
 * @brief Uses \ref audoutAppendAudioOutBuffer and \ref audoutWaitPlayFinish internally.
 * @param source AudioOutBuffer containing the source sample data to be played.
 * @param released AudioOutBuffer to receive the played buffer after being released.
 */
Result audoutPlayBuffer(AudioOutBuffer *source, AudioOutBuffer **released);

/**
 * @brief Waits for audio playback to finish.
 * @param released AudioOutBuffer to receive the first played buffer after being released.
 * @param released_count Pointer to receive the number of played buffers.
 * @param timeout Timeout value, use U64_MAX to wait until all finished.
 */
Result audoutWaitPlayFinish(AudioOutBuffer **released, u32* released_count, u64 timeout);

/// These return the state associated with the currently active audio output device.
u32 audoutGetSampleRate(void);                      ///< Supported sample rate (48000Hz).
u32 audoutGetChannelCount(void);                    ///< Supported channel count (2 channels).
PcmFormat audoutGetPcmFormat(void);                 ///< Supported PCM format (Int16).
AudioOutState audoutGetDeviceState(void);           ///< Initial device state (stopped).
