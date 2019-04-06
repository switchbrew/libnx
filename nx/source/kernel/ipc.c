#include "kernel/ipc.h"

void ipcInitialize(IpcCommand* cmd) {
    *cmd = (IpcCommand){0};
}

void ipcAddSendBuffer(IpcCommand* cmd, const void* buffer, size_t size, BufferType type) {
    size_t off = cmd->NumSend;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->BufferTypes[off] = type;
    cmd->NumSend++;
}

void ipcAddRecvBuffer(IpcCommand* cmd, void* buffer, size_t size, BufferType type) {
    size_t off = cmd->NumSend + cmd->NumRecv;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->BufferTypes[off] = type;
    cmd->NumRecv++;
}

void ipcAddExchBuffer(IpcCommand* cmd, void* buffer, size_t size, BufferType type) {
    size_t off = cmd->NumSend + cmd->NumRecv + cmd->NumExch;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->BufferTypes[off] = type;
    cmd->NumExch++;
}

void ipcAddSendStatic(IpcCommand* cmd, const void* buffer, size_t size, u8 index) {
    size_t off = cmd->NumStaticIn;
    cmd->Statics[off] = buffer;
    cmd->StaticSizes[off] = size;
    cmd->StaticIndices[off] = index;
    cmd->NumStaticIn++;
}

void ipcAddRecvStatic(IpcCommand* cmd, void* buffer, size_t size, u8 index) {
    size_t off = cmd->NumStaticIn + cmd->NumStaticOut;
    cmd->Statics[off] = buffer;
    cmd->StaticSizes[off] = size;
    cmd->StaticIndices[off] = index;
    cmd->NumStaticOut++;
}

void ipcAddSendSmart(IpcCommand* cmd, size_t ipc_buffer_size, const void* buffer, size_t size, u8 index) {
    if (ipc_buffer_size != 0 && size <= ipc_buffer_size) {
        ipcAddSendBuffer(cmd, NULL, 0, BufferType_Normal);
        ipcAddSendStatic(cmd, buffer, size, index);
    } else {
        ipcAddSendBuffer(cmd, buffer, size, BufferType_Normal);
        ipcAddSendStatic(cmd, NULL, 0, index);
    }
}

void ipcAddRecvSmart(IpcCommand* cmd, size_t ipc_buffer_size, void* buffer, size_t size, u8 index) {
    if (ipc_buffer_size != 0 && size <= ipc_buffer_size) {
        ipcAddRecvBuffer(cmd, NULL, 0, BufferType_Normal);
        ipcAddRecvStatic(cmd, buffer, size, index);
    } else {
        ipcAddRecvBuffer(cmd, buffer, size, BufferType_Normal);
        ipcAddRecvStatic(cmd, NULL, 0, index);
    }
}

void ipcSendPid(IpcCommand* cmd) {
    cmd->SendPid = true;
}

void ipcSendHandleCopy(IpcCommand* cmd, Handle h) {
    cmd->Handles[cmd->NumHandlesCopy++] = h;
}

void ipcSendHandleMove(IpcCommand* cmd, Handle h) {
    cmd->Handles[cmd->NumHandlesCopy + cmd->NumHandlesMove++] = h;
}

