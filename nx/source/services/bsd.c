#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <errno.h>
#include <string.h>

// Complete definition of struct timeout:
#include <sys/time.h>

#include <fcntl.h>

// For ioctls:
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_media.h>

#include "service_guard.h"
#include "kernel/shmem.h"
#include "kernel/rwlock.h"
#include "sf/sessionmgr.h"
#include "services/bsd.h"

typedef struct BsdSelectTimeval {
    struct timeval tv;
    bool is_null;
} BsdSelectTimeval;

__thread Result g_bsdResult;
__thread int g_bsdErrno;

static Service g_bsdSrv;
static Service g_bsdMonitor;
static SessionMgr g_bsdSessionMgr;
static u64 g_bsdClientPid = -1;

static TransferMemory g_bsdTmem;

static const BsdInitConfig g_defaultBsdInitConfig = {
    .version = 1,

    .tcp_tx_buf_size        = 0x8000,
    .tcp_rx_buf_size        = 0x10000,
    .tcp_tx_buf_max_size    = 0x40000,
    .tcp_rx_buf_max_size    = 0x40000,

    .udp_tx_buf_size = 0x2400,
    .udp_rx_buf_size = 0xA500,

    .sb_efficiency = 4,
};

NX_GENERATE_SERVICE_GUARD_PARAMS(bsd, (const BsdInitConfig *config, u32 num_sessions, u32 service_type), (config, num_sessions, service_type));

// This function computes the minimal size of the transfer memory to be passed to @ref bsdInitalize.
// Should the transfer memory be smaller than that, the BSD sockets service would only send
// ZeroWindow packets (for TCP), resulting in a transfer rate not exceeding 1 byte/s.
NX_CONSTEXPR size_t _bsdGetTransferMemSizeForConfig(const BsdInitConfig *config) {
    u32 tcp_tx_buf_max_size = config->tcp_tx_buf_max_size != 0 ? config->tcp_tx_buf_max_size : config->tcp_tx_buf_size;
    u32 tcp_rx_buf_max_size = config->tcp_rx_buf_max_size != 0 ? config->tcp_rx_buf_max_size : config->tcp_rx_buf_size;
    u32 sum = tcp_tx_buf_max_size + tcp_rx_buf_max_size + config->udp_tx_buf_size + config->udp_rx_buf_size;

    sum = (sum + 0xFFF) &~ 0xFFF; // page round-up
    return (size_t)(config->sb_efficiency * sum);
}

NX_CONSTEXPR BsdSelectTimeval _bsdCreateSelectTimeval(struct timeval *timeval) {
    BsdSelectTimeval ret = {};
    if (timeval)
        ret.tv = *timeval;
    else
        ret.is_null = true;
    return ret;
}

static Result _bsdRegisterClient(TransferMemory* tmem, const BsdInitConfig *config, u64* pid_out) {
    const struct {
        BsdInitConfig config;
        u64 pid_placeholder;
        u64 tmem_sz;
    } in = { *config, 0, tmem->size };

    Result rc = serviceDispatchInOut(&g_bsdSrv, 0, in, *pid_out,
        .in_send_pid = true,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );

    g_bsdResult = rc;
    g_bsdErrno = 0;
    return rc;
}

static Result _bsdStartMonitoring(u64 pid) {
    Result rc = serviceDispatchIn(&g_bsdMonitor, 1, pid, .in_send_pid = true);
    g_bsdResult = rc;
    g_bsdErrno = 0;
    return rc;
}

