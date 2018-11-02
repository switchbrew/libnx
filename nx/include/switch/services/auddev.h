/**
 * @file auddev.h
 * @brief Audio device.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../audio/audio.h"

Result auddevInitialize(void);
void auddevExit(void);

Result auddevListAudioDeviceName(AudioDeviceName *DeviceNames, s32 max_names, s32 *total_names);
Result auddevSetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float volume);
Result auddevGetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float *volume);