void* ipcPrepareHeader(IpcCommand* cmd, size_t sizeof_raw) {
    u32* buf = (u32*)armGetTls();
    size_t i;
    *buf++ = IpcCommandType_Request | (cmd->NumStaticIn << 16) | (cmd->NumSend << 20) | (cmd->NumRecv << 24) | (cmd->NumExch << 28);

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
        desc->Packed = cmd->StaticIndices[i] | (cmd->StaticSizes[i] << 16) |
            (((ptr >> 32) & 15) << 12) | (((ptr >> 36) & 15) << 6);
    }

    for (i=0; i<(cmd->NumSend + cmd->NumRecv + cmd->NumExch); i++, buf+=3) {
        IpcBufferDescriptor* desc = (IpcBufferDescriptor*) buf;
        desc->Size = cmd->BufferSizes[i];

        uintptr_t ptr = (uintptr_t) cmd->Buffers[i];
        desc->Addr = ptr;
        desc->Packed = cmd->BufferTypes[i] |
            (((ptr >> 32) & 15) << 28) | ((ptr >> 36) << 2);
    }

    u32 padding = ((16 - (((uintptr_t) buf) & 15)) & 15) / 4;
    u32* raw = (u32*) (buf + padding);

    size_t raw_size = (sizeof_raw/4) + 4;
    buf += raw_size;

    u16* buf_u16 = (u16*) buf;

    for (i=0; i<cmd->NumStaticOut; i++) {
        size_t off = cmd->NumStaticIn + i;
        size_t sz = (uintptr_t) cmd->StaticSizes[off];

        buf_u16[i] = (sz > 0xFFFF) ? 0 : sz;
    }

    size_t u16s_size = ((2*cmd->NumStaticOut) + 3)/4;
    buf += u16s_size;
    raw_size += u16s_size;

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

Result ipcDispatch(Handle session) {
    return svcSendSyncRequest(session);
}

Result ipcParse(IpcParsedCommand* r) {
    u32* buf = (u32*)armGetTls();
    u32 ctrl0 = *buf++;
    u32 ctrl1 = *buf++;
    size_t i;
    
    r->IsDomainRequest = false;
    r->IsDomainResponse = false;

    r->CommandType = (IpcCommandType) (ctrl0 & 0xffff);
    r->HasPid = false;
    r->RawSize = (ctrl1 & 0x1ff) * 4;
    r->NumHandles = 0;
    
    r->NumStaticsOut = (ctrl1 >> 10) & 15;
    if (r->NumStaticsOut >> 1) r->NumStaticsOut--; // Value 2  -> Single descriptor
    if (r->NumStaticsOut >> 1) r->NumStaticsOut--; // Value 3+ -> (Value - 2) descriptors

    if (ctrl1 & 0x80000000) {
        u32 ctrl2 = *buf++;

        if (ctrl2 & 1) {
            r->HasPid = true;
            r->Pid = *buf++;
            r->Pid |= ((u64)(*buf++)) << 32;
        }

        size_t num_handles_copy = ((ctrl2 >> 1) & 15);
        size_t num_handles_move = ((ctrl2 >> 5) & 15);

        size_t num_handles = num_handles_copy + num_handles_move;
        u32* buf_after_handles = buf + num_handles;

        if (num_handles > IPC_MAX_OBJECTS)
            num_handles = IPC_MAX_OBJECTS;

        for (i=0; i<num_handles; i++)
        {
            r->Handles[i] = *(buf+i);
            r->WasHandleCopied[i] = (i < num_handles_copy);
        }

        r->NumHandles = num_handles;
        buf = buf_after_handles;
    }

    size_t num_statics = (ctrl0 >> 16) & 15;
    u32* buf_after_statics = buf + num_statics*2;

    if (num_statics > IPC_MAX_BUFFERS)
        num_statics = IPC_MAX_BUFFERS;

    for (i=0; i<num_statics; i++, buf+=2) {
        IpcStaticSendDescriptor* desc = (IpcStaticSendDescriptor*) buf;
        u64 packed = (u64) desc->Packed;

        r->Statics[i] = (void*) (desc->Addr | (((packed >> 12) & 15) << 32) | (((packed >> 6) & 15) << 36));
        r->StaticSizes[i]   = packed >> 16;
        r->StaticIndices[i] = packed & 63;
    }

    r->NumStatics = num_statics;
    buf = buf_after_statics;

    size_t num_bufs_send = (ctrl0 >> 20) & 15;
    size_t num_bufs_recv = (ctrl0 >> 24) & 15;
    size_t num_bufs_exch = (ctrl0 >> 28) & 15;

    size_t num_bufs = num_bufs_send + num_bufs_recv + num_bufs_exch;
    r->Raw = (void*)(((uintptr_t)(buf + num_bufs*3) + 15) &~ 15);
    r->RawWithoutPadding = (void*)((uintptr_t)(buf + num_bufs*3));

    if (num_bufs > IPC_MAX_BUFFERS)
        num_bufs = IPC_MAX_BUFFERS;

    for (i=0; i<num_bufs; i++, buf+=3) {
        IpcBufferDescriptor* desc = (IpcBufferDescriptor*) buf;
        u64 packed = (u64) desc->Packed;

        r->Buffers[i] = (void*) (desc->Addr | ((packed >> 28) << 32) | (((packed >> 2) & 15) << 36));
        r->BufferSizes[i] = desc->Size;
        r->BufferTypes[i] = (BufferType) (packed & 3);

        if (i < num_bufs_send)
            r->BufferDirections[i] = BufferDirection_Send;
        else if (i < (num_bufs_send + num_bufs_recv))
            r->BufferDirections[i] = BufferDirection_Recv;
        else
            r->BufferDirections[i] = BufferDirection_Exch;
    }

    r->NumBuffers = num_bufs;
    return 0;
}

