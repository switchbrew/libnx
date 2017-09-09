#define SFCI_MAGIC 0x49434653

typedef struct {
    size_t NumSend; // A
    size_t NumRecv; // B
    size_t NumStaticSend; // X
    size_t NumStaticRecv; // C
    void*  Buffers[8];
    size_t Sizes[8];
    u8     Flags[8];

    bool   SendPid;
    size_t NumHandlesCopy;
    size_t NumHandlesMove;
    Handle Handles[8];
} IpcCommand;

static inline void ipcInitialize(IpcCommand* cmd) {
    cmd->NumSend = 0;
    cmd->NumRecv = 0;
    cmd->NumStaticSend = 0;
    cmd->NumStaticRecv = 0;

    cmd->SendPid = false;
    cmd->NumHandles = 0;
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

static inline void ipcAddSendBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 Flags) {
    size_t off = cmd->NumSend;
    cmd->Buffers[off] = buffer;
    cmd->Sizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumSend++;
}

static inline void ipcAddRecvBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 Flags) {
    size_t off = cmd->NumSend + cmd->NumRecv;
    cmd->Buffers[off] = buffer;
    cmd->Sizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumRecv++;
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
    u32* buf = armGetTlsPtr();
    *buf++ = 4 | (cmd->NumSend << 20) | (cmd->NumRecv << 24);

    u32 flag = 0;
    if (cmd->SendPid || cmd->NumHandlesCopy > 0 || cmd->NumHandlesMove > 0) {
        *buf++ = (sizeof_raw/4) | 0x80000000;
        *buf++ = (!!cmd->SendPid) | (cmd->NumHandlesCopy << 1) | (cmd->NumHandlesMove << 1);
    }
    else {
        *buf++ = sizeof_raw/4;
    }

    size_t i;
    for (i=0; i<(cmd->NumSend + cmd->NumRecv); i++, buf+=3) {
        IpcBufferDescriptor* desc = (IpcBufferDescriptor*) buf;
        desc->Size = cmd->Sizes[i];
        desc->Addr = cmd->Buffers[i];
        uintptr_t ptr = (uintptr_t) cmd->Buffers[i]
        desc->Packed =
            (((ptr) >> 36) << 28) |
            (((ptr >> 32) & 15) << 28) |
            cmd->Flags[i];
    }

    // todo: More
    return (void*) ((uintptr_t)buf + 15) &~ 15;
}

static inline Result ipcDispatch(Handle session) {
    return svcSendSyncRequest(session);
}
