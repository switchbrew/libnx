/**
 * @file audctl.h
 * @brief Audio Control IPC wrapper.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../audio/audio.h"
#include "../sf/service.h"
#include "../kernel/event.h"

typedef enum {
    AudioTarget_Invalid = 0,
    AudioTarget_Speaker = 1,
    AudioTarget_Headphone = 2,
    AudioTarget_Tv = 3,
    AudioTarget_UsbOutputDevice = 4,
    AudioTarget_Bluetooth = 5,
} AudioTarget;

typedef enum {
    AudioOutputMode_Invalid = 0,
    AudioOutputMode_Pcm1ch = 1,
    AudioOutputMode_Pcm2ch = 2,
    AudioOutputMode_Pcm6ch = 3,
    AudioOutputMode_PcmAuto = 4,
} AudioOutputMode;

typedef enum {
    AudioForceMutePolicy_Disable = 0,
    AudioForceMutePolicy_SpeakerMuteOnHeadphoneUnplugged = 1,
} AudioForceMutePolicy;

typedef enum {
    AudioHeadphoneOutputLevelMode_Normal = 0,
    AudioHeadphoneOutputLevelMode_HighPower = 1,
} AudioHeadphoneOutputLevelMode;

Result audctlInitialize(void);
void audctlExit(void);
Service* audctlGetServiceSession(void);

Result audctlGetTargetVolume(s32* volume_out, AudioTarget target);
Result audctlSetTargetVolume(AudioTarget target, s32 volume);
Result audctlGetTargetVolumeMin(s32* volume_out);
Result audctlGetTargetVolumeMax(s32* volume_out);
Result audctlIsTargetMute(bool* mute_out, AudioTarget target);
Result audctlSetTargetMute(AudioTarget target, bool mute);
Result audctlIsTargetConnected(bool* connected_out, AudioTarget target); ///< [1.0.0-17.0.1]
Result audctlSetDefaultTarget(AudioTarget target, u64 fade_in_ns, u64 fade_out_ns);
Result audctlGetDefaultTarget(AudioTarget* target_out);
Result audctlGetAudioOutputMode(AudioOutputMode* mode_out, AudioTarget target);
Result audctlSetAudioOutputMode(AudioTarget target, AudioOutputMode mode);
Result audctlSetForceMutePolicy(AudioForceMutePolicy policy); ///< [1.0.0-13.2.1]
Result audctlGetForceMutePolicy(AudioForceMutePolicy* policy_out); ///< [1.0.0-13.2.1]
Result audctlGetOutputModeSetting(AudioOutputMode* mode_out, AudioTarget target);
Result audctlSetOutputModeSetting(AudioTarget target, AudioOutputMode mode);
Result audctlSetOutputTarget(AudioTarget target);
Result audctlSetInputTargetForceEnabled(bool enable);
Result audctlSetHeadphoneOutputLevelMode(AudioHeadphoneOutputLevelMode mode); ///< [3.0.0+]
Result audctlGetHeadphoneOutputLevelMode(AudioHeadphoneOutputLevelMode* mode_out); ///< [3.0.0+]
Result audctlAcquireAudioVolumeUpdateEventForPlayReport(Event* event_out); ///< [3.0.0-13.2.1]
Result audctlAcquireAudioOutputDeviceUpdateEventForPlayReport(Event* event_out); ///< [3.0.0-13.2.1]
Result audctlGetAudioOutputTargetForPlayReport(AudioTarget* target_out); ///< [3.0.0+]
Result audctlNotifyHeadphoneVolumeWarningDisplayedEvent(void); ///< [3.0.0+]
Result audctlSetSystemOutputMasterVolume(float volume); ///< [4.0.0+]
Result audctlGetSystemOutputMasterVolume(float* volume_out); ///< [4.0.0+]
Result audctlGetActiveOutputTarget(AudioTarget* target);
