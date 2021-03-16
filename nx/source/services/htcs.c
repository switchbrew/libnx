#include "service_guard.h"
#include "sf/sessionmgr.h"
#include "runtime/hosversion.h"
#include "services/htcs.h"

static Service g_htcsSrv;
static Service g_htcsMonitor;

static SessionMgr g_htcsSessionMgr;

NX_INLINE bool _htcsObjectIsChild(Service* s) {
    return s->session == g_htcsSrv.session;
}

static void _htcsObjectClose(Service* s) {
    if (!_htcsObjectIsChild(s)) {
        serviceClose(s);
    }
    else {
        int slot = sessionmgrAttachClient(&g_htcsSessionMgr);
        uint32_t object_id = serviceGetObjectId(s);
        serviceAssumeDomain(s);
        cmifMakeCloseRequest(armGetTls(), object_id);
        svcSendSyncRequest(sessionmgrGetClientSession(&g_htcsSessionMgr, slot));
        sessionmgrDetachClient(&g_htcsSessionMgr, slot);
    }
}

NX_INLINE Result _htcsObjectDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
) {
    int slot = -1;
    if (_htcsObjectIsChild(s)) {
        slot = sessionmgrAttachClient(&g_htcsSessionMgr);
        if (slot < 0) __builtin_unreachable();
        disp.target_session = sessionmgrGetClientSession(&g_htcsSessionMgr, slot);
        serviceAssumeDomain(s);
    }

    Result rc = serviceDispatchImpl(s, request_id, in_data, in_data_size, out_data, out_data_size, disp);

    if (slot >= 0) {
        sessionmgrDetachClient(&g_htcsSessionMgr, slot);
    }

    return rc;
}

#define _htcsObjectDispatch(_s,_rid,...) \
    _htcsObjectDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _htcsObjectDispatchIn(_s,_rid,_in,...) \
    _htcsObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _htcsObjectDispatchOut(_s,_rid,_out,...) \
    _htcsObjectDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _htcsObjectDispatchInOut(_s,_rid,_in,_out,...) \
    _htcsObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

NX_GENERATE_SERVICE_GUARD_PARAMS(htcs, (u32 num_sessions), (num_sessions));

Result _htcsInitialize(u32 num_sessions) {
    Result rc = smGetServiceWrapper(&g_htcsSrv, smEncodeName("htcs"));

    if (R_SUCCEEDED(rc))
        rc = smGetServiceWrapper(&g_htcsMonitor, smEncodeName("htcs"));

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_htcsSrv);

    if (R_SUCCEEDED(rc)) {
        u64 pid_placeholder = 0;
        serviceAssumeDomain(&g_htcsSrv);
        rc = serviceDispatchIn(&g_htcsSrv, 100, pid_placeholder, .in_send_pid = true);
    }

    if (R_SUCCEEDED(rc)) {
        u64 pid_placeholder = 0;
        rc = serviceDispatchIn(&g_htcsMonitor, 101, pid_placeholder, .in_send_pid = true);
    }

    if (R_SUCCEEDED(rc))
        rc = sessionmgrCreate(&g_htcsSessionMgr, g_htcsSrv.session, num_sessions);

    return rc;
}

void _htcsCleanup() {
    sessionmgrClose(&g_htcsSessionMgr);
    serviceClose(&g_htcsMonitor);
    serviceClose(&g_htcsSrv);
}

Service* htcsGetManagerServiceSession(void) {
    return &g_htcsSrv;
}

Service* htcsGetMonitorServiceSession(void) {
    return &g_htcsMonitor;
}

static Result _htcsGetPeerName(HtcsPeerName *out, u32 cmd_id) {
    return _htcsObjectDispatchOut(&g_htcsSrv, cmd_id, *out);
}

Result htcsGetPeerNameAny(HtcsPeerName *out) {
    return _htcsGetPeerName(out, 10);
}

Result htcsGetDefaultHostName(HtcsPeerName *out) {
    return _htcsGetPeerName(out, 11);
}

Result htcsCreateSocket(s32 *out_err, HtcsSocket *out, bool enable_disconnection_emulation) {
    const u8 in = (enable_disconnection_emulation) ? 1 : 0;
    return _htcsObjectDispatchInOut(&g_htcsSrv, 13, in, *out_err,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result htcsStartSelect(u32 *out_task_id, Handle *out_event_handle, const s32 *read, size_t num_read, const s32 *write, size_t num_write, const s32 *except, size_t num_except, s64 tv_sec, s64 tv_usec) {
    const struct {
        s64 tv_sec;
        s64 tv_usec;
    } in = { tv_sec, tv_usec };
    return _htcsObjectDispatchInOut(&g_htcsSrv, 130, in, *out_task_id,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { read, num_read * sizeof(*read) },
            { write, num_write * sizeof(*write) },
            { except, num_except * sizeof(*except) },
        },
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_event_handle,
    );
}

Result htcsEndSelect(s32 *out_err, s32 *out_count, s32 *read, size_t num_read, s32 *write, size_t num_write, s32 *except, size_t num_except, u32 task_id) {
    struct {
        s32 err;
        s32 count;
    } out;
    Result rc = _htcsObjectDispatchInOut(&g_htcsSrv, 131, task_id, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { read, num_read * sizeof(*read) },
            { write, num_write * sizeof(*write) },
            { except, num_except * sizeof(*except) },
        }
    );
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_count) *out_count = out.count;
    }
    return rc;
}

static Result _htcsSocketCmdInAddress(HtcsSocket *s, s32 *out_err, s32 *out_res, const HtcsSockAddr *address, u32 cmd_id) {
    struct {
        s32 err;
        s32 res;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, cmd_id, *address, out);
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_res) *out_res = out.res;
    }
    return rc;
}

