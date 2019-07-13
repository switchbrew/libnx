/**
 * @file auddev.h
 * @brief Audio device.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../audio/audio.h"
#include "../services/sm.h"

Result auddevInitialize(void);
void auddevExit(void);
Service* auddevgetServiceSession(void);

Result auddevListAudioDeviceName(AudioDeviceName *DeviceNames, s32 max_names, s32 *total_names);
Result auddevSetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float volume);
Result auddevGetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float *volume);

