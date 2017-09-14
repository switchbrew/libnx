// Copyright 2017 plutoo
#include <switch.h>

static Handle g_bsdHandle = -1;
static u32 g_bsdObjectId = -1;
static int g_Errno = 0;

#define EPIPE 32

Result bsdInitialize(TransferMemory* tmem) {
    Result rc = smGetService(&g_bsdHandle, "bsd:s");

    if (R_FAILED(rc)) {
        rc = smGetService(&g_bsdHandle, "bsd:u");
    }

    if (R_SUCCEEDED(rc)) {
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
        raw->unk0[0] = 0x4000;
        raw->unk0[1] = 0x4000;
        raw->unk0[2] = 0x4000;
        raw->unk0[3] = 0x4000;
        raw->unk0[4] = 0;
        raw->tmem_sz = tmem->Size;

        rc = ipcDispatch(g_bsdHandle);

        if (R_SUCCEEDED(rc)) {
            IpcCommandResponse r;
            ipcParseResponse(&r);

            struct {
                u64 magic;
                u64 result;
            } *resp = r.Raw;

            rc = resp->result;

            if (R_SUCCEEDED(rc)) {
                rc = ipcConvertSessionToDomain(g_bsdHandle, &g_bsdObjectId);
            }
        }
    }

    if (R_FAILED(rc)) {
        g_Errno = EPIPE;
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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->domain = domain;
    raw->type = type;
    raw->protocol = protocol;

    Result rc = ipcDispatch(g_bsdHandle);
    int fd = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->sockfd = sockfd;
    raw->flags = flags;

    Result rc = ipcDispatch(g_bsdHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->sockfd = sockfd;
    raw->flags = flags;

    Result rc = ipcDispatch(g_bsdHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    raw->sockfd = sockfd;

    Result rc = ipcDispatch(g_bsdHandle);
    int fd = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;

    Result rc = ipcDispatch(g_bsdHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;
    raw->sockfd = sockfd;
    raw->backlog = backlog;

    Result rc = ipcDispatch(g_bsdHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), g_bsdObjectId);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;
    raw->sockfd = sockfd;

    Result rc = ipcDispatch(g_bsdHandle);
    int ret = -1;

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponseForDomain(&r);

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