Result ipcQueryPointerBufferSize(Handle session, size_t *size) {
    u32* buf = (u32*)armGetTls();

    buf[0] = IpcCommandType_Control;
    buf[1] = 8;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = SFCI_MAGIC;
    buf[5] = 0;
    buf[6] = 3;
    buf[7] = 0;

    Result rc = ipcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct ipcQueryPointerBufferSizeResponse {
            u64 magic;
            u64 result;
            u32 size;
        } *raw = (struct ipcQueryPointerBufferSizeResponse*)r.Raw;

        rc = raw->result;

        if (R_SUCCEEDED(rc)) {
            *size = raw->size & 0xffff;
        }
    }

    return rc;
}

Result ipcCloseSession(Handle session) {
    u32* buf = (u32*)armGetTls();
    buf[0] = IpcCommandType_Close;
    buf[1] = 0;
    return ipcDispatch(session);
}

Result ipcCloneSession(Handle session, u32 unk, Handle* new_session_out) {
    u32* buf = (u32*)armGetTls();

    buf[0] = IpcCommandType_Control;
    buf[1] = 9;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = SFCI_MAGIC;
    buf[5] = 0;
    buf[6] = 4;
    buf[7] = 0;
    buf[8] = unk;

    Result rc = ipcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct ipcCloneSessionResponse {
            u64 magic;
            u64 result;
        } *raw = (struct ipcCloneSessionResponse*)r.Raw;

        rc = raw->result;

        if (R_SUCCEEDED(rc) && new_session_out) {
            *new_session_out = r.Handles[0];
        }
    }

    return rc;
}

Result ipcConvertSessionToDomain(Handle session, u32* object_id_out) {
    u32* buf = (u32*)armGetTls();

    buf[0] = IpcCommandType_Control;
    buf[1] = 8;
    buf[4] = SFCI_MAGIC;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;

    Result rc = ipcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct ipcConvertSessionToDomainResponse {
            u64 magic;
            u64 result;
            u32 object_id;
        } *raw = (struct ipcConvertSessionToDomainResponse*)r.Raw;

        rc = raw->result;

        if (R_SUCCEEDED(rc)) {
            *object_id_out = raw->object_id;
        }
    }

    return rc;
}

void ipcSendObjectId(IpcCommand* cmd, u32 object_id) {
    cmd->ObjectIds[cmd->NumObjectIds++] = object_id;
}

