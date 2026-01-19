#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
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

static Result _psmCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 outval = 0;
    Result rc = serviceDispatchOut(srv, cmd_id, outval);
    if (R_SUCCEEDED(rc)) {
        if (out) *out = outval & 1;
    }
    return rc;
}

static Result _psmCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _psmCmdNoInOutDouble(Service* srv, double *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

Result psmGetBatteryChargePercentage(u32 *out) {
    return _psmCmdNoInOutU32(&g_psmSrv, out, 0);
}

Result psmGetChargerType(PsmChargerType *out) {
    return _psmCmdNoInOutU32(&g_psmSrv, out, 1);
}

Result psmEnableBatteryCharging(void) {
    return _psmCmdNoIO(&g_psmSrv, 2);
}

Result psmDisableBatteryCharging(void) {
    return _psmCmdNoIO(&g_psmSrv, 3);
}

Result psmIsBatteryChargingEnabled(bool *out) {
    return _psmCmdNoInOutBool(&g_psmSrv, out, 4);
}

Result psmAcquireControllerPowerSupply(void) {
    return _psmCmdNoIO(&g_psmSrv, 5);
}

Result psmReleaseControllerPowerSupply(void) {
    return _psmCmdNoIO(&g_psmSrv, 6);
}

Result psmEnableEnoughPowerChargeEmulation(void) {
    return _psmCmdNoIO(&g_psmSrv, 8);
}

Result psmDisableEnoughPowerChargeEmulation(void) {
    return _psmCmdNoIO(&g_psmSrv, 9);
}

Result psmEnableFastBatteryCharging(void) {
    return _psmCmdNoIO(&g_psmSrv, 10);
}

Result psmDisableFastBatteryCharging(void) {
    return _psmCmdNoIO(&g_psmSrv, 11);
}

Result psmGetBatteryVoltageState(PsmBatteryVoltageState *out) {
    u32 state;
    Result rc = _psmCmdNoInOutU32(&g_psmSrv, &state, 12);
    if (R_SUCCEEDED(rc) && out) *out = state;
    return rc;
}

Result psmGetRawBatteryChargePercentage(double *out) {
    return _psmCmdNoInOutDouble(&g_psmSrv, out, 13);
}

Result psmIsEnoughPowerSupplied(bool *out) {
    return _psmCmdNoInOutBool(&g_psmSrv, out, 14);
}

Result psmGetBatteryAgePercentage(double *out) {
    return _psmCmdNoInOutDouble(&g_psmSrv, out, 15);
}

Result psmGetBatteryChargeInfoEvent(Event* out_event, bool autoclear) {
    return _psmCmdGetEvent(&g_psmSrv, out_event, autoclear, 16);
}

Result psmGetBatteryChargeInfoFields(PsmBatteryChargeInfoFields *out_fields) {
    if (hosversionBefore(17,0,0)) {
        PsmBatteryChargeInfoFieldsOld fields;
        Result rc = serviceDispatchOut(&g_psmSrv, 17, fields);
        if (R_SUCCEEDED(rc)) {
            memset(out_fields, 0, sizeof(*out_fields));
            memcpy(out_fields, &fields, sizeof(fields));
        }
        return rc;
    }

    return serviceDispatchOut(&g_psmSrv, 17, *out_fields);
}

Result psmGetBatteryChargeCalibratedEvent(Event* out_event, bool autoclear) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _psmCmdGetEvent(&g_psmSrv, out_event, autoclear, 18);
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
