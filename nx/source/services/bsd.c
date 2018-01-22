// Copyright 2017 plutoo
#include <switch/types.h>
#include <switch/result.h>
#include <switch/ipc.h>
#include <switch/services/bsd.h>
#include <switch/services/sm.h>
#include <switch/kernel/shmem.h>
#include <switch/kernel/rwlock.h>

static Service g_bsdSrv;
static Service g_bsdMonitor;
static u64 g_bsdClientPid = -1;
static int g_Errno = 0;

static const BsdConfig g_defaultBsdConfig = {
    .version = 2,

    .tcp_tx_buf_size        = 0x8000,
    .tcp_rx_buf_size        = 0x10000,
    .tcp_tx_buf_max_size    = 0x40000,
    .tcp_rx_buf_max_size    = 0x40000,

    .udp_tx_buf_size = 0x2400,
    .udp_rx_buf_size = 0xA500,

    .sb_efficiency = 4,
};

#define EPIPE 32

static Result _bsdRegisterClient(Service* srv, TransferMemory* tmem, const BsdConfig *config, u64* pid_out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);
    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        BsdConfig config;
        u64 pid_reserved;
        u64 tmem_sz;
        u64 pad[2];
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
        rc = resp->result;
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

        rc = resp->result;
    }

    return rc;
}

const BsdConfig *bsdGetDefaultConfig(void) {
    return &g_defaultBsdConfig;
}

Result bsdInitialize(TransferMemory* tmem, const BsdConfig *config) {
    const char* bsd_srv = "bsd:s";
    Result rc = smGetService(&g_bsdSrv, bsd_srv);

    if (R_FAILED(rc)) {
        bsd_srv = "bsd:u";
        rc = smGetService(&g_bsdSrv, bsd_srv);
    }

    if (R_SUCCEEDED(rc)) {
        rc = smGetService(&g_bsdMonitor, bsd_srv);

        if (R_SUCCEEDED(rc)) {
            rc = _bsdRegisterClient(&g_bsdSrv, tmem, config, &g_bsdClientPid);

            if (R_SUCCEEDED(rc)) {
                rc = _bsdStartMonitor(&g_bsdMonitor, g_bsdClientPid);
            }
        }
    }

    return rc;
}

int bsdGetErrno(void) {
    return g_Errno;
}

int bsdSocket(int domain, int type, int protocol) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 domain;
        u32 type;
        u32 protocol;
        u32 pad[4];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->domain = domain;
    raw->type = type;
    raw->protocol = protocol;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int fd = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 fd;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            fd = resp->fd;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return fd;
}
int bsdRecv(int sockfd, void* buffer, size_t length, int flags) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, length, 0);
    ipcAddRecvStatic(&c, buffer, length, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
        u32 flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->sockfd = sockfd;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = rc; //EPIPE;
    }

    return ret;
}

int bsdSend(int sockfd, const void* buffer, size_t length, int flags) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buffer, length, 0);
    ipcAddSendStatic(&c, buffer, length, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
        u32 flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->sockfd = sockfd;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return ret;
}

int bsdSendTo(int sockfd, const void* buffer, size_t length, int flags, const struct bsd_sockaddr_in *dest_addr, size_t dest_len) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buffer, length, 0);
    ipcAddSendStatic(&c, buffer, length, 0);

    ipcAddSendBuffer(&c, dest_addr, dest_len, 0);
    ipcAddSendStatic(&c, dest_addr, dest_len, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
        u32 flags;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;
    raw->sockfd = sockfd;
    raw->flags = flags;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return ret;
}

int bsdConnect(int sockfd, const void* addr, u32 addrlen) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, addr, addrlen, 0);
    ipcAddSendStatic(&c, addr, addrlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
        u32 pad[4];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    raw->sockfd = sockfd;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int fd = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 fd;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            fd = resp->fd;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return fd;
}

int bsdBind(int sockfd, const void* addr, u32 addrlen) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, addr, addrlen, 0);
    ipcAddSendStatic(&c, addr, addrlen, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 pad[5];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return ret;
}

int bsdListen(int sockfd, int backlog) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
        u32 backlog;
        u32 pad[4];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;
    raw->sockfd = sockfd;
    raw->backlog = backlog;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return ret;
}

int bsdSetSockOpt(int sockfd, int level, int option_name, const void *option_value, size_t option_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, option_value, option_size, 0);
    ipcAddSendStatic(&c, option_value, option_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
        u32 level;
        u32 option_name;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;
    raw->sockfd = sockfd;
    raw->level = level;
    raw->option_name = option_name;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return ret;
}

int bsdWrite(int sockfd, const void* buffer, size_t length) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buffer, length, 0);
    ipcAddSendStatic(&c, buffer, length, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 sockfd;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;
    raw->sockfd = sockfd;

    Result rc = serviceIpcDispatch(&g_bsdSrv);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 ret;
            u32 errno;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            g_Errno = resp->errno;
            ret = resp->ret;
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
    }

    return ret;
}
