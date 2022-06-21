#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/ovln.h"

static Service g_ovlnsndSrv;
static Service g_ovlnrcvSrv;

NX_GENERATE_SERVICE_GUARD(ovlnsnd);
NX_GENERATE_SERVICE_GUARD(ovlnrcv);

Result _ovlnsndInitialize(void) {
    return smGetService(&g_ovlnsndSrv, "ovln:snd");
}

void _ovlnsndCleanup(void) {
    serviceClose(&g_ovlnsndSrv);
}

Result _ovlnrcvInitialize(void) {
    return smGetService(&g_ovlnrcvSrv, "ovln:rcv");
}

void _ovlnrcvCleanup(void) {
    serviceClose(&g_ovlnrcvSrv);
}

Service* ovlnsndGetServiceSession(void) {
    return &g_ovlnsndSrv;
}

Service* ovlnrcvGetServiceSession(void) {
    return &g_ovlnrcvSrv;
}

Result ovlnsndOpenSender(OvlnSenderSession *sender, const OvlnName *name, u64 queue_length) {
    const struct {
        OvlnName name;
        u64 queue_length;
    } in = { *name, queue_length };

    return serviceDispatchIn(&g_ovlnsndSrv, 0, in,
        .out_num_objects = 1,
        .out_objects = &sender->s,
    );
}

void ovlnsndCloseSender(OvlnSenderSession *sender) {
    serviceClose(&sender->s);
}

Result ovlnsndSend(OvlnSenderSession *sender, u64 id, const OvlnMessage *message) {
    const struct {
        u64 id;
        OvlnMessage message;
    } in = { id, *message };

    return serviceDispatchIn(&sender->s, 0, in);
}

Result ovlnsndGetUnreceivedMessageCount(OvlnSenderSession *sender, u32 *count) {
    return serviceDispatchOut(&sender->s, 1, *count);
}

Result ovlnrcvOpenReceiver(OvlnReceiverSession *receiver) {
    return serviceDispatch(&g_ovlnrcvSrv, 0,
        .out_num_objects = 1,
        .out_objects = &receiver->s,
    );
}

void ovlnrcvCloseReceiver(OvlnReceiverSession *receiver) {
    serviceClose(&receiver->s);
}

Result ovlnrcvAddSource(OvlnReceiverSession *receiver, const OvlnName *source) {
    return serviceDispatchIn(&receiver->s, 0, *source);
}

Result ovlnrcvRemoveSource(OvlnReceiverSession *receiver, const OvlnName *source) {
    return serviceDispatchIn(&receiver->s, 1, *source);
}

Result ovlnrcvGetReceiveEventHandle(OvlnReceiverSession *receiver, Event* out_event) {
    Handle tmp_handle = INVALID_HANDLE;

    Result rc = serviceDispatch(&receiver->s, 2,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, true);
    return rc;
}

Result ovlnrcvReceive(OvlnReceiverSession *receiver, OvlnMessage *message) {
    return serviceDispatchOut(&receiver->s, 3, *message);
}

Result ovlnrcvReceiveWithTick(OvlnReceiverSession *receiver, OvlnMessage *message, s64 *tick) {
    struct {
        OvlnMessage message;
        s64 tick;
    } out;

    Result rc =  serviceDispatchOut(&receiver->s, 4, out);
    if (R_SUCCEEDED(rc)) {
        *message = out.message;
        *tick = out.tick;
    }

    return rc;
}
