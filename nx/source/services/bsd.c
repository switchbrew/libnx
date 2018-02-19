// Copyright 2017 plutoo
#include <errno.h>
#include <string.h>

// Complete definition of struct timeout:
#include <sys/time.h>

#include <fcntl.h>

// For ioctls:
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_media.h>

#include "types.h"
#include "result.h"
#include "ipc.h"
#include "services/bsd.h"
#include "services/sm.h"
#include "kernel/shmem.h"
#include "kernel/rwlock.h"

__thread Result g_bsdResult;
__thread int g_bsdErrno;

static Service g_bsdSrv;
static Service g_bsdMonitor;
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

/*
    This function computes the minimal size of the transfer memory to be passed to @ref bsdInitalize.
    Should the transfer memory be smaller than that, the BSD sockets service would only send
    ZeroWindow packets (for TCP), resulting in a transfer rate not exceeding 1 byte/s.
*/
static size_t _bsdGetTransferMemSizeForConfig(const BsdInitConfig *config) {
    u32 tcp_tx_buf_max_size = config->tcp_tx_buf_max_size != 0 ? config->tcp_tx_buf_max_size : config->tcp_tx_buf_size;
    u32 tcp_rx_buf_max_size = config->tcp_rx_buf_max_size != 0 ? config->tcp_rx_buf_max_size : config->tcp_rx_buf_size;
    u32 sum = tcp_tx_buf_max_size + tcp_rx_buf_max_size + config->udp_tx_buf_size + config->udp_rx_buf_size;

    sum = ((sum + 0xFFF) >> 12) << 12; // page round-up
    return (size_t)(config->sb_efficiency * sum);
}

static Result _bsdRegisterClient(Service* srv, TransferMemory* tmem, const BsdInitConfig *config, u64* pid_out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);
    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        BsdInitConfig config;
        u64 pid_reserved;
        u64 tmem_sz;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->config = *config;
    raw->pid_reserved = 0;
    raw->tmem_sz = tmem->size;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 pid;
        } *resp = r.Raw;

        *pid_out = resp->pid;
        g_bsdResult = rc = resp->result;
        g_bsdErrno = 0;
    }

    return rc;
}

static Result _bsdStartMonitor(Service* srv, u64 pid) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        g_bsdResult = rc = resp->result;
        g_bsdErrno = 0;
    }

    return rc;
}

typedef struct  {
    u64 magic;
    u64 result;
    int ret;
    int errno_;
} BsdIpcResponseBase;

static int _bsdDispatchBasicCommand(IpcCommand *c, IpcParsedCommand *rOut) {
    Result rc = serviceIpcDispatch(&g_bsdSrv);
    IpcParsedCommand r;
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        ipcParse(&r);

        BsdIpcResponseBase *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            ret = resp->ret;
            g_bsdErrno = (ret < 0) ? resp->errno_ : 0;
        }
    }

    if (R_FAILED(rc))
        g_bsdErrno = -1;

    if(rOut != NULL)
        *rOut = r;

    g_bsdResult = rc;
    return ret;
}

static int _bsdDispatchCommandWithOutAddrlen(IpcCommand *c, socklen_t *addrlen) {
    IpcParsedCommand r;
    int ret = _bsdDispatchBasicCommand(c, &r);
    if(ret != -1 && addrlen != NULL) {
        struct {
            BsdIpcResponseBase bsd_resp;
            socklen_t addrlen;
        } *resp = r.Raw;
        *addrlen = resp->addrlen;
    }
    return ret;
}

static int _bsdNameGetterCommand(u32 cmd_id, int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    IpcCommand c;
    ipcInitialize(&c);

    socklen_t maxaddrlen = addrlen == NULL ? 0 : *addrlen;

    ipcAddRecvBuffer(&c, addr, maxaddrlen, 0);
    ipcAddRecvStatic(&c, addr, maxaddrlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->sockfd = sockfd;

    return _bsdDispatchCommandWithOutAddrlen(&c, addrlen);
}

static int _bsdSocketCreationCommand(u32 cmd_id, int domain, int type, int protocol) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int domain;
        int type;
        int protocol;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->domain = domain;
    raw->type = type;
    raw->protocol = protocol;

    return _bsdDispatchBasicCommand(&c, NULL);
}

const BsdInitConfig *bsdGetDefaultInitConfig(void) {
    return &g_defaultBsdInitConfig;
}

