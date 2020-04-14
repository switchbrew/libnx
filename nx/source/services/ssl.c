#include <string.h>
#include <unistd.h>
#include "service_guard.h"
#include "sf/sessionmgr.h"
#include "runtime/hosversion.h"
#include "services/ssl.h"

static Service g_sslSrv;
static SessionMgr g_sslSessionMgr;

static Result _sslSetInterfaceVersion(u32 version);

NX_INLINE bool _sslObjectIsChild(Service* s)
{
    return s->session == g_sslSrv.session;
}

static void _sslObjectClose(Service* s) {
    if (!_sslObjectIsChild(s)) {
        serviceClose(s);
    }
    else {
        int slot = sessionmgrAttachClient(&g_sslSessionMgr);
        uint32_t object_id = serviceGetObjectId(s);
        serviceAssumeDomain(s);
        cmifMakeCloseRequest(armGetTls(), object_id);
        svcSendSyncRequest(sessionmgrGetClientSession(&g_sslSessionMgr, slot));
        sessionmgrDetachClient(&g_sslSessionMgr, slot);
    }
}

NX_INLINE Result _sslObjectDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
) {
    int slot = -1;
    if (_sslObjectIsChild(s)) {
        slot = sessionmgrAttachClient(&g_sslSessionMgr);
        if (slot < 0) __builtin_unreachable();
        disp.target_session = sessionmgrGetClientSession(&g_sslSessionMgr, slot);
        serviceAssumeDomain(s);
    }

    Result rc = serviceDispatchImpl(s, request_id, in_data, in_data_size, out_data, out_data_size, disp);

    if (slot >= 0) {
        sessionmgrDetachClient(&g_sslSessionMgr, slot);
    }

    return rc;
}

#define _sslObjectDispatch(_s,_rid,...) \
    _sslObjectDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _sslObjectDispatchIn(_s,_rid,_in,...) \
    _sslObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _sslObjectDispatchOut(_s,_rid,_out,...) \
    _sslObjectDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _sslObjectDispatchInOut(_s,_rid,_in,_out,...) \
    _sslObjectDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

NX_GENERATE_SERVICE_GUARD_PARAMS(ssl, (u32 num_sessions), (num_sessions));

Result _sslInitialize(u32 num_sessions) {
    if (num_sessions > 4)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    Result rc = smGetService(&g_sslSrv, "ssl");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_sslSrv);
    }

    if (hosversionAtLeast(3,0,0)) {
        u32 version = 0x1;

        if (hosversionAtLeast(5,0,0))
            version = 0x2;
        if (hosversionAtLeast(6,0,0))
            version = 0x3;

        rc = _sslSetInterfaceVersion(version);
    }

    if (R_SUCCEEDED(rc))
        rc = sessionmgrCreate(&g_sslSessionMgr, g_sslSrv.session, num_sessions);

    return rc;
}

void _sslCleanup(void) {
    // Close extra sessions
    sessionmgrClose(&g_sslSessionMgr);

    // We can't assume g_sslSrv is a domain here because serviceConvertToDomain might have failed
    serviceClose(&g_sslSrv);
}

Service* sslGetServiceSession(void) {
    return &g_sslSrv;
}

static Result _sslCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    return _sslObjectDispatchIn(srv, cmd_id, inval);
}

static Result _sslCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return _sslObjectDispatchOut(srv, cmd_id, *out);
}

Result sslCreateContext(SslContext *c, SslVersion ssl_version) {
    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u32 ssl_version;
        u32 pad;
        u64 pid_placeholder;
    } in = { ssl_version, 0, 0 };

    return _sslObjectDispatchIn(&g_sslSrv, 0, in,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = &c->s,
    );
}

Result sslGetContextCount(u32 *out) {
    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU32(&g_sslSrv, out, 1);
}

static Result _sslGetCertificates(void* buffer, u32 size, u32 *ca_cert_ids, u32 count, u32 *out) {
    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(3,0,0)) {
        Result rc = _sslObjectDispatch(&g_sslSrv, 2,
            .buffer_attrs = {
                SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
                SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            },
            .buffers = {
                { buffer, size },
                { ca_cert_ids, count*sizeof(*ca_cert_ids) },
            },
        );
        if (R_SUCCEEDED(rc) && out) *out = count;
        return rc;
    }

    return _sslObjectDispatchOut(&g_sslSrv, 2, *out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { buffer, size },
            { ca_cert_ids, count*sizeof(*ca_cert_ids) },
        },
    );
}

Result sslGetCertificates(void* buffer, u32 size, u32 *ca_cert_ids, u32 count) {
    if (buffer==NULL || !size || ca_cert_ids==NULL || !count)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    u32 out_count=0;
    Result rc = _sslGetCertificates(buffer, size, ca_cert_ids, count, &out_count);

    if (R_SUCCEEDED(rc)) {
        SslBuiltInCertificateInfo *certs = (SslBuiltInCertificateInfo*)buffer;
        for (u32 i=0; i<out_count; i++) {
            // sdknso doesn't check this.
            if ((uintptr_t)certs[i].cert_data >= (uintptr_t)size || certs[i].cert_size >= size || (uintptr_t)certs[i].cert_data + (uintptr_t)certs[i].cert_size > (uintptr_t)size)
                return MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);

            certs[i].cert_data+= (uintptr_t)buffer;
        }
    }

    return rc;
}

Result sslGetCertificateBufSize(u32 *ca_cert_ids, u32 count, u32 *out) {
    if (ca_cert_ids==NULL || !count)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslObjectDispatchOut(&g_sslSrv, 3, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { ca_cert_ids, count*sizeof(*ca_cert_ids) } },
    );
}

