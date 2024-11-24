#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/audctl.h"

NX_GENERATE_SERVICE_GUARD(audctl);

static Service g_audctlSrv;

Result _audctlInitialize(void) {
    return smGetService(&g_audctlSrv, "audctl");
}

void _audctlCleanup(void) {
    serviceClose(&g_audctlSrv);
}

Service* audctlGetServiceSession(void) {
    return &g_audctlSrv;
}

Result audctlGetTargetVolume(s32* volume_out, AudioTarget target) {
    const struct {
        u32 target;
    } in = { target };

    struct {
        s32 volume;
    } out;

    Result rc = serviceDispatchInOut(&g_audctlSrv, 0, in, out);

    if (R_SUCCEEDED(rc)) {
        *volume_out = out.volume;
    }
    return rc;
}

Result audctlSetTargetVolume(AudioTarget target, s32 volume) {
    const struct {
        u32 target;
        s32 volume;
    } in = { target, volume };

    return serviceDispatchIn(&g_audctlSrv, 1, in);
}

Result audctlGetTargetVolumeMin(s32* volume_out) {
    struct {
        s32 volume;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 2, out);

    if (R_SUCCEEDED(rc)) {
        *volume_out = out.volume;
    }
    return rc;
}

Result audctlGetTargetVolumeMax(s32* volume_out) {
    struct {
        s32 volume;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 3, out);

    if (R_SUCCEEDED(rc)) {
        *volume_out = out.volume;
    }
    return rc;
}

Result audctlIsTargetMute(bool* mute_out, AudioTarget target) {
    const struct {
        u32 target;
    } in = { target };

    struct {
        u8 mute;
    } out;

    Result rc = serviceDispatchInOut(&g_audctlSrv, 4, in, out);

    if (R_SUCCEEDED(rc)) {
        *mute_out = out.mute;
    }
    return rc;
}

Result audctlSetTargetMute(AudioTarget target, bool mute) {
    const struct {
        u32 mute;
        u32 target;
    } in = { mute, target };

    return serviceDispatchIn(&g_audctlSrv, 5, in);
}

Result audctlIsTargetConnected(bool* connected_out, AudioTarget target) {
    if (hosversionAtLeast(18,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 target;
    } in = { target };

    struct {
        u8 connected;
    } out;

    Result rc = serviceDispatchInOut(&g_audctlSrv, 6, in, out);

    if (R_SUCCEEDED(rc)) {
        *connected_out = out.connected;
    }
    return rc;
}

Result audctlSetDefaultTarget(AudioTarget target, u64 fade_in_ns, u64 fade_out_ns) {
    const struct {
        u32 target;
        u32 padding;
        u64 fade_in_ns;
        u64 fade_out_ns;
    } in = { target, 0, fade_in_ns, fade_out_ns  };

    return serviceDispatchIn(&g_audctlSrv, 7, in);
}

Result audctlGetDefaultTarget(AudioTarget* target_out) {
    struct {
        u32 target;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 8, out);

    if (R_SUCCEEDED(rc)) {
        *target_out = out.target;
    }
    return rc;
}

Result audctlGetAudioOutputMode(AudioOutputMode* mode_out, AudioTarget target) {
    const struct {
        u32 target;
    } in = { target };

    struct {
        u32 mode;
    } out;

    Result rc = serviceDispatchInOut(&g_audctlSrv, 9, in, out);

    if (R_SUCCEEDED(rc)) {
        *mode_out = out.mode;
    }
    return rc;
}

Result audctlSetAudioOutputMode(AudioTarget target, AudioOutputMode mode) {
    const struct {
        u32 target;
        u32 mode;
    } in = { target, mode };

    return serviceDispatchIn(&g_audctlSrv, 10, in);
}

Result audctlSetForceMutePolicy(AudioForceMutePolicy policy) {
    if (hosversionAtLeast(14,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 policy;
    } in = { policy };

    return serviceDispatchIn(&g_audctlSrv, 11, in);
}

Result audctlGetForceMutePolicy(AudioForceMutePolicy* policy_out) {
    if (hosversionAtLeast(14,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 policy;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 12, out);

    if (R_SUCCEEDED(rc)) {
        *policy_out = out.policy;
    }
    return rc;
}

Result audctlGetOutputModeSetting(AudioOutputMode* mode_out, AudioTarget target) {
    const struct {
        u32 target;
    } in = { target };

    struct {
        u32 mode;
    } out;

    Result rc = serviceDispatchInOut(&g_audctlSrv, 13, in, out);

    if (R_SUCCEEDED(rc)) {
        *mode_out = out.mode;
    }
    return rc;
}

Result audctlSetOutputModeSetting(AudioTarget target, AudioOutputMode mode) {
    const struct {
        u32 target;
        u32 mode;
    } in = { target, mode };

    return serviceDispatchIn(&g_audctlSrv, 14, in);
}

Result audctlSetOutputTarget(AudioTarget target) {
    const struct {
        u32 target;
    } in = { target };

    return serviceDispatchIn(&g_audctlSrv, 15, in);
}

Result audctlSetInputTargetForceEnabled(bool enable) {
    const struct {
        bool enable;
    } in = { enable };

    return serviceDispatchIn(&g_audctlSrv, 16, in);
}

Result audctlSetHeadphoneOutputLevelMode(AudioHeadphoneOutputLevelMode mode) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 mode;
    } in = { mode };

    return serviceDispatchIn(&g_audctlSrv, 17, in);
}

Result audctlGetHeadphoneOutputLevelMode(AudioHeadphoneOutputLevelMode* mode_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 mode;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 18, out);

    if (R_SUCCEEDED(rc)) {
        *mode_out = out.mode;
    }
    return rc;
}

Result audctlAcquireAudioVolumeUpdateEventForPlayReport(Event* event_out) {
    if (!hosversionBetween(3,14))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Handle tmp_handle;

    Result rc = serviceDispatch(&g_audctlSrv, 19,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(event_out, tmp_handle, 1);
    }

    return rc;
}

Result audctlAcquireAudioOutputDeviceUpdateEventForPlayReport(Event* event_out) {
    if (!hosversionBetween(3,14))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Handle tmp_handle;

    Result rc = serviceDispatch(&g_audctlSrv, 20,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(event_out, tmp_handle, 1);
    }

    return rc;
}

Result audctlGetAudioOutputTargetForPlayReport(AudioTarget* target_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 target;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 21, out);

    if (R_SUCCEEDED(rc)) {
        *target_out = out.target;
    }
    return rc;
}

Result audctlNotifyHeadphoneVolumeWarningDisplayedEvent(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_audctlSrv, 22);
}

Result audctlSetSystemOutputMasterVolume(float volume) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        float volume;
    } in = { volume };

    return serviceDispatchIn(&g_audctlSrv, 23, in);
}

Result audctlGetSystemOutputMasterVolume(float* volume_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        float volume;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 24, out);

    if (R_SUCCEEDED(rc)) {
        *volume_out = out.volume;
    }
    return rc;
}

Result audctlGetActiveOutputTarget(AudioTarget* target) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 target;
    } out;

    Result rc = serviceDispatchOut(&g_audctlSrv, 32, out);

    if (R_SUCCEEDED(rc)) {
        *target = out.target;
    }
    return rc;
}
