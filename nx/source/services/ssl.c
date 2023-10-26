#include <string.h>
#include <unistd.h>
#include "service_guard.h"
#include "sf/sessionmgr.h"
#include "runtime/hosversion.h"
#include "services/ssl.h"

static Service g_sslSrv;
static SessionMgr g_sslSessionMgr;
__attribute__((weak)) SslServiceType __nx_ssl_service_type = SslServiceType_Default;

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

    const char *serv = "ssl";
    if (__nx_ssl_service_type == SslServiceType_System) {
        if (hosversionAtLeast(15,0,0))
            serv = "ssl:s";
        else
            __nx_ssl_service_type = SslServiceType_Default;
    }
    Result rc = smGetService(&g_sslSrv, serv);

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

static Result _sslCmdNoIO(Service* srv, u32 cmd_id) {
    return _sslObjectDispatch(srv, cmd_id);
}

static Result _sslCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    return _sslObjectDispatchIn(srv, cmd_id, inval);
}

static Result _sslCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    return _sslObjectDispatchIn(srv, cmd_id, inval);
}

static Result _sslCmdInU8U32NoOut(Service* srv, u8 val0, u32 val1, u32 cmd_id) {
    const struct {
        u8 val0;
        u8 pad[3];
        u32 val1;
    } in = { val0, {0}, val1 };

    return _sslObjectDispatchIn(srv, cmd_id, in);
}

static Result _sslCmdInTwoU32sNoOut(Service* srv, u32 val0, u32 val1, u32 cmd_id) {
    const struct {
        u32 val0;
        u32 val1;
    } in = { val0, val1 };

    return _sslObjectDispatchIn(srv, cmd_id, in);
}

static Result _sslCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return _sslObjectDispatchOut(srv, cmd_id, *out);
}

static Result _sslCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return _sslObjectDispatchOut(srv, cmd_id, *out);
}

