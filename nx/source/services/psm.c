#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/psm.h"

static Service g_psmSrv;

static Result _psmOpenSession(Service* srv_out);
static Result _psmBindStateChangeEvent(PsmSession* s, Event* event_out);

static Result _psmSetChargerTypeChangeEventEnabled(PsmSession* s, bool flag);
static Result _psmSetPowerSupplyChangeEventEnabled(PsmSession* s, bool flag);
static Result _psmSetBatteryVoltageStateChangeEventEnabled(PsmSession* s, bool flag);

NX_GENERATE_SERVICE_GUARD(psm);

Result _psmInitialize(void) {
    return smGetService(&g_psmSrv, "psm");
}

void _psmCleanup(void) {
    serviceClose(&g_psmSrv);
}

Service* psmGetServiceSession(void) {
    return &g_psmSrv;
}

static Result _psmCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _psmCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, autoclear);

    return rc;
}

static Result _psmCmdInU8NoOut(Service* srv, u8 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _psmCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    return _psmCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _psmCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

Result psmGetBatteryChargePercentage(u32 *out) {
    return _psmCmdNoInOutU32(&g_psmSrv, out, 0);
}

Result psmGetChargerType(ChargerType *out) {
    return _psmCmdNoInOutU32(&g_psmSrv, out, 1);
}

Result psmGetBatteryVoltageState(PsmBatteryVoltageState *out) {
    u32 state;
    Result rc = _psmCmdNoInOutU32(&g_psmSrv, &state, 12);
    if (R_SUCCEEDED(rc) && out) *out = state;
    return rc;
}

static Result _psmOpenSession(Service* srv_out) {
    return serviceDispatch(&g_psmSrv, 7,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
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
    return _psmCmdGetEvent(&s->s, event_out, false, 0);
}

Result psmUnbindStateChangeEvent(PsmSession* s) {
    Result rc=0;
    if (serviceIsActive(&s->s)) rc = _psmCmdNoIO(&s->s, 1);

    eventClose(&s->StateChangeEvent);
    serviceClose(&s->s);

    return rc;
}

static Result _psmSetEventEnabled(PsmSession* s, bool flag, u32 cmd_id) {
    return _psmCmdInBoolNoOut(&s->s, flag, cmd_id);
}

static Result _psmSetChargerTypeChangeEventEnabled(PsmSession* s, bool flag) {
    return _psmSetEventEnabled(s, flag, 2);
}

static Result _psmSetPowerSupplyChangeEventEnabled(PsmSession* s, bool flag) {
    return _psmSetEventEnabled(s, flag, 3);
}

static Result _psmSetBatteryVoltageStateChangeEventEnabled(PsmSession* s, bool flag) {
    return _psmSetEventEnabled(s, flag, 4);
}
