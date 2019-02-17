#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/auddev.h"
#include "services/applet.h"
#include "services/sm.h"

static Service g_auddevIAudioDevice;
static u64 g_auddevRefCnt;
static size_t g_auddevIpcBufferSize;

static Result _auddevGetAudioDeviceService(Service* srv, Service* out_srv, u64 aruid);

Result auddevInitialize(void) {
    Result rc=0;

    atomicIncrement64(&g_auddevRefCnt);

    if (serviceIsActive(&g_auddevIAudioDevice))
        return 0;

    u64 aruid = 0;
    rc = appletGetAppletResourceUserId(&aruid);

    Service audrenMgrSrv;
    rc = smGetService(&audrenMgrSrv, "audren:u");
    if (R_SUCCEEDED(rc)) {
        rc = _auddevGetAudioDeviceService(&audrenMgrSrv, &g_auddevIAudioDevice, aruid);

        serviceClose(&audrenMgrSrv);

        if (R_SUCCEEDED(rc)) rc = ipcQueryPointerBufferSize(g_auddevIAudioDevice.handle, &g_auddevIpcBufferSize);
    }

    if (R_FAILED(rc)) auddevExit();

    return rc;
}

void auddevExit(void) {
    if (atomicDecrement64(&g_auddevRefCnt) == 0) {
        serviceClose(&g_auddevIAudioDevice);
    }
}

static Result _auddevGetAudioDeviceService(Service* srv, Service* out_srv, u64 aruid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 aruid;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->aruid = aruid;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(out_srv, srv, &r, 0);
        }
    }

    return rc;
}

Result auddevListAudioDeviceName(AudioDeviceName *DeviceNames, s32 max_names, s32 *total_names) {
    bool new_cmd = hosversionAtLeast(3,0,0);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    if (!new_cmd) ipcAddRecvBuffer(&c, DeviceNames, sizeof(AudioDeviceName) * max_names, BufferType_Normal);
    if (new_cmd) ipcAddRecvSmart(&c, g_auddevIpcBufferSize, DeviceNames, sizeof(AudioDeviceName)  * max_names, 0);

    raw = serviceIpcPrepareHeader(&g_auddevIAudioDevice, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = new_cmd==0 ? 0 : 6;

    Result rc = serviceIpcDispatch(&g_auddevIAudioDevice);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            s32 total_names;
        } *resp;

        serviceIpcParse(&g_auddevIAudioDevice, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_names) *total_names = resp->total_names;
    }

    return rc;
}

Result auddevSetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float volume) {
    bool new_cmd = hosversionAtLeast(3,0,0);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        float volume;
    } *raw;

    if (!new_cmd) ipcAddSendBuffer(&c, DeviceName, sizeof(AudioDeviceName), BufferType_Normal);
    if (new_cmd) ipcAddSendSmart(&c, g_auddevIpcBufferSize, DeviceName, sizeof(AudioDeviceName), 0);

    raw = serviceIpcPrepareHeader(&g_auddevIAudioDevice, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = new_cmd==0 ? 1 : 7;
    raw->volume = volume;

    Result rc = serviceIpcDispatch(&g_auddevIAudioDevice);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_auddevIAudioDevice, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result auddevGetAudioDeviceOutputVolume(const AudioDeviceName *DeviceName, float *volume) {
    bool new_cmd = hosversionAtLeast(3,0,0);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    if (!new_cmd) ipcAddSendBuffer(&c, DeviceName, sizeof(AudioDeviceName), BufferType_Normal);
    if (new_cmd) ipcAddSendSmart(&c, g_auddevIpcBufferSize, DeviceName, sizeof(AudioDeviceName), 0);

    raw = serviceIpcPrepareHeader(&g_auddevIAudioDevice, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = new_cmd==0 ? 2 : 8;

    Result rc = serviceIpcDispatch(&g_auddevIAudioDevice);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            float volume;
        } *resp;

        serviceIpcParse(&g_auddevIAudioDevice, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && volume) *volume = resp->volume;
    }

    return rc;
}