static Result _sslSetInterfaceVersion(u32 version) { // [3.0.0+]
    serviceAssumeDomain(&g_sslSrv);
    return serviceDispatchIn(&g_sslSrv, 5, version);
}

static Result _sslFlushSessionCache(const char *str, size_t str_size, u32 type, u32 *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslObjectDispatchInOut(&g_sslSrv, 6, type, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { str, str_size } },
    );
}

Result sslFlushSessionCache(const char *str, size_t str_bufsize, SslFlushSessionCacheOptionType type, u32 *out) {
    size_t str_size=0;
    if (out) *out = 0;

    if (type == SslFlushSessionCacheOptionType_SingleHost) {
        if (str==NULL)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        str_size = strnlen(str, str_bufsize);

        if (!str_size)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        str_size++;
    }
    else if (type == SslFlushSessionCacheOptionType_AllHosts) {
        if (str || str_bufsize)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }
    else {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    return _sslFlushSessionCache(str, str_size, type, out);
}

Result sslSetDebugOption(const void* buffer, size_t size, SslDebugOptionType type) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    u32 tmp=type;
    return _sslObjectDispatchIn(&g_sslSrv, 7, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result sslGetDebugOption(void* buffer, size_t size, SslDebugOptionType type) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=type;
    return _sslObjectDispatchIn(&g_sslSrv, 8, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

// ISslContext

void sslContextClose(SslContext *c) {
    _sslObjectClose(&c->s);
}

Result sslContextSetOption(SslContext *c, SslContextOption option, s32 value) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u32 option;
        s32 value;
    } in = { option, value };

    return _sslObjectDispatchIn(&c->s, 0, in);
}

Result sslContextGetOption(SslContext *c, SslContextOption option, s32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=option;
    return _sslObjectDispatchInOut(&c->s, 1, tmp, *out);
}

Result sslContextCreateConnection(SslContext *c, SslConnection *conn) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    memset(conn, 0, sizeof(*conn));
    conn->sockfd = -1;

    return _sslObjectDispatch(&c->s, 2,
        .out_num_objects = 1,
        .out_objects = &conn->s,
    );
}

Result sslContextGetConnectionCount(SslContext *c, u32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU32(&c->s, out, 3);
}

Result sslContextImportServerPki(SslContext *c, const void* buffer, u32 size, SslCertificateFormat format, u64 *id) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    u32 tmp = format;
    return _sslObjectDispatchInOut(&c->s, 4, tmp, *id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

Result sslContextImportClientPki(SslContext *c, const void* buf0, u32 size0, const void* buf1, u32 size1, u64 *id) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buf0==NULL || (buf1==NULL && size1) || (buf1!=NULL && !size1))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslObjectDispatchOut(&c->s, 5, *id,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { buf0, size0 },
            { buf1, size1 },
        },
    );
}

Result sslContextRemovePki(SslContext *c, u64 id) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc=0;
    u32 cnt = hosversionBefore(3,0,0) ? 2 : 3;

    for (u32 i=0; i<cnt; i++) {
        u32 cmd_id=0;
        if (i==0)
            cmd_id = 6; // RemoveServerPki
        else if (i==1)
            cmd_id = 7; // RemoveClientPki
        else if (i==2)
            cmd_id = 11; // RemoveCrl

        rc = _sslObjectDispatchIn(&c->s, cmd_id, id);

        if (i<2 && R_VALUE(rc) != MAKERESULT(123, 214)) break;
    }

    return rc;
}

Result sslContextRegisterInternalPki(SslContext *c, SslInternalPki internal_pki, u64 *id) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp = internal_pki;
    return _sslObjectDispatchInOut(&c->s, 8, tmp, *id);
}

Result sslContextAddPolicyOid(SslContext *c, const char* str, u32 str_bufsize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (str==NULL || str_bufsize > 0xff)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslObjectDispatch(&c->s, 9,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { str, str_bufsize } },
    );
}

Result sslContextImportCrl(SslContext *c, const void* buffer, u32 size, u64 *id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslObjectDispatchOut(&c->s, 10, *id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

// ISslConnection

void sslConnectionClose(SslConnection *c) {
    if (c->sockfd >= 0) close(c->sockfd);
    _sslObjectClose(&c->s);

    memset(c, 0, sizeof(*c));
    c->sockfd = -1;
}

Result sslConnectionSetSocketDescriptor(SslConnection *c, int sockfd) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslObjectDispatchInOut(&c->s, 0, sockfd, c->sockfd);
}

Result sslConnectionSetHostName(SslConnection *c, const char* str, u32 str_bufsize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (str==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslObjectDispatch(&c->s, 1,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { str, str_bufsize } },
    );
}

Result sslConnectionSetVerifyOption(SslConnection *c, u32 verify_option) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU32NoOut(&c->s, verify_option, 2);
}

Result sslConnectionSetIoMode(SslConnection *c, SslIoMode mode) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU32NoOut(&c->s, mode, 3);
}

Result sslConnectionGetSocketDescriptor(SslConnection *c, int *sockfd) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU32(&c->s, (u32*)sockfd, 4);
}

Result sslConnectionGetHostName(SslConnection *c, char* str, u32 str_bufsize, u32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (str==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslObjectDispatchOut(&c->s, 5, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { str, str_bufsize } },
    );
}

Result sslConnectionSetSessionCacheMode(SslConnection *c, SslSessionCacheMode mode) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU32NoOut(&c->s, mode, 17);
}

Result sslConnectionSetRenegotiationMode(SslConnection *c, SslRenegotiationMode mode) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU32NoOut(&c->s, mode, 20);
}

