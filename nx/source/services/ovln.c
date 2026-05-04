#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/ovln.h"

static Service g_ovlnrcvSrv;
static Service g_ovlnsndSrv;

NX_GENERATE_SERVICE_GUARD(ovlnrcv);
NX_GENERATE_SERVICE_GUARD(ovlnsnd);

Result _ovlnrcvInitialize(void) {
    return smGetService(&g_ovlnrcvSrv, "ovln:rcv");
}

void _ovlnrcvCleanup(void) {
    serviceClose(&g_ovlnrcvSrv);
}

Result _ovlnsndInitialize(void) {
    return smGetService(&g_ovlnsndSrv, "ovln:snd");
}

void _ovlnsndCleanup(void) {
    serviceClose(&g_ovlnsndSrv);
}

Service* ovlnrcvGetServiceSession(void) {
    return &g_ovlnrcvSrv;
}

Service* ovlnsndGetServiceSession(void) {
    return &g_ovlnsndSrv;
}

Result ovlnrcvOpenReceiver(OvlnReceiver *receiver) {
    return serviceDispatch(&g_ovlnrcvSrv, 0,
        .out_num_objects = 1,
        .out_objects = &receiver->s,
    );
}

void ovlnrcvCloseReceiver(OvlnReceiver *r) {
    serviceClose(&r->s);
}

Result ovlnrcvAddSource(OvlnReceiver *r, const OvlnSourceName *name) {
    return serviceDispatchIn(&r->s, 0, *name);
}

Result ovlnrcvRemoveSource(OvlnReceiver *r, const OvlnSourceName *name) {
    return serviceDispatchIn(&r->s, 1, *name);
}

Result ovlnrcvGetReceiveEventHandle(OvlnReceiver *r, Event* out_event) {
    Handle tmp_handle = INVALID_HANDLE;

    Result rc = serviceDispatch(&r->s, 2,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, true);
    return rc;
}

Result ovlnrcvReceive(OvlnReceiver *r, OvlnRawMessage *message) {
    return serviceDispatchOut(&r->s, 3, *message);
}

Result ovlnrcvReceiveWithTick(OvlnReceiver *r, OvlnRawMessage *message, s64 *tick) {
    struct {
        OvlnRawMessage message;
        s64 tick;
    } out;

    Result rc =  serviceDispatchOut(&r->s, 4, out);
    if (R_SUCCEEDED(rc)) {
        *message = out.message;
        *tick = out.tick;
    }

    return rc;
}

Result ovlnsndOpenSender(OvlnSender *s, const OvlnSourceName *name, OvlnQueueAttribute attribute) {
    const struct {
        OvlnSourceName name;
        OvlnQueueAttribute attribute;
    } in = { *name, attribute };

    return serviceDispatchIn(&g_ovlnsndSrv, 0, in,
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
}

void ovlnsndCloseSender(OvlnSender *s) {
    serviceClose(&s->s);
}

Result ovlnsndSend(OvlnSender *s, OvlnSendOption option, const OvlnRawMessage *message) {
    const struct {
        OvlnSendOption option;
        OvlnRawMessage message;
    } in = { option, *message };

    return serviceDispatchIn(&s->s, 0, in);
}

Result ovlnsndGetUnreceivedMessageCount(OvlnSender *s, u32 *count) {
    return serviceDispatchOut(&s->s, 1, *count);
}