Result bsdInitialize(const BsdInitConfig *config) {
    const char* bsd_srv = "bsd:s";

    if(serviceIsActive(&g_bsdSrv) || serviceIsActive(&g_bsdMonitor))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    Result rc = smGetService(&g_bsdSrv, bsd_srv);

    if (R_FAILED(rc)) {
        bsd_srv = "bsd:u";
        rc = smGetService(&g_bsdSrv, bsd_srv);
    }
    if(R_FAILED(rc)) goto error;

    rc = smGetService(&g_bsdMonitor, bsd_srv);
    if(R_FAILED(rc)) goto error;

    rc = tmemCreate(&g_bsdTmem, _bsdGetTransferMemSizeForConfig(config), 0);
    if(R_FAILED(rc)) goto error;

    rc = _bsdRegisterClient(&g_bsdSrv, &g_bsdTmem, config, &g_bsdClientPid);
    if(R_FAILED(rc)) goto error;

    rc = _bsdStartMonitor(&g_bsdMonitor, g_bsdClientPid);
    if(R_FAILED(rc)) goto error;

    return rc;

error:
    bsdExit();
    return rc;
}

void bsdExit(void) {
    serviceClose(&g_bsdMonitor);
    serviceClose(&g_bsdSrv);
    tmemClose(&g_bsdTmem);
}

int bsdSocket(int domain, int type, int protocol) {
    return _bsdSocketCreationCommand(2, domain, type, protocol);
}

int bsdSocketExempt(int domain, int type, int protocol) {
    return _bsdSocketCreationCommand(3, domain, type, protocol);
}

