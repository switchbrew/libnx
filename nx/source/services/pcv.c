#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/pcv.h"
#include "services/sm.h"

static Service g_pcvSrv;
static u64 g_refCnt;

Result pcvInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_pcvSrv))
        return 0;

    rc = smGetService(&g_pcvSrv, "pcv");

    if (R_FAILED(rc)) pcvExit();

    return rc;
}

void pcvExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_pcvSrv);
    }
}

Result pcvSetClockRate(PcvModule module, u32 hz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 module;
        u32 hz;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->module = module;
    raw->hz = hz;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result pcvGetClockRate(PcvModule module, u32 *out_hz) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 module;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->module = module;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 hz;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out_hz = resp->hz;
        }
    }

    return rc;
}

Result pcvSetVoltageEnabled(bool state, u32 voltage) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 state;
        u32 voltage;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->state = (u8)state;
    raw->voltage = voltage;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result pcvGetVoltageEnabled(bool *isEnabled, u32 voltage) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 voltage;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->voltage = voltage;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 isEnabled;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if(R_SUCCEEDED(rc) && isEnabled) {
            *isEnabled = (bool)resp->isEnabled;
        }
    }

    return rc;
}
