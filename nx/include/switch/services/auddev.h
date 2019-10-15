/**
 * @file auddev.h
 * @brief IAudioDevice IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../audio/audio.h"
#include "../sf/service.h"

/// Initialize IAudioDevice.
Result auddevInitialize(void);

/// Exit IAudioDevice.
void auddevExit(void);

/// Gets the Service object for IAudioDevice.
Service* auddevGetServiceSession(void);

Result auddevListAudioDeviceName(AudioDeviceName *DeviceNames, s32 max_names, s32 *total_names);
Result auddevSetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float volume);
Result auddevGetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float *volume);

