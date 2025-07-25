/**
 * @file aud.h
 * @brief Only available on [11.0.0+].
 * @note Only one session may be open at once.
 * @author TotalJustice
 * @copyright libnx Authors
 */

#pragma once

#include "../types.h"
#include "../sf/service.h"

#define AUD_MAX_DELAY (1000000000ULL)

/// Initialize aud:a. Only available on [11.0.0+].
Result audaInitialize(void);

/// Exit aud:a.
void audaExit(void);

/// Initialize aud:d. Only available on [11.0.0+].
Result auddInitialize(void);

/// Exit aud:d.
void auddExit(void);

/// Gets the Service for aud:a.
Service* audaGetServiceSession(void);

/// Gets the Service for aud:d.
Service* auddGetServiceSession(void);

Result audaRequestSuspendAudio(u64 pid, u64 delay);
Result audaRequestResumeAudio(u64 pid, u64 delay);
Result audaGetAudioOutputProcessMasterVolume(u64 pid, float* volume_out);

Result audaSetAudioOutputProcessMasterVolume(u64 pid, u64 delay, float volume);
Result audaGetAudioInputProcessMasterVolume(u64 pid, float* volume_out);

// Sets both Output and Input volume
Result audaSetAudioInputProcessMasterVolume(u64 pid, u64 delay, float volume);
Result audaGetAudioOutputProcessRecordVolume(u64 pid, float* volume_out);
Result audaSetAudioOutputProcessRecordVolume(u64 pid, u64 delay, float volume);

Result auddRequestSuspendAudioForDebug(u64 pid, u64 delay);
Result auddRequestResumeAudioForDebug(u64 pid, u64 delay);
