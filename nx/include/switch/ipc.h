/**
 * @file ipc.h
 * @brief Inter-process communication handling
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "result.h"
#include "arm/tls.h"
#include "kernel/svc.h"

/// IPC input header magic
#define SFCI_MAGIC 0x49434653
/// IPC output header magic
#define SFCO_MAGIC 0x4f434653

/// IPC invalid object ID
#define IPC_INVALID_OBJECT_ID UINT32_MAX

///@name IPC request building
///@{

/// IPC command (request) structure.
#define IPC_MAX_BUFFERS 8
#define IPC_MAX_OBJECTS 8

typedef struct {
    size_t NumSend; // A
    size_t NumRecv; // B
    size_t NumTransfer; // W
    const void* Buffers[IPC_MAX_BUFFERS];
    size_t BufferSizes[IPC_MAX_BUFFERS];
    u8     Flags[IPC_MAX_BUFFERS];

    size_t NumStaticIn;  // X
    size_t NumStaticOut; // C
    const void* Statics[IPC_MAX_BUFFERS];
    size_t StaticSizes[IPC_MAX_BUFFERS];
    u8     Indices[IPC_MAX_BUFFERS];

    bool   SendPid;
    size_t NumHandlesCopy;
    size_t NumHandlesMove;
    Handle Handles[IPC_MAX_OBJECTS];

    size_t NumObjectIds;
    u32    ObjectIds[IPC_MAX_OBJECTS];
} IpcCommand;

/**
 * @brief Initializes an IPC command structure.
 * @param cmd IPC command structure.
 */
static inline void ipcInitialize(IpcCommand* cmd) {
    *cmd = (IpcCommand){0};
}

/// IPC buffer descriptor.
typedef struct {
    u32 Size;   ///< Size of the buffer.
    u32 Addr;   ///< Lower 32-bits of the address of the buffer
    u32 Packed; ///< Packed data (including higher bits of the address)
} IpcBufferDescriptor;

/// IPC static send-buffer descriptor.
typedef struct {
    u32 Packed; ///< Packed data (including higher bits of the address)
    u32 Addr;   ///< Lower 32-bits of the address
} IpcStaticSendDescriptor;

/// IPC static receive-buffer descriptor.
typedef struct {
    u32 Addr;   ///< Lower 32-bits of the address of the buffer
    u32 Packed; ///< Packed data (including higher bits of the address)
} IpcStaticRecvDescriptor;

/**
 * @brief Adds a buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param flags Flags to attach to the buffer.
 */
static inline void ipcAddSendBuffer(IpcCommand* cmd, const void* buffer, size_t size, u8 flags) {
    size_t off = cmd->NumSend;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumSend++;
}

/**
 * @brief Adds a receive-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param flags Flags to attach to the buffer.
 */
static inline void ipcAddRecvBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 flags) {
    size_t off = cmd->NumSend + cmd->NumRecv;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumRecv++;
}

/**
 * @brief Adds a transfer-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param flags Flags to attach to the buffer.
 */
static inline void ipcAddTransferBuffer(IpcCommand* cmd, void* buffer, size_t size, u8 flags) {
    size_t off = cmd->NumSend + cmd->NumRecv + cmd->NumTransfer;
    cmd->Buffers[off] = buffer;
    cmd->BufferSizes[off] = size;
    cmd->Flags[off] = flags;
    cmd->NumTransfer++;
}

/**
 * @brief Adds a static-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param flags Flags to attach to the buffer.
 */
static inline void ipcAddSendStatic(IpcCommand* cmd, const void* buffer, size_t size, u8 index) {
    size_t off = cmd->NumStaticIn;
    cmd->Statics[off] = buffer;
    cmd->StaticSizes[off] = size;
    cmd->Indices[off] = index;
    cmd->NumStaticIn++;
}

/**
 * @brief Adds a static-receive-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param flags Flags to attach to the buffer.
 */
static inline void ipcAddRecvStatic(IpcCommand* cmd, void* buffer, size_t size, u8 index) {
    size_t off = cmd->NumStaticIn + cmd->NumStaticOut;
    cmd->Statics[off] = buffer;
    cmd->StaticSizes[off] = size;
    cmd->Indices[off] = index;
    cmd->NumStaticOut++;
}