NX_INLINE int _bsdDispatchImpl(
    u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
)
{
    // Make a copy of the service struct, so that the compiler can assume that it won't be modified by function calls.
    Service srv = g_bsdSrv;

    void* in = serviceMakeRequest(&srv, request_id, disp.context,
        in_data_size, disp.in_send_pid,
        disp.buffer_attrs, disp.buffers,
        disp.in_num_objects, disp.in_objects,
        disp.in_num_handles, disp.in_handles);

    if (in_data_size)
        __builtin_memcpy(in, in_data, in_data_size);

    int slot = sessionmgrAttachClient(&g_bsdSessionMgr);
    Result rc = svcSendSyncRequest(sessionmgrGetClientSession(&g_bsdSessionMgr, slot));
    sessionmgrDetachClient(&g_bsdSessionMgr, slot);

    int ret = -1;
    int errno_ = -1;
    void* out_ptr = NULL;
    if (R_SUCCEEDED(rc)) {
        // This is only correct if extra outputs need 32-bit alignment or more
        // So far no BSD commands with extra outputs smaller than 32-bit have been observed
        struct {
            int ret;
            int errno_;
        } *out = NULL;

        rc = serviceParseResponse(&srv,
            sizeof(*out)+out_data_size, (void**)&out,
            disp.out_num_objects, disp.out_objects,
            disp.out_handle_attrs, disp.out_handles);

        if (R_SUCCEEDED(rc)) {
            ret = out->ret;
            errno_ = ret < 0 ? out->errno_ : 0;
            if (errno_ == 0)
                out_ptr = out+1;
        }
    }

    if (out_ptr && out_data && out_data_size)
        __builtin_memcpy(out_data, out_ptr, out_data_size);

    g_bsdResult = rc;
    g_bsdErrno = errno_;
    return ret;
}

#define _bsdDispatch(_rid,...) \
    _bsdDispatchImpl((_rid),NULL,0,NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _bsdDispatchIn(_rid,_in,...) \
    _bsdDispatchImpl((_rid),&(_in),sizeof(_in),NULL,0,(SfDispatchParams){ __VA_ARGS__ })

#define _bsdDispatchOut(_rid,_out,...) \
    _bsdDispatchImpl((_rid),NULL,0,&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

#define _bsdDispatchInOut(_rid,_in,_out,...) \
    _bsdDispatchImpl((_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfDispatchParams){ __VA_ARGS__ })

static int _bsdCmdInSockfdOutSockaddr(int sockfd, struct sockaddr *addr, socklen_t *addrlen, u32 cmd_id) {
    socklen_t maxaddrlen = addrlen ? *addrlen : 0;

    return _bsdDispatchInOut(cmd_id, sockfd, *addrlen,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { addr, maxaddrlen } },
    );
}

static int _bsdCmdInSockfdSockaddr(int sockfd, const struct sockaddr *addr, socklen_t addrlen, u32 cmd_id) {
    return _bsdDispatchIn(cmd_id, sockfd,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { addr, addrlen } },
    );
}

static int _bsdCmdInDomainTypeProtocol(int domain, int type, int protocol, u32 cmd_id) {
    const struct {
        int domain;
        int type;
        int protocol;
    } in = { domain, type, protocol };

    return _bsdDispatchIn(cmd_id, in);
}

const BsdInitConfig *bsdGetDefaultInitConfig(void) {
    return &g_defaultBsdInitConfig;
}

Result _bsdInitialize(const BsdInitConfig *config, u32 num_sessions, u32 service_type) {
    if (!config)
        config = &g_defaultBsdInitConfig;

    SmServiceName bsd_srv = {0};
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (service_type & BIT(1)) {
        bsd_srv = smEncodeName("bsd:s");
        rc = smGetServiceWrapper(&g_bsdSrv, bsd_srv);
    }

    if (R_FAILED(rc) && (service_type & BIT(0))) {
        bsd_srv = smEncodeName("bsd:u");
        rc = smGetServiceWrapper(&g_bsdSrv, bsd_srv);
    }

    if (R_SUCCEEDED(rc))
        rc = smGetServiceWrapper(&g_bsdMonitor, bsd_srv);

    if (R_SUCCEEDED(rc))
        rc = tmemCreate(&g_bsdTmem, _bsdGetTransferMemSizeForConfig(config), 0);

    if (R_SUCCEEDED(rc))
        rc = _bsdRegisterClient(&g_bsdTmem, config, &g_bsdClientPid);

    if (R_SUCCEEDED(rc))
        rc = _bsdStartMonitoring(g_bsdClientPid);

    if (R_SUCCEEDED(rc))
        rc = sessionmgrCreate(&g_bsdSessionMgr, g_bsdSrv.session, num_sessions);

    return rc;
}