int bsdOpen(const char *pathname, int flags) {
    IpcCommand c;
    ipcInitialize(&c);

    size_t pathlen = strlen(pathname) + 1;
    ipcAddSendBuffer(&c, pathname, pathlen, 0);
    ipcAddSendStatic(&c, pathname, pathlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->flags = flags;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdSelect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, readfds, readfds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddSendStatic(&c, readfds, readfds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddSendBuffer(&c, writefds, writefds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddSendStatic(&c, writefds, writefds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddSendBuffer(&c, exceptfds, exceptfds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddSendStatic(&c, exceptfds, exceptfds == NULL ? 0 : sizeof(fd_set), 0);

    ipcAddRecvBuffer(&c, readfds, readfds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddRecvStatic(&c, readfds, readfds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddRecvBuffer(&c, writefds, writefds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddRecvStatic(&c, writefds, writefds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddRecvBuffer(&c, exceptfds, exceptfds == NULL ? 0 : sizeof(fd_set), 0);
    ipcAddRecvStatic(&c, exceptfds, exceptfds == NULL ? 0 : sizeof(fd_set), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int nfds;
        struct timeval timeout;
        bool nullTimeout;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->nfds = nfds;
    if(!(raw->nullTimeout = timeout == NULL))
        raw->timeout = *timeout;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdPoll(struct pollfd *fds, nfds_t nfds, int timeout) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, fds, nfds * sizeof(struct pollfd), 0);
    ipcAddSendStatic(&c, fds, nfds * sizeof(struct pollfd), 0);

    ipcAddRecvBuffer(&c, fds, nfds * sizeof(struct pollfd), 0);
    ipcAddRecvStatic(&c, fds, nfds * sizeof(struct pollfd), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        nfds_t nfds;
        int timeout;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;
    raw->nfds = nfds;
    raw->timeout = timeout;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdSysctl(const int *name, unsigned int namelen, void *oldp, size_t *oldlenp, const void *newp, size_t newlen) {
    IpcCommand c;
    size_t inlen = oldlenp == NULL ? 0 : *oldlenp;

    ipcInitialize(&c);

    ipcAddSendBuffer(&c, name, 4 * namelen, 0);
    ipcAddSendStatic(&c, name, 4 * namelen, 0);
    ipcAddSendBuffer(&c, newp, newlen, 0);
    ipcAddSendStatic(&c, newp, newlen, 0);

    ipcAddRecvBuffer(&c, oldp, inlen, 0);
    ipcAddRecvStatic(&c, oldp, inlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    IpcParsedCommand r;
    int ret = _bsdDispatchBasicCommand(&c, &r);
    if(ret != -1 && oldlenp != NULL) {
        struct {
            BsdIpcResponseBase bsd_resp;
            size_t oldlenp;
        } *resp = r.Raw;
        *oldlenp = resp->oldlenp;
    }
    return ret;
}

ssize_t bsdRecv(int sockfd, void *buf, size_t len, int flags) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, len, 0);
    ipcAddRecvStatic(&c, buf, len, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->sockfd = sockfd;
    raw->flags = flags;

    return _bsdDispatchBasicCommand(&c, NULL);
}

ssize_t bsdRecvFrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    IpcCommand c;
    socklen_t inaddrlen = addrlen == NULL ? 0 : *addrlen;

    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, buf, len, 0);
    ipcAddRecvStatic(&c, buf, len, 0);
    ipcAddRecvBuffer(&c, src_addr, inaddrlen, 0);
    ipcAddRecvBuffer(&c, src_addr, inaddrlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->sockfd = sockfd;
    raw->flags = flags;

    return _bsdDispatchCommandWithOutAddrlen(&c, addrlen);
}

ssize_t bsdSend(int sockfd, const void* buf, size_t len, int flags) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, 0);
    ipcAddSendStatic(&c, buf, len, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->sockfd = sockfd;
    raw->flags = flags;

    return _bsdDispatchBasicCommand(&c, NULL);
}

ssize_t bsdSendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, len, 0);
    ipcAddSendStatic(&c, buf, len, 0);

    ipcAddSendBuffer(&c, dest_addr, addrlen, 1);
    ipcAddSendStatic(&c, dest_addr, addrlen, 1);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->sockfd = sockfd;
    raw->flags = flags;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdAccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return _bsdNameGetterCommand(12, sockfd, addr, addrlen);
}

int bsdBind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, addr, addrlen, 0);
    ipcAddSendStatic(&c, addr, addrlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;
    raw->sockfd = sockfd;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, addr, addrlen, 0);
    ipcAddSendStatic(&c, addr, addrlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    raw->sockfd = sockfd;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdGetPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return _bsdNameGetterCommand(15, sockfd, addr, addrlen);
}

int bsdGetSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return _bsdNameGetterCommand(16, sockfd, addr, addrlen);
}

int bsdGetSockOpt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    IpcCommand c;
    ipcInitialize(&c);

    socklen_t inoptlen = optlen == NULL ? 0 : *optlen;

    ipcAddRecvBuffer(&c, optval, inoptlen, 0);
    ipcAddRecvStatic(&c, optval, inoptlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int level;
        int optname;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 17;
    raw->sockfd = sockfd;
    raw->level = level;
    raw->optname = optname;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdListen(int sockfd, int backlog) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int backlog;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;
    raw->sockfd = sockfd;
    raw->backlog = backlog;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdIoctl(int fd, int request, void *data) {
    IpcCommand c;

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

    ipcInitialize(&c);

    ipcAddSendBuffer(&c, in1, in1sz, 0);
    ipcAddSendStatic(&c, in1, in1sz, 0);
    ipcAddSendBuffer(&c, in2, in2sz, 0);
    ipcAddSendStatic(&c, in2, in2sz, 0);
    ipcAddSendBuffer(&c, in3, in3sz, 0);
    ipcAddSendStatic(&c, in3, in3sz, 0);
    ipcAddSendBuffer(&c, in4, in4sz, 0);
    ipcAddSendStatic(&c, in4, in4sz, 0);

    ipcAddRecvBuffer(&c, out1, out1sz, 0);
    ipcAddRecvStatic(&c, out1, out1sz, 0);
    ipcAddRecvBuffer(&c, out2, out2sz, 0);
    ipcAddRecvStatic(&c, out2, out2sz, 0);
    ipcAddRecvBuffer(&c, out3, out3sz, 0);
    ipcAddRecvStatic(&c, out3, out3sz, 0);
    ipcAddRecvBuffer(&c, out4, out4sz, 0);
    ipcAddRecvStatic(&c, out4, out4sz, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int fd;
        int request;
        int bufcount;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 19;
    raw->fd = fd;
    raw->request = request;
    raw->bufcount = bufcount;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdFcntl(int fd, int cmd, int flags) {
    IpcCommand c;

    if(cmd != F_GETFL && cmd != F_SETFL) {
        g_bsdResult = 0;
        g_bsdErrno = EOPNOTSUPP;
        return -1;
    }

    if(cmd == F_GETFL)
        flags = 0;

    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int fd;
        int cmd;
        int flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;
    raw->fd = fd;
    raw->cmd = cmd;
    raw->flags = flags;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdSetSockOpt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, optval, optlen, 0);
    ipcAddSendStatic(&c, optval, optlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int level;
        int optname;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;
    raw->sockfd = sockfd;
    raw->level = level;
    raw->optname = optname;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdShutdown(int sockfd, int how) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        int how;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 22;
    raw->sockfd = sockfd;
    raw->how = how;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdShutdownAllSockets(int how) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int how;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;
    raw->how = how;

    return _bsdDispatchBasicCommand(&c, NULL);
}

ssize_t bsdWrite(int fd, const void *buf, size_t count) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buf, count, 0);
    ipcAddSendStatic(&c, buf, count, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int fd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;
    raw->fd = fd;

    return _bsdDispatchBasicCommand(&c, NULL);
}

ssize_t bsdRead(int fd, void *buf, size_t count) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buf, count, 0);
    ipcAddRecvStatic(&c, buf, count, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int fd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 25;
    raw->fd = fd;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdClose(int fd) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int fd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 26;
    raw->fd = fd;

    return _bsdDispatchBasicCommand(&c, NULL);
}

int bsdDuplicateSocket(int sockfd) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        int sockfd;
        u64 reserved;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 27;
    raw->sockfd = sockfd;
    raw->reserved = 0;

    return _bsdDispatchBasicCommand(&c, NULL);
}
