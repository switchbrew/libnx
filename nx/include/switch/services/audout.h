/**
 * @file audout.h
 * @brief Audio output service.
 * @author hexkyz, TotalJustice
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../audio/audio.h"
#include "../sf/service.h"

#define AUDOUT_MAX_DELAY (1000000000ULL)

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

/// Initialize audout.
Result audoutInitialize(void);

/// Exit audout.
void audoutExit(void);

/// Initialize audout:a. Removed in [11.0.0].
Result audoutaInitialize(void);

/// Exit audout:a.
void audoutaExit(void);

/// Initialize audout:d. Removed in [11.0.0].
Result audoutdInitialize(void);

/// Exit audout:d.
void audoutdExit(void);

/// Gets the Service object for the actual audout service session.
Service* audoutGetServiceSession(void);

/// Gets the Service object for IAudioOut.
Service* audoutGetServiceSession_AudioOut(void);

/// Gets the Service for audout:a.
Service* audoutaGetServiceSession(void);

/// Gets the Service for audout:d.
Service* audoutdGetServiceSession(void);

Result audoutListAudioOuts(char *DeviceNames, s32 count, u32 *DeviceNamesCount);
Result audoutOpenAudioOut(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioOutState *State);
Result audoutGetAudioOutState(AudioOutState *State);
Result audoutStartAudioOut(void);
Result audoutStopAudioOut(void);

/// Submits an \ref AudioOutBuffer for playing.
Result audoutAppendAudioOutBuffer(AudioOutBuffer *Buffer);

Result audoutGetReleasedAudioOutBuffer(AudioOutBuffer **Buffer, u32 *ReleasedBuffersCount);
Result audoutContainsAudioOutBuffer(AudioOutBuffer *Buffer, bool *ContainsBuffer);

/// Only available with [4.0.0+].
Result audoutGetAudioOutBufferCount(u32 *count);
/// Only available with [4.0.0+].
Result audoutGetAudioOutPlayedSampleCount(u64 *count);
/// Only available with [4.0.0+].
Result audoutFlushAudioOutBuffers(bool *flushed);
/// Only available with [6.0.0+].
Result audoutSetAudioOutVolume(float volume);
/// Only available with [6.0.0+].
Result audoutGetAudioOutVolume(float *volume);

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
 * @param timeout Timeout value, use UINT64_MAX to wait until all finished.
 */
Result audoutWaitPlayFinish(AudioOutBuffer **released, u32* released_count, u64 timeout);

/// These return the state associated with the currently active audio output device.
u32 audoutGetSampleRate(void);                      ///< Supported sample rate (48000Hz).
u32 audoutGetChannelCount(void);                    ///< Supported channel count (2 channels).
PcmFormat audoutGetPcmFormat(void);                 ///< Supported PCM format (Int16).
AudioOutState audoutGetDeviceState(void);           ///< Initial device state (stopped).

Result audoutaRequestSuspend(u64 pid, u64 delay); // [4.0.0+]
Result audoutaRequestResume(u64 pid, u64 delay); // [4.0.0+]
Result audoutaGetProcessMasterVolume(u64 pid, float* volume_out);
Result audoutaSetProcessMasterVolume(u64 pid, u64 delay, float volume);
Result audoutaGetProcessRecordVolume(u64 pid, float* volume_out); // [4.0.0+]
Result audoutaSetProcessRecordVolume(u64 pid, u64 delay, float volume); // [4.0.0+]

Result audoutdRequestSuspendForDebug(u64 pid, u64 delay);
Result audoutdRequestResumeForDebug(u64 pid, u64 delay);
