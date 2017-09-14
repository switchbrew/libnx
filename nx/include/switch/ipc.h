// Copyright 2017 plutoo
#define SFCI_MAGIC 0x49434653
#define SFCO_MAGIC 0x4f434653

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

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

typedef struct {
    u32 Addr;
    u32 Packed;
} IpcStaticRecvDescriptor;

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
    size_t i;
    *buf++ = 4 | (cmd->NumStaticIn << 16) | (cmd->NumSend << 20) | (cmd->NumRecv << 24) | (cmd->NumTransfer << 28);

    u32* fill_in_size_later = buf;

    if (cmd->NumStaticOut > 0) {
        *buf = (cmd->NumStaticOut + 2) << 10;
    }
    else {
        *buf = 0;
    }

    if (cmd->SendPid || cmd->NumHandlesCopy > 0 || cmd->NumHandlesMove > 0) {
        *buf++ |= 0x80000000;
        *buf++ = (!!cmd->SendPid) | (cmd->NumHandlesCopy << 1) | (cmd->NumHandlesMove << 5);

        if (cmd->SendPid)
            buf += 2;

        for (i=0; i<(cmd->NumHandlesCopy + cmd->NumHandlesMove); i++)
            *buf++ = cmd->Handles[i];
    }
    else {
        buf++;
    }

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

    u32 padding = ((16 - (((uintptr_t) buf) & 15)) & 15) / 4;
    u32* raw = (u32*) (buf + padding);

    size_t raw_size = (sizeof_raw/4) + 4;
    raw_size += ((cmd->NumStaticOut*2) + 3)/4; // todo: these contain u16 lengths for StaticOuts

    buf += raw_size;
    *fill_in_size_later |= raw_size;

    for (i=0; i<cmd->NumStaticOut; i++, buf+=2) {
        IpcStaticRecvDescriptor* desc = (IpcStaticRecvDescriptor*) buf;
        size_t off = cmd->NumStaticIn + i;

        uintptr_t ptr = (uintptr_t) cmd->Statics[off];
        desc->Addr = ptr;
        desc->Packed = (ptr >> 32) | (cmd->StaticSizes[off] << 16);
    }

    return (void*) raw;
}

static inline Result ipcDispatch(Handle session) {
    return svcSendSyncRequest(session);
}

// Response parsing
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
            r->Handles[i] = *(buf-i-1);

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

// Domain shit
static inline Result ipcConvertSessionToDomain(Handle session, u32* object_id_out) {
    u32* buf = armGetTls();

    buf[0] = 5;
    buf[1] = 8;
    buf[4] = SFCI_MAGIC;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;

    Result rc = ipcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 object_id;
        } *raw = r.Raw;

        rc = raw->result;

        if (R_SUCCEEDED(rc)) {
            *object_id_out = raw->object_id;
        }
    }

    return rc;
}

typedef struct {
    u8  Type;
    u8  Pad0;
    u16 Length;
    u32 ObjectId;
    u32 Pad1[2];
} DomainMessageHeader;

static inline void* ipcPrepareHeaderForDomain(IpcCommand* cmd, size_t sizeof_raw, size_t object_id) {
    void* raw = ipcPrepareHeader(cmd, sizeof_raw + sizeof(DomainMessageHeader));
    DomainMessageHeader* hdr = (DomainMessageHeader*) raw;

    hdr->Type = 1;
    hdr->Length = sizeof_raw;
    hdr->ObjectId = object_id;

    hdr->Pad0 = hdr->Pad1[0] = hdr->Pad1[1] = 0;

    return (void*)(((uintptr_t) raw) + sizeof(DomainMessageHeader)); 
}

static inline Result ipcParseResponseForDomain(IpcCommandResponse* r) {
    Result rc = ipcParseResponse(r);

    if (R_SUCCEEDED(rc)) {
        r->Raw = (void*)(((uintptr_t) r->Raw) + sizeof(DomainMessageHeader)); 
    }

    return rc;
}
