#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/detect.h"
#include "kernel/event.h"
#include "services/psm.h"
#include "services/sm.h"

static Service g_psmSrv;
static Service g_psmSession;
static Event g_psmStateChangeEvent;
static bool g_psmStateChangeEventInitialized;
static u64 g_refCnt;

static Result _psmOpenSession(Service* out);
static Result _psmBindStateChangeEvent(Event* event_out);

static Result _psmSetChargerTypeChangeEventEnabled(bool flag);
static Result _psmSetPowerSupplyChangeEventEnabled(bool flag);
static Result _psmSetBatteryVoltageStateChangeEventEnabled(bool flag);

Result psmInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_psmSrv))
        return 0;

    g_psmStateChangeEventInitialized = 0;

    rc = smGetService(&g_psmSrv, "psm");

    if (R_SUCCEEDED(rc)) {
        rc = _psmOpenSession(&g_psmSession);
    }

    if (R_FAILED(rc)) psmExit();

    return rc;
}

void psmExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        if (g_psmStateChangeEventInitialized) psmUnbindStateChangeEvent();
        serviceClose(&g_psmSession);
        serviceClose(&g_psmSrv);
    }
}

static Result _psmGetOutU32(u64 cmd_id, u32 *out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = serviceIpcPrepareHeader(&g_psmSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    
    Result rc = serviceIpcDispatch(&g_psmSrv);
    
    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_psmSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->out;
        }
    }
    
    return rc;
}

Result psmGetBatteryChargePercentage(u32 *out) {
    return _psmGetOutU32(0, out);
}

Result psmGetChargerType(ChargerType *out) {
    return _psmGetOutU32(1, out);
}

Result psmGetBatteryVoltageState(u32 *out) {
    return _psmGetOutU32(12, out);
}

static Result _psmOpenSession(Service* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_psmSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(&g_psmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_psmSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreateSubservice(out, &g_psmSrv, &r, 0);
    }

    return rc;
}

Result psmBindStateChangeEvent(bool ChargerType, bool PowerSupply, bool BatteryVoltage) {
    Result rc=0;

    if (g_psmStateChangeEventInitialized) {
        rc = psmUnbindStateChangeEvent();
        if (R_FAILED(rc)) return rc;
    }

    rc = _psmSetChargerTypeChangeEventEnabled(ChargerType);
    if (R_FAILED(rc)) return rc;

    rc = _psmSetPowerSupplyChangeEventEnabled(PowerSupply);
    if (R_FAILED(rc)) return rc;

    rc = _psmSetBatteryVoltageStateChangeEventEnabled(BatteryVoltage);
    if (R_FAILED(rc)) return rc;

    rc = _psmBindStateChangeEvent(&g_psmStateChangeEvent);

    if (R_SUCCEEDED(rc)) g_psmStateChangeEventInitialized = 1;

    return rc;
}

Result psmWaitStateChangeEvent(u64 timeout) {
    Result rc = 0;

    rc = eventWait(&g_psmStateChangeEvent, timeout);
    if (R_SUCCEEDED(rc)) rc = eventClear(&g_psmStateChangeEvent);
    return rc;
}

static Result _psmBindStateChangeEvent(Event *event_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_psmSession, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_psmSession);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_psmSession, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event_out, r.Handles[0], false);
        }
    }

    return rc;
}

Result psmUnbindStateChangeEvent(void) {
    IpcCommand c;
    ipcInitialize(&c);

    if (g_psmStateChangeEventInitialized) {
        g_psmStateChangeEventInitialized = 0;
        eventClose(&g_psmStateChangeEvent);
    }

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_psmSession, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&g_psmSession);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_psmSession, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _psmSetEventEnabled(u64 cmd_id, bool flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_psmSession, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->flag = (flag != 0);

    Result rc = serviceIpcDispatch(&g_psmSession);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_psmSession, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _psmSetChargerTypeChangeEventEnabled(bool flag) {
    return _psmSetEventEnabled(2, flag);
}

static Result _psmSetPowerSupplyChangeEventEnabled(bool flag) {
    return _psmSetEventEnabled(3, flag);
}

static Result _psmSetBatteryVoltageStateChangeEventEnabled(bool flag) {
    return _psmSetEventEnabled(4, flag);
}
