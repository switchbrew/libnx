#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "services/psm.h"
#include "services/sm.h"

static Service g_psmSrv;
static u64 g_refCnt;

static Result _psmOpenSession(Service* out);
static Result _psmBindStateChangeEvent(PsmSession* s, Event* event_out);

static Result _psmSetChargerTypeChangeEventEnabled(PsmSession* s, bool flag);
static Result _psmSetPowerSupplyChangeEventEnabled(PsmSession* s, bool flag);
static Result _psmSetBatteryVoltageStateChangeEventEnabled(PsmSession* s, bool flag);

Result psmInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_psmSrv))
        return 0;

    rc = smGetService(&g_psmSrv, "psm");

    if (R_FAILED(rc)) psmExit();

    return rc;
}

void psmExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
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

Result psmGetBatteryVoltageState(PsmBatteryVoltageState *out) {
    u32 state;
    Result rc = _psmGetOutU32(12, &state);
    if (R_SUCCEEDED(rc)) {
        *out = (PsmBatteryVoltageState)state;
    }
    return rc;
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

Result psmBindStateChangeEvent(PsmSession* s, bool ChargerType, bool PowerSupply, bool BatteryVoltage) {
    Result rc=0;

    rc = _psmOpenSession(&s->s);
    if (R_FAILED(rc)) return rc;

    rc = _psmSetChargerTypeChangeEventEnabled(s, ChargerType);
    if (R_FAILED(rc)) return rc;

    rc = _psmSetPowerSupplyChangeEventEnabled(s, PowerSupply);
    if (R_FAILED(rc)) return rc;

    rc = _psmSetBatteryVoltageStateChangeEventEnabled(s, BatteryVoltage);
    if (R_FAILED(rc)) return rc;

    rc = _psmBindStateChangeEvent(s, &s->StateChangeEvent);
    if (R_FAILED(rc)) serviceClose(&s->s);

    return rc;
}

Result psmWaitStateChangeEvent(PsmSession* s, u64 timeout) {
    Result rc = 0;

    rc = eventWait(&s->StateChangeEvent, timeout);
    if (R_SUCCEEDED(rc)) rc = eventClear(&s->StateChangeEvent);
    return rc;
}

static Result _psmBindStateChangeEvent(PsmSession* s, Event *event_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&s->s);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event_out, r.Handles[0], false);
        }
    }

    return rc;
}

Result psmUnbindStateChangeEvent(PsmSession* s) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&s->s);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    eventClose(&s->StateChangeEvent);
    serviceClose(&s->s);

    return rc;
}

static Result _psmSetEventEnabled(PsmSession* s, u64 cmd_id, bool flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->flag = (flag != 0);

    Result rc = serviceIpcDispatch(&s->s);

    if(R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _psmSetChargerTypeChangeEventEnabled(PsmSession* s, bool flag) {
    return _psmSetEventEnabled(s, 2, flag);
}

static Result _psmSetPowerSupplyChangeEventEnabled(PsmSession* s, bool flag) {
    return _psmSetEventEnabled(s, 3, flag);
}

static Result _psmSetBatteryVoltageStateChangeEventEnabled(PsmSession* s, bool flag) {
    return _psmSetEventEnabled(s, 4, flag);
}