static Result _htcsSocketCmdInS32(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 value, u32 cmd_id) {
    struct {
        s32 err;
        s32 res;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, cmd_id, value, out);
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_res) *out_res = out.res;
    }
    return rc;
}

static Result _htcsSocketCmdInU32OutErrSize(HtcsSocket *s, s32 *out_err, s64 *out_size, u32 value, u32 cmd_id) {
    struct {
        s32 err;
        s64 size;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, cmd_id, value, out);
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_size) *out_size = out.size;
    }
    return rc;
}

Result htcsSocketClose(HtcsSocket *s, s32 *out_err, s32 *out_res) {
    struct {
        s32 err;
        s32 res;
    } out;
    Result rc = _htcsObjectDispatchOut(&s->s, 0, out);
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_res) *out_res = out.res;
    }
    return rc;
}

Result htcsSocketConnect(HtcsSocket *s, s32 *out_err, s32 *out_res, const HtcsSockAddr *address) {
    return _htcsSocketCmdInAddress(s, out_err, out_res, address, 1);
}

Result htcsSocketBind(HtcsSocket *s, s32 *out_err, s32 *out_res, const HtcsSockAddr *address) {
    return _htcsSocketCmdInAddress(s, out_err, out_res, address, 2);
}

Result htcsSocketListen(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 backlog_count) {
    return _htcsSocketCmdInS32(s, out_err, out_res, backlog_count, 3);
}

Result htcsSocketShutdown(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 how) {
    return _htcsSocketCmdInS32(s, out_err, out_res, how, 7);
}

Result htcsSocketFcntl(HtcsSocket *s, s32 *out_err, s32 *out_res, s32 command, s32 value) {
    const struct {
        s32 command;
        s32 value;
    } in = { command, value };
    struct {
        s32 err;
        s32 res;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, 8, in, out);
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_res) *out_res = out.res;
    }
    return rc;
}

Result htcsSocketAcceptStart(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle) {
    return _htcsObjectDispatchOut(&s->s, 9, *out_task_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_event_handle,
    );
}

Result htcsSocketAcceptResults(HtcsSocket *s, s32 *out_err, HtcsSocket *out_socket, HtcsSockAddr *out_address, u32 task_id) {
    struct {
        HtcsSockAddr address;
        s32 err;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, 10, task_id, out,
        .out_num_objects = 1,
        .out_objects = &out_socket->s,
    );
    if (R_SUCCEEDED(rc)) {
        if (out_address) *out_address = out.address;
        if (out_err) *out_err = out.err;
    }
    return rc;
}

Result htcsSocketRecvStart(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, s32 mem_size, s32 flags) {
    const struct {
        s32 mem_size;
        s32 flags;
    } in = { mem_size, flags };
    return _htcsObjectDispatchInOut(&s->s, 11, in, *out_task_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_event_handle,
    );
}

Result htcsSocketRecvResults(HtcsSocket *s, s32 *out_err, s64 *out_size, void *buffer, size_t buffer_size, u32 task_id) {
    struct {
        s32 err;
        s64 size;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, 12, task_id, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_size) *out_size = out.size;
    }
    return rc;
}

Result htcsSocketSendStart(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, const void *buffer, size_t buffer_size, s32 flags) {
    return _htcsObjectDispatchInOut(&s->s, 22, flags, *out_task_id,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buffer, buffer_size } },
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_event_handle,
    );
}

Result htcsSocketSendResults(HtcsSocket *s, s32 *out_err, s64 *out_size, u32 task_id) {
    return _htcsSocketCmdInU32OutErrSize(s, out_err, out_size, task_id, 16);
}

Result htcsSocketStartSend(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, s64 *out_max_size, s64 size, s32 flags) {
    const struct {
        s32 flags;
        s64 size;
    } in = { flags, size };
    struct {
        u32 task_id;
        s64 max_size;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, 17, in, out,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_event_handle,
    );
    if (R_SUCCEEDED(rc)) {
        if (out_task_id) *out_task_id = out.task_id;
        if (out_max_size) *out_max_size = out.max_size;
    }
    return rc;
}

Result htcsSocketContinueSend(HtcsSocket *s, s64 *out_size, bool *out_wait, const void *buffer, size_t buffer_size, u32 task_id) {
    struct {
        u8 wait;
        s64 size;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, 23, task_id, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In | SfBufferAttr_HipcMapTransferAllowsNonSecure },
        .buffers = { { buffer, buffer_size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (out_wait) *out_wait = out.wait != 0;
        if (out_size) *out_size = out.size;
    }
    return rc;
}

Result htcsSocketEndSend(HtcsSocket *s, s32 *out_err, s64 *out_size, u32 task_id) {
    return _htcsSocketCmdInU32OutErrSize(s, out_err, out_size, task_id, 19);
}

Result htcsSocketStartRecv(HtcsSocket *s, u32 *out_task_id, Handle *out_event_handle, s64 size, s32 flags) {
    const struct {
        s32 flags;
        s64 size;
    } in = { flags, size };
    return _htcsObjectDispatchInOut(&s->s, 20, in, *out_task_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = out_event_handle,
    );
}

Result htcsSocketEndRecv(HtcsSocket *s, s32 *out_err, s64 *out_size, void *buffer, size_t buffer_size, u32 task_id) {
    struct {
        s32 err;
        s64 size;
    } out;
    Result rc = _htcsObjectDispatchInOut(&s->s, 21, task_id, out,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (out_err) *out_err = out.err;
        if (out_size) *out_size = out.size;
    }
    return rc;
}

Result htcsSocketGetPrimitive(HtcsSocket *s, s32 *out) {
    return _htcsObjectDispatchOut(&s->s, 130, *out);
}

void htcsCloseSocket(HtcsSocket *s) {
    _htcsObjectClose(&s->s);
}
