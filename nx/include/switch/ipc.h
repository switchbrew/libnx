// Copyright 2017 plutoo
#define SFCI_MAGIC 0x49434653
#define SFCO_MAGIC 0x4f434653

typedef struct {
    size_t NumSend; // A
    size_t NumRecv; // B
    size_t NumTransfer; // W
    void*  Buffers[4];
    size_t BufferSizes[4];
    u8     Flags[4];

    size_t NumStaticIn;  // X
    size_t NumStaticOut; // C
    void*  Statics[4];
    size_t StaticSizes[4];
    u8     Indices[4];

    bool   SendPid;
    size_t NumHandlesCopy;
    size_t NumHandlesMove;
    Handle Handles[8];
} IpcCommand;

static inline void ipcInitialize(IpcCommand* cmd) {
    cmd->NumSend = 0;
    cmd->NumRecv = 0;
    cmd->NumTransfer = 0;

    cmd->NumStaticIn  = 0;
    cmd->NumStaticOut = 0;

    cmd->SendPid = false;
    cmd->NumHandlesCopy = 0;
    cmd->NumHandlesMove = 0;
}

typedef struct { // todo: Make sure sizeof isn't 16 bytes!
    u32 Size;
    u32 Addr;
    u32 Packed;
} IpcBufferDescriptor;

typedef struct {
    u32 Packed;
    u32 Addr;
} IpcStaticSendDescriptor;

static inline void ipcAddSendBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 flags) {
    size_t off = cmd->NumSend;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumSend++;
}

static inline void ipcAddRecvBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 flags) {
    size_t off = cmd->NumSend + cmd->NumRecv;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumRecv++;
}

static inline void ipcAddTransferBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 flags) {
    size_t off = cmd->NumSend + cmd->NumRecv + cmd->NumTransfer;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumTransfer++;
}

static inline void ipcAddSendStatic(IpcCommand* cmd, void* buffer, size_t size, u8 index) {
    size_t off = cmd->NumStaticIn;
    cmd->Statics[off] = buffer;
    cmd->StaticSizes[off] = size;
    cmd->Indices[off] = index;
    cmd->NumStaticIn++;
}

static inline void ipcAddRecvStatic(IpcCommand* cmd, void* buffer, size_t size, u8 index) {
    size_t off = cmd->NumStaticIn + cmd->NumStaticOut;
    cmd->Statics[off] = buffer;
    cmd->StaticSizes[off] = size;
    cmd->Indices[off] = index;
    cmd->NumStaticOut++;
}

static inline void ipcSendPid(IpcCommand* cmd) {
    cmd->SendPid = true;
}

static inline void ipcSendHandleCopy(IpcCommand* cmd, Handle h) {
    cmd->Handles[cmd->NumHandlesCopy++] = h;
}

static inline void ipcSendHandleMove(IpcCommand* cmd, Handle h) {
    cmd->Handles[cmd->NumHandlesCopy + cmd->NumHandlesMove++] = h;
}

static inline void* ipcPrepareHeader(IpcCommand* cmd, size_t sizeof_raw) {
    u32* buf = armGetTls();
    *buf++ = 4 | (cmd->NumStaticIn << 16) | (cmd->NumSend << 20) | (cmd->NumRecv << 24) | (cmd->NumTransfer << 28);

    if (cmd->SendPid || cmd->NumHandlesCopy > 0 || cmd->NumHandlesMove > 0) {
        *buf++ = (sizeof_raw/4) | 0x80000000;
        *buf++ = (!!cmd->SendPid) | (cmd->NumHandlesCopy << 1) | (cmd->NumHandlesMove << 1);

        if (cmd->SendPid)
            buf += 2;

        buf += cmd->NumHandlesCopy;
        buf += cmd->NumHandlesMove;
    }
    else {
        *buf++ = sizeof_raw/4;
    }

    size_t i;
    for (i=0; i<cmd->NumStaticIn; i++, buf+=2) {
        IpcStaticSendDescriptor* desc = (IpcStaticSendDescriptor*) buf;
        uintptr_t ptr = (uintptr_t) cmd->Statics[i];
        desc->Addr = ptr;
        desc->Packed = cmd->Indices[i] | (cmd->StaticSizes[i] << 16) |
            (((ptr >> 32) & 15) << 12) | (((ptr >> 36) & 15) << 6);
    }

    for (i=0; i<(cmd->NumSend + cmd->NumRecv + cmd->NumTransfer); i++, buf+=3) {
        IpcBufferDescriptor* desc = (IpcBufferDescriptor*) buf;
        desc->Size = cmd->BufferSizes[i];

        uintptr_t ptr = (uintptr_t) cmd->Buffers[i];
        desc->Addr = ptr;
        desc->Packed = cmd->Flags[i] |
            (((ptr >> 32) & 15) << 28) | ((ptr >> 36) << 2);
    }

    // todo: More
    return (void*) ((((uintptr_t)buf) + 15) &~ 15);
}

static inline Result ipcDispatch(Handle session) {
    return svcSendSyncRequest(session);
}

typedef struct {
    bool HasPid;
    u64  Pid;

    size_t NumHandles;
    Handle Handles[8];

    size_t NumBuffers;
    void*  Buffers[4];
    size_t BufferSizes[4];

    void*  Raw;
    size_t RawSize;
} IpcCommandResponse;

static inline Result ipcParseResponse(IpcCommandResponse* r) {
    u32* buf = armGetTls();
    u32 ctrl0 = *buf++;
    u32 ctrl1 = *buf++;
    size_t i;

    r->HasPid = false;
    r->RawSize = (ctrl1 & 0x1ff) * 4;
    r->NumHandles = 0;

    if (ctrl1 & 0x80000000) {
        u32 ctrl2 = *buf++;

        if (ctrl2 & 1) {
            r->HasPid = true;
            r->Pid = *buf++;
            r->Pid |= ((u64)(*buf++)) << 32;
        }

        size_t num_handles = ((ctrl2 >> 1) & 15) + ((ctrl2 >> 5) & 15);
        buf += num_handles;

        if (num_handles > 8)
            num_handles = 8;

        for (i=0; i<num_handles; i++)
            r->Handles[i] = *(buf-i);

        r->NumHandles = num_handles;
    }

    size_t num_statics = (ctrl0 >> 16) & 15;
    buf += num_statics*2;

    size_t num_bufs = ((ctrl0 >> 20) & 15) + ((ctrl0 >> 24) & 15) + (((ctrl0 >> 28) & 15));
    r->Raw = (void*)(((uintptr_t)(buf + num_bufs*3) + 15) &~ 15);

    if (num_bufs > 4)
        num_bufs = 4;

    for (i=0; i<num_bufs; i++, buf+=3) {
        IpcBufferDescriptor* desc = (IpcBufferDescriptor*) buf;
        u64 packed = (u64) desc->Packed;

        r->Buffers[i] = (void*) (desc->Addr | ((packed >> 28) << 32) | (((packed >> 2) & 15) << 36));
        r->BufferSizes[i] = desc->Size;
        // todo: Do we care about buffer type?
    }

    r->NumBuffers = num_bufs;
    return 0;
}
