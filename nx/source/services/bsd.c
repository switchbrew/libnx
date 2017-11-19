// Copyright 2017 plutoo
#include <switch.h>

static Handle g_bsdClientHandle = -1;
static Handle g_bsdMonitorHandle = -1;
static u64 g_bsdClientPid = -1;
static int g_Errno = 0;

#define EPIPE 32

static Result _bsdRegisterClient(Handle h, TransferMemory* tmem, u64* pid_out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);
    ipcSendHandleCopy(&c, tmem->MemHandle);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 unk0[5];
        u64 tmem_sz;
        u64 pad[2];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->unk0[0] = 1;
    raw->unk0[1] = 0x10000;
    raw->unk0[2] = 0x40000;
    raw->unk0[3] = 0xA500;
    raw->unk0[4] = 13;
    raw->tmem_sz = tmem->Size;

    Result rc = ipcDispatch(h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

static Result _bsdStartMonitor(Handle h, u64 pid) {
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

    Result rc = ipcDispatch(h);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result bsdInitialize(TransferMemory* tmem) {
    const char* bsd_srv = "bsd:s";
    Result rc = smGetService(&g_bsdClientHandle, bsd_srv);

    if (R_FAILED(rc)) {
        bsd_srv = "bsd:u";
        rc = smGetService(&g_bsdClientHandle, bsd_srv);
    }

    if (R_SUCCEEDED(rc)) {
        rc = smGetService(&g_bsdMonitorHandle, bsd_srv);

        if (R_SUCCEEDED(rc)) {
            rc = _bsdRegisterClient(g_bsdClientHandle, tmem, &g_bsdClientPid);

            if (R_SUCCEEDED(rc)) {
                rc = _bsdStartMonitor(g_bsdMonitorHandle, g_bsdClientPid);
            }
        }
    }

    return rc;
}

int bsdGetErrno() {
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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int fd = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

int bsdSend(int sockfd, void* buffer, size_t length, int flags) {
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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

int bsdSendTo(int sockfd, void* buffer, size_t length, int flags, const struct bsd_sockaddr_in *dest_addr, size_t dest_len) {
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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

int bsdConnect(int sockfd, void* addr, u32 addrlen) {
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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int fd = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

int bsdBind(int sockfd, void* addr, u32 addrlen) {
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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

int bsdWrite(int sockfd, void* buffer, size_t length) {
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

    Result rc = ipcDispatch(g_bsdClientHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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
