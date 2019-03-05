#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/lbl.h"
#include "services/sm.h"

static Service g_lblSrv;
static u64 g_refCnt;

Result lblInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_lblSrv))
        return 0;

    rc = smGetService(&g_lblSrv, "lbl");

    if (R_FAILED(rc)) lblExit();

    return rc;
}

void lblExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_lblSrv);
    }
}

static Result _lblCmdNoIO(u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_lblSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_lblSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_lblSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _lblSwitchBacklight(u64 cmd_id, u64 fade_time) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 fade_time;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_lblSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->fade_time = fade_time;

    Result rc = serviceIpcDispatch(&g_lblSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_lblSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result lblSwitchBacklightOn(u64 fade_time) {
    return _lblSwitchBacklight(6, fade_time);
}

Result lblSwitchBacklightOff(u64 fade_time) {
    return _lblSwitchBacklight(7, fade_time);
}

Result lblSetCurrentBrightnessSetting(float brightness) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        float value;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_lblSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->value = brightness;

    Result rc = serviceIpcDispatch(&g_lblSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_lblSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result lblGetCurrentBrightnessSetting(float *out_value) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_lblSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(&g_lblSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            float brightness;
        } *resp;

        serviceIpcParse(&g_lblSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc))
            *out_value = resp->brightness;
    }

    return rc;
}

Result lblEnableAutoBrightnessControl(void) {
    return _lblCmdNoIO(12);
}

Result lblDisableAutoBrightnessControl(void) {
    return _lblCmdNoIO(13);    
}

Result lblIsAutoBrightnessControlEnabled(bool *out_value){
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_lblSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;

    Result rc = serviceIpcDispatch(&g_lblSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 isEnabled;
        } *resp;

        serviceIpcParse(&g_lblSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc))
            *out_value = resp->isEnabled != 0;
    }

    return rc;
}