void _bsdCleanup(void) {
    g_bsdClientPid = 0;
    sessionmgrClose(&g_bsdSessionMgr);
    serviceClose(&g_bsdMonitor);
    serviceClose(&g_bsdSrv);
    tmemClose(&g_bsdTmem);
}

Service* bsdGetServiceSession(void) {
    return &g_bsdSrv;
}

int bsdSocket(int domain, int type, int protocol) {
    return _bsdCmdInDomainTypeProtocol(domain, type, protocol, 2);
}

int bsdSocketExempt(int domain, int type, int protocol) {
    return _bsdCmdInDomainTypeProtocol(domain, type, protocol, 3);
}

int bsdOpen(const char *pathname, int flags) {
    return _bsdDispatchIn(4, flags,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { pathname, strlen(pathname) + 1 } },
    );
}

int bsdSelect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    size_t readfds_size   = readfds   ? sizeof(fd_set) : 0;
    size_t writefds_size  = writefds  ? sizeof(fd_set) : 0;
    size_t exceptfds_size = exceptfds ? sizeof(fd_set) : 0;

    const struct {
        int nfds;
        BsdSelectTimeval timeout;
    } in = { nfds, _bsdCreateSelectTimeval(timeout) };

    return _bsdDispatchIn(5, in,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { readfds,   readfds_size   },
            { writefds,  writefds_size  },
            { exceptfds, exceptfds_size },
            { readfds,   readfds_size   },
            { writefds,  writefds_size  },
            { exceptfds, exceptfds_size },
        },
    );
}

int bsdPoll(struct pollfd *fds, nfds_t nfds, int timeout) {
    size_t fds_size = nfds * sizeof(struct pollfd);

    const struct {
        nfds_t nfds;
        int timeout;
    } in = { nfds, timeout };

    return _bsdDispatchIn(6, in,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { fds, fds_size },
            { fds, fds_size },
        },
    );
}

int bsdSysctl(const int *name, unsigned int namelen, void *oldp, size_t *oldlenp, const void *newp, size_t newlen) {
    size_t inlen = oldlenp ? *oldlenp : 0;

    return _bsdDispatchOut(7, *oldlenp,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { name, 4*namelen },
            { newp, newlen    },
            { oldp, inlen     },
        },
    );
}

ssize_t bsdRecv(int sockfd, void *buf, size_t len, int flags) {
    const struct {
        int sockfd;
        int flags;
    } in = { sockfd, flags };

    return _bsdDispatchIn(8, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buf, len } },
    );
}

ssize_t bsdRecvFrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    socklen_t inaddrlen = addrlen ? *addrlen : 0;

    const struct {
        int sockfd;
        int flags;
    } in = { sockfd, flags };

    return _bsdDispatchInOut(9, in, *addrlen,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { buf,      len       },
            { src_addr, inaddrlen },
        },
    );
}

ssize_t bsdSend(int sockfd, const void* buf, size_t len, int flags) {
    const struct {
        int sockfd;
        int flags;
    } in = { sockfd, flags };

    return _bsdDispatchIn(10, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buf, len } },
    );
}

ssize_t bsdSendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    const struct {
        int sockfd;
        int flags;
    } in = { sockfd, flags };

    return _bsdDispatchIn(11, in,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
        },
        .buffers = {
            { buf,       len     },
            { dest_addr, addrlen },
        },
    );
}

int bsdAccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return _bsdCmdInSockfdOutSockaddr(sockfd, addr, addrlen, 12);
}

int bsdBind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return _bsdCmdInSockfdSockaddr(sockfd, addr, addrlen, 13);
}

int bsdConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return _bsdCmdInSockfdSockaddr(sockfd, addr, addrlen, 14);
}

int bsdGetPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return _bsdCmdInSockfdOutSockaddr(sockfd, addr, addrlen, 15);
}

int bsdGetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return _bsdCmdInSockfdOutSockaddr(sockfd, addr, addrlen, 16);
}

int bsdGetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    socklen_t inoptlen = optlen ? *optlen : 0;

    const struct {
        int sockfd;
        int level;
        int optname;
    } in = { sockfd, level, optname };

    return _bsdDispatchInOut(17, in, *optlen,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { optval, inoptlen } },
    );
}

int bsdListen(int sockfd, int backlog) {
    struct {
        int sockfd;
        int backlog;
    } in = { sockfd, backlog };

    return _bsdDispatchIn(18, in);
}

int bsdIoctl(int fd, int request, void *data) {
    const void *in1 = NULL, *in2 = NULL, *in3 = NULL, *in4 = NULL;
    size_t in1sz = 0, in2sz = 0, in3sz = 0, in4sz = 0;

    void *out1 = NULL, *out2 = NULL, *out3 = NULL, *out4 = NULL;
    size_t out1sz = 0, out2sz = 0, out3sz = 0, out4sz = 0;

    int bufcount = 1;

    switch(request) {
        case SIOCGIFCONF: {
            struct ifconf *data_ = (struct ifconf *)data;
            in1 = out1 = data;
            in1sz = out1sz = sizeof(struct ifconf);
            in2 = out2 = data_->ifc_req;
            in2sz = out2sz = data_->ifc_len;
            bufcount = 2;
            break;
        }
        case SIOCGIFMEDIA:
        case SIOCGIFXMEDIA: {
            struct ifmediareq *data_ = (struct ifmediareq *)data;
            in1 = out1 = data;
            in1sz = out1sz = sizeof(struct ifmediareq);
            in2 = out2 = data_->ifm_ulist;
            in2sz = out2sz = 8 * data_->ifm_count;
            bufcount = 2;
            break;
        }
        // Generic ioctl
        default: {
            void *data_ = NULL;
            if(request & IOC_INOUT)
                data_ = data;
            if(request & IOC_IN) {
                in1 = data_;
                in1sz = IOCPARM_LEN(request);
            }
            if(request & IOC_OUT) {
                out1 = data_;
                out1sz = IOCPARM_LEN(request);
            }
            break;
        }
    }

    const struct {
        int fd;
        int request;
        int bufcount;
    } in = { fd, request, bufcount };

    return _bsdDispatchIn(19, in,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { in1,  in1sz  },
            { in2,  in2sz  },
            { in3,  in3sz  },
            { in4,  in4sz  },
            { out1, out1sz },
            { out2, out2sz },
            { out3, out3sz },
            { out4, out4sz },
        },
    );
}

int bsdFcntl(int fd, int cmd, int flags) {
    if(cmd != F_GETFL && cmd != F_SETFL) {
        g_bsdResult = 0;
        g_bsdErrno = EOPNOTSUPP;
        return -1;
    }

    if(cmd == F_GETFL)
        flags = 0;

    const struct {
        int fd;
        int cmd;
        int flags;
    } in = { fd, cmd, flags };

    return _bsdDispatchIn(20, in);
}

int bsdSetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    const struct {
        int sockfd;
        int level;
        int optname;
    } in = { sockfd, level, optname };

    return _bsdDispatchIn(21, in,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { optval, optlen } },
    );
}

int bsdShutdown(int sockfd, int how) {
    const struct {
        int sockfd;
        int how;
    } in = { sockfd, how };

    return _bsdDispatchIn(22, in);
}

int bsdShutdownAllSockets(int how) {
    return _bsdDispatchIn(23, how);
}

ssize_t bsdWrite(int fd, const void *buf, size_t count) {
    return _bsdDispatchIn(24, fd,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { buf, count } },
    );
}

ssize_t bsdRead(int fd, void *buf, size_t count) {
    return _bsdDispatchIn(25, fd,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { buf, count } },
    );
}

int bsdClose(int fd) {
    return _bsdDispatchIn(26, fd);
}

int bsdDuplicateSocket(int sockfd) {
    const struct {
        int sockfd;
        u32 _padding;
        u64 reserved;
    } in = { sockfd, 0, 0 };

    return _bsdDispatchIn(27, in);
}