static Result _sslCmdInBufNoOut(Service* srv, const void* buffer, size_t size, u32 cmd_id) {
    return _sslObjectDispatch(srv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _sslCmdNoInOutBufTwoOutU32s(Service* srv, u32 *out0, u32 *out1, void* buffer, size_t size, u32 cmd_id) {
    struct {
        u32 out0;
        u32 out1;
    } out;

    Result rc = _sslObjectDispatchOut(srv, cmd_id, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (out0) *out0 = out.out0;
        if (out1) *out1 = out.out1;
    }
    return rc;
}

static Result _sslCmdInBufOutU32(Service* srv, const void* buffer, size_t size, u32 *out, u32 cmd_id) {
    return _sslObjectDispatchOut(srv, cmd_id, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _sslCmdOutBufOutU32(Service* srv, void* buffer, size_t size, u32 *out, u32 cmd_id) {
    return _sslObjectDispatchOut(srv, cmd_id, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result sslCreateContext(SslContext *c, u32 ssl_version) {
    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u32 ssl_version;
        u32 pad;
        u64 pid_placeholder;
    } in = { ssl_version, 0, 0 };

    u32 cmd_id = __nx_ssl_service_type == SslServiceType_System ? 100 : 0;
    return _sslObjectDispatchIn(&g_sslSrv, cmd_id, in,
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

Result sslGetCertificates(void* buffer, u32 size, u32 *ca_cert_ids, u32 count, u32 *total_out) {
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

    if (R_SUCCEEDED(rc) && total_out) *total_out = out_count;

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

Result sslClearTls12FallbackFlag(void) {
    if (hosversionBefore(14,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_sslSrv))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoIO(&g_sslSrv, 9);
}

Result sslSetThreadCoreMask(u64 mask) {
    if (hosversionBefore(15,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_sslSrv) || __nx_ssl_service_type != SslServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU64NoOut(&g_sslSrv, mask, 101);
}

Result sslGetThreadCoreMask(u64 *out) {
    if (hosversionBefore(15,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&g_sslSrv) || __nx_ssl_service_type != SslServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU64(&g_sslSrv, out, 102);
}

// ISslContext

void sslContextClose(SslContext *c) {
    // sdknso uses sslContextGetConnectionCount here, returning the error from there on fail / throwing an error if the output count is non-zero. We won't do that.
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

Result sslContextImportClientPki(SslContext *c, const void* pkcs12, u32 pkcs12_size, const char *pw, u32 pw_size, u64 *id) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (pkcs12==NULL || (pw==NULL && pw_size) || (pw!=NULL && !pw_size))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslObjectDispatchOut(&c->s, 5, *id,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { pkcs12, pkcs12_size },
            { pw, pw_size },
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

Result sslContextAddPolicyOid(SslContext *c, const char *str, u32 str_bufsize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (str==NULL || str_bufsize > 0xff)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdInBufNoOut(&c->s, str, str_bufsize, 9);
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

Result sslContextImportClientCertKeyPki(SslContext *c, const void* cert, u32 cert_size, const void* key, u32 key_size, SslCertificateFormat format, u64 *id) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=format;
    return _sslObjectDispatchInOut(&c->s, 12, tmp, *id,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { cert, cert_size },
            { key, key_size },
        },
    );
}

Result sslContextGeneratePrivateKeyAndCert(SslContext *c, void* cert, u32 cert_size, void* key, u32 key_size, u32 val, const SslKeyAndCertParams *params, u32 *out_certsize, u32 *out_keysize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u32 out_certsize;
        u32 out_keysize;
    } out;

    Result rc = _sslObjectDispatchInOut(&c->s, 13, val, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { cert, cert_size },
            { key, key_size },
            { params, sizeof(*params) },
        },
    );
    if (R_SUCCEEDED(rc)) {
        if (out_certsize) *out_certsize = out.out_certsize;
        if (out_keysize) *out_keysize = out.out_keysize;
    }
    return rc;
}

Result sslContextCreateConnectionForSystem(SslContext *c, SslConnection *conn) {
    if (hosversionBefore(15,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&c->s) || __nx_ssl_service_type != SslServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslObjectDispatch(&c->s, 100,
        .out_num_objects = 1,
        .out_objects = &conn->s,
    );
}

// ISslConnection

void sslConnectionClose(SslConnection *c) {
    _sslObjectClose(&c->s);
}

Result sslConnectionSetSocketDescriptor(SslConnection *c, int sockfd, int *out_sockfd) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslObjectDispatchInOut(&c->s, 0, sockfd, *out_sockfd);
}

Result sslConnectionSetHostName(SslConnection *c, const char *str, u32 str_bufsize) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (str==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdInBufNoOut(&c->s, str, str_bufsize, 1);
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

Result sslConnectionGetHostName(SslConnection *c, char *str, u32 str_bufsize, u32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (str==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdOutBufOutU32(&c->s, str, str_bufsize, out, 5);
}

Result sslConnectionGetVerifyOption(SslConnection *c, u32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU32(&c->s, out, 6);
}

Result sslConnectionGetIoMode(SslConnection *c, SslIoMode *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=0;
    Result rc = _sslCmdNoInOutU32(&c->s, &tmp, 7);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result sslConnectionDoHandshake(SslConnection *c, u32 *out_size, u32 *total_certs, void* server_certbuf, u32 server_certbuf_size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (out_size) *out_size = 0;
    if (total_certs) *total_certs = 0;

    if (server_certbuf==NULL || !server_certbuf_size)
        return _sslCmdNoIO(&c->s, 8); // DoHandshake

    return _sslCmdNoInOutBufTwoOutU32s(&c->s, out_size, total_certs, server_certbuf, server_certbuf_size, 9); // DoHandshakeGetServerCert
}

Result sslConnectionGetServerCertDetail(const void* certbuf, u32 certbuf_size, u32 cert_index, void** cert, u32 *cert_size) {
    if (certbuf==NULL || cert==NULL || cert_size==NULL || certbuf_size < sizeof(SslServerCertDetailHeader) + cert_index*sizeof(SslServerCertDetailEntry))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    // sdknso doesn't have a certbuf_size param for this. sdknso also uses a struct for output, but that just contains a size/addr anyway.

    SslServerCertDetailHeader *hdr = (SslServerCertDetailHeader*)certbuf;
    if (hdr->magicnum != 0x4E4D684374726543 || hdr->cert_total <= cert_index) // "CertChMN"
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    SslServerCertDetailEntry *entry = (SslServerCertDetailEntry*)(++hdr);
    entry+= cert_index;

    if (certbuf_size <= entry->offset || certbuf_size < entry->size || certbuf_size < entry->offset + entry->size)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    *cert = (void*)certbuf + entry->offset;
    *cert_size = entry->size;

    return 0;
}

Result sslConnectionRead(SslConnection *c, void* buffer, u32 size, u32 *out_size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer==NULL || !size)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdOutBufOutU32(&c->s, buffer, size, out_size, 10);
}

Result sslConnectionWrite(SslConnection *c, const void* buffer, u32 size, u32 *out_size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer==NULL || !size)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdInBufOutU32(&c->s, buffer, size, out_size, 11);
}

Result sslConnectionPending(SslConnection *c, s32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU32(&c->s, (u32*)out, 12);
}

Result sslConnectionPeek(SslConnection *c, void* buffer, u32 size, u32 *out_size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer==NULL || !size)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdOutBufOutU32(&c->s, buffer, size, out_size, 13);
}

Result sslConnectionPoll(SslConnection *c, u32 in_pollevent, u32 *out_pollevent, u32 timeout) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u32 in_pollevent;
        u32 timeout;
    } in = { in_pollevent, timeout };

    return _sslObjectDispatchInOut(&c->s, 14, in, *out_pollevent);
}

Result sslConnectionGetVerifyCertError(SslConnection *c) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoIO(&c->s, 15);
}

Result sslConnectionGetNeededServerCertBufferSize(SslConnection *c, u32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoInOutU32(&c->s, out, 16);
}

Result sslConnectionSetSessionCacheMode(SslConnection *c, SslSessionCacheMode mode) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU32NoOut(&c->s, mode, 17);
}

Result sslConnectionGetSessionCacheMode(SslConnection *c, SslSessionCacheMode *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=0;
    Result rc = _sslCmdNoInOutU32(&c->s, &tmp, 18);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result sslConnectionFlushSessionCache(SslConnection *c) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdNoIO(&c->s, 19);
}

Result sslConnectionSetRenegotiationMode(SslConnection *c, SslRenegotiationMode mode) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU32NoOut(&c->s, mode, 20);
}

Result sslConnectionGetRenegotiationMode(SslConnection *c, SslRenegotiationMode *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=0;
    Result rc = _sslCmdNoInOutU32(&c->s, &tmp, 21);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result sslConnectionSetOption(SslConnection *c, SslOptionType option, bool flag) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _sslCmdInU8U32NoOut(&c->s, flag!=0, option, 22);
}

Result sslConnectionGetOption(SslConnection *c, SslOptionType option, bool *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u32 tmp=option;
    u8 tmpout=0;
    Result rc = _sslObjectDispatchInOut(&c->s, 23, tmp, tmpout);
    if (R_SUCCEEDED(rc) && out) *out = tmpout & 1;
    return rc;
}

Result sslConnectionGetVerifyCertErrors(SslConnection *c, u32 *out0, u32 *out1, Result *errors, u32 count) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (out0==NULL || errors==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (out1) *out1 = 0;

    u32 tmp=0;
    Result rc = _sslCmdNoInOutBufTwoOutU32s(&c->s, out0, &tmp, errors, count*sizeof(Result), 24);
    if (R_SUCCEEDED(rc) && out1) *out1 = tmp;
    if (R_SUCCEEDED(rc) && *out0 != tmp) rc = MAKERESULT(123, 112);
    return rc;
}

Result sslConnectionGetCipherInfo(SslConnection *c, SslCipherInfo *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=1;
    return _sslObjectDispatchIn(&c->s, 25, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result sslConnectionSetNextAlpnProto(SslConnection *c, const u8 *buffer, u32 size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (buffer==NULL || !size)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    return _sslCmdInBufNoOut(&c->s, buffer, size, 26);
}

Result sslConnectionGetNextAlpnProto(SslConnection *c, SslAlpnProtoState *state, u32 *out, u8 *buffer, u32 size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (buffer==NULL || !size)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (state) *state = SslAlpnProtoState_NoSupport;
    if (out) *out = 0;
    buffer[0] = 0;

    u32 tmp=0;
    Result rc = _sslCmdNoInOutBufTwoOutU32s(&c->s, &tmp, out, buffer, size, 27);
    if (R_SUCCEEDED(rc) && state) *state = tmp;
    return rc;
}

Result sslConnectionSetDtlsSocketDescriptor(SslConnection *c, int sockfd, const void* buf, size_t size, int *out_sockfd) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslObjectDispatchInOut(&c->s, 28, sockfd, *out_sockfd,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = {
            { buf, size },
        },
    );
}

Result sslConnectionGetDtlsHandshakeTimeout(SslConnection *c, u64 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslObjectDispatch(&c->s, 29,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = {
            { out, sizeof(*out) },
        },
    );
}

Result sslConnectionSetPrivateOption(SslConnection *c, SslPrivateOptionType option, u32 value) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (hosversionBefore(17,0,0))
        return _sslCmdInU8U32NoOut(&c->s, value!=0, option, 30);
    else
        return _sslCmdInTwoU32sNoOut(&c->s, option, value, 30);
}

Result sslConnectionSetSrtpCiphers(SslConnection *c, const u16 *ciphers, u32 count) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslCmdInBufNoOut(&c->s, ciphers, count*sizeof(u16), 31);
}

Result sslConnectionGetSrtpCipher(SslConnection *c, u16 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslObjectDispatchOut(&c->s, 32, *out);
}

Result sslConnectionExportKeyingMaterial(SslConnection *c, u8 *outbuf, u32 outbuf_size, const char *label, u32 label_size, const void* context, u32 context_size) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslObjectDispatch(&c->s, 33,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { outbuf, outbuf_size },
            { label, label_size },
            { context, context_size },
        },
    );
}

Result sslConnectionSetIoTimeout(SslConnection *c, u32 timeout) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslCmdInU32NoOut(&c->s, timeout, 34);
}

Result sslConnectionGetIoTimeout(SslConnection *c, u32 *out) {
    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(16,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _sslCmdNoInOutU32(&c->s, out, 35);
}