/**
 * @brief Tags an IPC command structure to send the PID.
 * @param cmd IPC command structure.
 */
static inline void ipcSendPid(IpcCommand* cmd) {
    cmd->SendPid = true;
}

/**
 * @brief Adds a copy-handle to be sent through an IPC command structure.
 * @param cmd IPC command structure.
 * @param h Handle to send.
 * @remark The receiving process gets a copy of the handle.
 */
static inline void ipcSendHandleCopy(IpcCommand* cmd, Handle h) {
    cmd->Handles[cmd->NumHandlesCopy++] = h;
}

/**
 * @brief Adds a move-handle to be sent through an IPC command structure.
 * @param cmd IPC command structure.
 * @param h Handle to send.
 * @remark The sending process loses ownership of the handle, which is transferred to the receiving process.
 */
static inline void ipcSendHandleMove(IpcCommand* cmd, Handle h) {
    cmd->Handles[cmd->NumHandlesCopy + cmd->NumHandlesMove++] = h;
}

/**
 * @brief Prepares the header of an IPC command structure.
 * @param cmd IPC command structure.
 * @param sizeof_raw Size in bytes of the raw data structure to embed inside the IPC request
 * @return Pointer to the raw embedded data structure in the request, ready to be filled out.
 */
static inline void* ipcPrepareHeader(IpcCommand* cmd, size_t sizeof_raw) {
    u32* buf = (u32*)armGetTls();
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

/**
 * @brief Dispatches an IPC request.
 * @param session IPC session handle.
 * @return Result code.
 */
static inline Result ipcDispatch(Handle session) {
    return svcSendSyncRequest(session);
}

///@}

///@name IPC response parsing
///@{

/// IPC parsed command (response) structure.
typedef struct {
    bool HasPid;                            ///< true if the 'Pid' field is filled out.
    u64  Pid;                               ///< PID included in the response (only if HasPid is true)

    size_t NumHandles;                      ///< Number of handles in the response.
    Handle Handles[IPC_MAX_OBJECTS];        ///< Handles.

    u32    ThisObjectId;                    ///< Object ID to call the command on (for domain messages).
    size_t NumObjectIds;                    ///< Number of object IDs (for domain messages).
    u32    ObjectIds[IPC_MAX_OBJECTS];      ///< Object IDs (for domain messages).

    size_t NumBuffers;                      ///< Number of buffers in the response.
    void*  Buffers[IPC_MAX_BUFFERS];        ///< Pointers to the buffers.
    size_t BufferSizes[IPC_MAX_BUFFERS];    ///< Sizes of the buffers.

    void*  Raw;                             ///< Pointer to the raw embedded data structure in the response.
    size_t RawSize;                         ///< Size of the raw embedded data.
} IpcParsedCommand;

/**
 * @brief Parse an IPC command response into an IPC parsed command structure.
 * @param IPC parsed command structure to fill in.
 * @return Result code.
 */
static inline Result ipcParse(IpcParsedCommand* r) {
    u32* buf = (u32*)armGetTls();
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

        if (num_handles > IPC_MAX_OBJECTS)
            num_handles = IPC_MAX_OBJECTS;

        for (i=0; i<num_handles; i++)
            r->Handles[i] = *(buf-num_handles+i);

        r->NumHandles = num_handles;
    }

    size_t num_statics = (ctrl0 >> 16) & 15;
    buf += num_statics*2;

    size_t num_bufs = ((ctrl0 >> 20) & 15) + ((ctrl0 >> 24) & 15) + (((ctrl0 >> 28) & 15));
    r->Raw = (void*)(((uintptr_t)(buf + num_bufs*3) + 15) &~ 15);

    if (num_bufs > IPC_MAX_BUFFERS)
        num_bufs = IPC_MAX_BUFFERS;

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

/**
 * @brief Queries the size of an IPC pointer buffer.
 * @param session IPC session handle.
 * @param size Output variable in which to store the size.
 * @return Result code.
 */
static inline Result ipcQueryPointerBufferSize(Handle session, size_t *size) {
    u32* buf = (u32*)armGetTls();

    buf[0] = 5;
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

///@}

///@name IPC domain handling
///@{

/**
 * @brief Converts an IPC session handle into a domain.
 * @param session IPC session handle.
 * @param object_id_out Output variable in which to store the object ID.
 * @return Result code.
 */
static inline Result ipcConvertSessionToDomain(Handle session, u32* object_id_out) {
    u32* buf = (u32*)armGetTls();

    buf[0] = 5;
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

/**
 * @brief Adds an object ID to be sent through an IPC domain command structure.
 * @param cmd IPC domain command structure.
 * @param object_id Object ID to send.
 */
static inline void ipcSendObjectId(IpcCommand* cmd, u32 object_id) {
    cmd->ObjectIds[cmd->NumObjectIds++] = object_id;
}

/// IPC domain message header.
typedef struct {
    u8  Type;
    u8  NumObjectIds;
    u16 Length;
    u32 ThisObjectId;
    u32 Pad[2];
} DomainMessageHeader;

/**
 * @brief Prepares the header of an IPC command structure (domain version).
 * @param cmd IPC command structure.
 * @param sizeof_raw Size in bytes of the raw data structure to embed inside the IPC request
 * @oaram object_id Domain object ID.
 * @return Pointer to the raw embedded data structure in the request, ready to be filled out.
 */
static inline void* ipcPrepareHeaderForDomain(IpcCommand* cmd, size_t sizeof_raw, u32 object_id) {
    void* raw = ipcPrepareHeader(cmd, sizeof_raw + sizeof(DomainMessageHeader));
    DomainMessageHeader* hdr = (DomainMessageHeader*) raw;
    u32 *object_ids = (u32*)(((uintptr_t) raw) + sizeof(DomainMessageHeader) + sizeof_raw);

    hdr->Type = 1;
    hdr->NumObjectIds = (u8)cmd->NumObjectIds;
    hdr->Length = sizeof_raw;
    hdr->ThisObjectId = object_id;
    hdr->Pad[0] = hdr->Pad[1] = 0;

    for(size_t i = 0; i < cmd->NumObjectIds; i++)
        object_ids[i] = cmd->ObjectIds[i];
    return (void*)(((uintptr_t) raw) + sizeof(DomainMessageHeader));
}

/**
 * @brief Parse an IPC command response into an IPC parsed command structure (domain version).
 * @param IPC parsed command structure to fill in.
 * @return Result code.
 */
static inline Result ipcParseForDomain(IpcParsedCommand* r) {
    Result rc = ipcParse(r);
    DomainMessageHeader* hdr;
    u32 *object_ids;
    if(R_FAILED(rc))
        return rc;

    hdr = (DomainMessageHeader*) r->Raw;
    object_ids = (u32*)(((uintptr_t) hdr) + sizeof(DomainMessageHeader) + hdr->Length);
    r->Raw = (void*)(((uintptr_t) r->Raw) + sizeof(DomainMessageHeader));

    r->ThisObjectId = hdr->ThisObjectId;
    r->NumObjectIds = hdr->NumObjectIds > 8 ? 8 : hdr->NumObjectIds;
    for(size_t i = 0; i < r->NumObjectIds; i++)
        r->ObjectIds[i] = object_ids[i];

    return rc;
}

/**
 * @brief Closes a domain object by ID.
 * @param session IPC session handle.
 * @param object_id ID of the object to close.
 * @return Result code.
 */
static inline Result ipcCloseObjectById(Handle session, u32 object_id) {
    IpcCommand c;
    DomainMessageHeader* hdr;

    ipcInitialize(&c);
    hdr = (DomainMessageHeader*)ipcPrepareHeader(&c, sizeof(DomainMessageHeader));

    hdr->Type = 2;
    hdr->NumObjectIds = 0;
    hdr->Length = 0;
    hdr->ThisObjectId = object_id;
    hdr->Pad[0] = hdr->Pad[1] = 0;

    return ipcDispatch(session); // this command has no associated response
}

///@}