void* ipcPrepareHeaderForDomain(IpcCommand* cmd, size_t sizeof_raw, u32 object_id) {
    void* raw = ipcPrepareHeader(cmd, sizeof_raw + sizeof(DomainMessageHeader) + cmd->NumObjectIds*sizeof(u32));
    DomainMessageHeader* hdr = (DomainMessageHeader*) raw;
    u32 *object_ids = (u32*)(((uintptr_t) raw) + sizeof(DomainMessageHeader) + sizeof_raw);

    hdr->Type = DomainMessageType_SendMessage;
    hdr->NumObjectIds = (u8)cmd->NumObjectIds;
    hdr->Length = sizeof_raw;
    hdr->ThisObjectId = object_id;
    hdr->Pad[0] = hdr->Pad[1] = 0;

    for(size_t i = 0; i < cmd->NumObjectIds; i++)
        object_ids[i] = cmd->ObjectIds[i];
    return (void*)(((uintptr_t) raw) + sizeof(DomainMessageHeader));
}

Result ipcParseDomainRequest(IpcParsedCommand* r) {
    Result rc = ipcParse(r);
    DomainMessageHeader *hdr;
    u32 *object_ids;
    if(R_FAILED(rc))
        return rc;

    hdr = (DomainMessageHeader*) r->Raw;
    object_ids = (u32*)(((uintptr_t) hdr) + sizeof(DomainMessageHeader) + hdr->Length);
    r->Raw = (void*)(((uintptr_t) r->Raw) + sizeof(DomainMessageHeader));

    r->IsDomainRequest = true;
    r->InMessageType = (DomainMessageType)(hdr->Type);
    switch (r->InMessageType) {
        case DomainMessageType_SendMessage:
        case DomainMessageType_Close:
            break;
        default:
            return MAKERESULT(Module_Libnx, LibnxError_DomainMessageUnknownType);
    }

    r->InThisObjectId = hdr->ThisObjectId;
    r->InNumObjectIds = hdr->NumObjectIds > 8 ? 8 : hdr->NumObjectIds;
    if ((uintptr_t)object_ids + sizeof(u32) * r->InNumObjectIds - (uintptr_t)armGetTls() >= 0x100) {
        return MAKERESULT(Module_Libnx, LibnxError_DomainMessageTooManyObjectIds);
    }
    for(size_t i = 0; i < r->InNumObjectIds; i++)
        r->InObjectIds[i] = object_ids[i];

    return rc;
}

Result ipcParseDomainResponse(IpcParsedCommand* r, size_t sizeof_raw) {
    Result rc = ipcParse(r);
    DomainResponseHeader *hdr;
    u32 *object_ids;
    if(R_FAILED(rc))
        return rc;

    hdr = (DomainResponseHeader*) r->Raw;
    r->Raw = (void*)(((uintptr_t) r->Raw) + sizeof(DomainResponseHeader));
    object_ids = (u32*)(((uintptr_t) r->Raw) + sizeof_raw);//Official sw doesn't align this.

    r->IsDomainResponse = true;
    
    r->OutNumObjectIds = hdr->NumObjectIds > 8 ? 8 : hdr->NumObjectIds;
    if ((uintptr_t)object_ids + sizeof(u32) * r->OutNumObjectIds - (uintptr_t)armGetTls() >= 0x100) {
        return MAKERESULT(Module_Libnx, LibnxError_DomainMessageTooManyObjectIds);
    }
    for(size_t i = 0; i < r->OutNumObjectIds; i++)
        r->OutObjectIds[i] = object_ids[i];

    return rc;
}

Result ipcCloseObjectById(Handle session, u32 object_id) {
    IpcCommand c;
    DomainMessageHeader* hdr;

    ipcInitialize(&c);
    hdr = (DomainMessageHeader*)ipcPrepareHeader(&c, sizeof(DomainMessageHeader));

    hdr->Type = DomainMessageType_Close;
    hdr->NumObjectIds = 0;
    hdr->Length = 0;
    hdr->ThisObjectId = object_id;
    hdr->Pad[0] = hdr->Pad[1] = 0;

    return ipcDispatch(session); // this command has no associated response
}