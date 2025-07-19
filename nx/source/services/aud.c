#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/aud.h"
#include "runtime/hosversion.h"

static Service g_audaSrv;
static Service g_auddSrv;

NX_GENERATE_SERVICE_GUARD(auda);
NX_GENERATE_SERVICE_GUARD(audd);

Result _audaInitialize(void) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_audaSrv, "aud:a");
}

void _audaCleanup(void) {
    serviceClose(&g_audaSrv);
}

Result _auddInitialize(void) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_auddSrv, "aud:d");
}

void _auddCleanup(void) {
    serviceClose(&g_auddSrv);
}

Service* audaGetServiceSession(void) {
    return &g_audaSrv;
}

Service* auddGetServiceSession(void) {
    return &g_auddSrv;
}

Result audaRequestSuspendAudio(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 delay;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audaSrv, 2, in);
}

Result audaRequestResumeAudio(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 delay;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audaSrv, 3, in);
}

Result audaGetAudioOutputProcessMasterVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audaSrv, 4, pid, *volume_out);
}

Result audaSetAudioOutputProcessMasterVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 delay;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audaSrv, 5, in);
}

Result audaGetAudioInputProcessMasterVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audaSrv, 6, pid, *volume_out);
}

Result audaSetAudioInputProcessMasterVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 delay;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audaSrv, 7, in);
}

Result audaGetAudioOutputProcessRecordVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audaSrv, 8, pid, *volume_out);
}

Result audaSetAudioOutputProcessRecordVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 delay;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audaSrv, 9, in);
}

Result auddRequestSuspendAudioForDebug(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 delay;
    } in = { pid, delay };

    return serviceDispatchIn(&g_auddSrv, 0, in);
}

Result auddRequestResumeAudioForDebug(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 delay;
    } in = { pid, delay };

    return serviceDispatchIn(&g_auddSrv, 1, in);
}
