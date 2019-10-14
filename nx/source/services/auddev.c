#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/auddev.h"
#include "services/applet.h"

static Service g_auddevIAudioDevice;

static Result _auddevGetAudioDeviceService(Service* srv, Service* srv_out, u64 aruid);

NX_GENERATE_SERVICE_GUARD(auddev);

Result _auddevInitialize(void) {
    Result rc=0;
    u64 aruid = 0;
    rc = appletGetAppletResourceUserId(&aruid);

    Service audrenMgrSrv;
    rc = smGetService(&audrenMgrSrv, "audren:u");
    if (R_SUCCEEDED(rc)) {
        rc = _auddevGetAudioDeviceService(&audrenMgrSrv, &g_auddevIAudioDevice, aruid);

        serviceClose(&audrenMgrSrv);
    }

    return rc;
}

void _auddevCleanup(void) {
    serviceClose(&g_auddevIAudioDevice);
}

Service* auddevGetServiceSession(void) {
    return &g_auddevIAudioDevice;
}

static Result _auddevGetAudioDeviceService(Service* srv, Service* srv_out, u64 aruid) {
    return serviceDispatchIn(srv, 2, aruid,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result auddevListAudioDeviceName(AudioDeviceName *DeviceNames, s32 max_names, s32 *total_names) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchOut(&g_auddevIAudioDevice, new_cmd==0 ? 0 : 6, *total_names,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_Out },
        .buffers = { { DeviceNames, max_names*sizeof(AudioDeviceName) } },
    );
}

Result auddevSetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float volume) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchIn(&g_auddevIAudioDevice, new_cmd==0 ? 1 : 7, volume,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_In },
        .buffers = { { DeviceName, sizeof(AudioDeviceName) } },
    );
}

Result auddevGetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float *volume) {
    bool new_cmd = hosversionAtLeast(3,0,0);
    return serviceDispatchOut(&g_auddevIAudioDevice, new_cmd==0 ? 2 : 8, *volume,
        .buffer_attrs = { (new_cmd==0 ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcAutoSelect) | SfBufferAttr_In },
        .buffers = { { DeviceName, sizeof(AudioDeviceName) } },
    );
}
