/**
 * @file audin.h
 * @brief Audio input service.
 * @author hexkyz
 * @copyright libnx Authors
 */
#pragma once

#include "../audio/audio.h"

typedef enum {
    AudioInState_Started = 0,
    AudioInState_Stopped = 1,
} AudioInState;

/// Audio input buffer format
typedef struct AudioInBuffer AudioInBuffer;

struct AudioInBuffer
{
    AudioInBuffer* next;        ///< Next buffer. (Unused)
    void* buffer;               ///< Sample buffer (aligned to 0x1000 bytes).
    u64 buffer_size;            ///< Sample buffer size (aligned to 0x1000 bytes).
    u64 data_size;              ///< Size of data inside the buffer.
    u64 data_offset;            ///< Offset of data inside the buffer. (Unused?)
};

Result audinInitialize(void);
void audinExit(void);

Result audinListAudioIns(char *DeviceNames, u32 *DeviceNamesCount);
Result audinOpenAudioIn(const char *DeviceNameIn, char *DeviceNameOut, u32 SampleRateIn, u32 ChannelCountIn, u32 *SampleRateOut, u32 *ChannelCountOut, PcmFormat *Format, AudioInState *State);
Result audinGetAudioInState(AudioInState *State);
Result audinStartAudioIn(void);
Result audinStopAudioIn(void);

/// Submits an \ref AudioInBuffer for capturing.
Result audinAppendAudioInBuffer(AudioInBuffer *Buffer);

Result audinGetReleasedAudioInBuffer(AudioInBuffer **Buffer, u32 *ReleasedBuffersCount);
Result audinContainsAudioInBuffer(AudioInBuffer *Buffer, bool *ContainsBuffer);

/**
 * @brief Submits an audio sample data buffer for capturing and waits for it to finish capturing.
 * @brief Uses \ref audinAppendAudioInBuffer and \ref audinWaitCaptureFinish internally.
 * @param source AudioInBuffer containing the buffer to hold the captured sample data.
 * @param released AudioInBuffer to receive the captured buffer after being released.
 */
Result audinCaptureBuffer(AudioInBuffer *source, AudioInBuffer **released);

/**
 * @brief Waits for audio capture to finish.
 * @param released AudioInBuffer to receive the first captured buffer after being released.
 * @param released_count Pointer to receive the number of captured buffers.
 * @param timeout Timeout value, use U64_MAX to wait until all finished.
 */
Result audinWaitCaptureFinish(AudioInBuffer **released, u32* released_count, u64 timeout);

/// These return the state associated with the currently active audio input device.
u32 audinGetSampleRate(void);                      ///< Supported sample rate (48000Hz).
u32 audinGetChannelCount(void);                    ///< Supported channel count (2 channels).
PcmFormat audinGetPcmFormat(void);                 ///< Supported PCM format (Int16).
AudioInState audinGetDeviceState(void);            ///< Initial device state (stopped).
