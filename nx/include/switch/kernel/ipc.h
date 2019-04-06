/**
 * @file ipc.h
 * @brief Inter-process communication handling
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../result.h"
#include "../arm/tls.h"
#include "../kernel/svc.h"

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

typedef enum {
    BufferType_Normal=0,  ///< Regular buffer.
    BufferType_Type1=1,   ///< Allows ProcessMemory and shared TransferMemory.
    BufferType_Invalid=2,
    BufferType_Type3=3    ///< Same as Type1 except remote process is not allowed to use device-mapping.
} BufferType;

typedef enum {
    BufferDirection_Send=0,
    BufferDirection_Recv=1,
    BufferDirection_Exch=2,
} BufferDirection;

typedef enum {
    IpcCommandType_Invalid = 0,
    IpcCommandType_LegacyRequest = 1,
    IpcCommandType_Close = 2,
    IpcCommandType_LegacyControl = 3,
    IpcCommandType_Request = 4,
    IpcCommandType_Control = 5,
    IpcCommandType_RequestWithContext = 6,
    IpcCommandType_ControlWithContext = 7,
} IpcCommandType;

typedef enum {
    DomainMessageType_Invalid = 0,
    DomainMessageType_SendMessage = 1,
    DomainMessageType_Close = 2,
} DomainMessageType;

/// IPC domain message header.
typedef struct {
    u8  Type;
    u8  NumObjectIds;
    u16 Length;
    u32 ThisObjectId;
    u32 Pad[2];
} DomainMessageHeader;

/// IPC domain response header.
typedef struct {
    u32 NumObjectIds;
    u32 Pad[3];
} DomainResponseHeader;


typedef struct {
    size_t NumSend; // A
    size_t NumRecv; // B
    size_t NumExch; // W
    const void* Buffers[IPC_MAX_BUFFERS];
    size_t BufferSizes[IPC_MAX_BUFFERS];
    BufferType BufferTypes[IPC_MAX_BUFFERS];

    size_t NumStaticIn;  // X
    size_t NumStaticOut; // C
    const void* Statics[IPC_MAX_BUFFERS];
    size_t StaticSizes[IPC_MAX_BUFFERS];
    u8     StaticIndices[IPC_MAX_BUFFERS];

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
void ipcInitialize(IpcCommand* cmd);

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
 * @param type Buffer type.
 */
void ipcAddSendBuffer(IpcCommand* cmd, const void* buffer, size_t size, BufferType type);

/**
 * @brief Adds a receive-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param type Buffer type.
 */
void ipcAddRecvBuffer(IpcCommand* cmd, void* buffer, size_t size, BufferType type);

/**
 * @brief Adds an exchange-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param type Buffer type.
 */
void ipcAddExchBuffer(IpcCommand* cmd, void* buffer, size_t size, BufferType type);

/**
 * @brief Adds a static-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param index Index of buffer.
 */
void ipcAddSendStatic(IpcCommand* cmd, const void* buffer, size_t size, u8 index);

/**
 * @brief Adds a static-receive-buffer to an IPC command structure.
 * @param cmd IPC command structure.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param index Index of buffer.
 */
void ipcAddRecvStatic(IpcCommand* cmd, void* buffer, size_t size, u8 index);
/**
 * @brief Adds a smart-buffer (buffer + static-buffer pair) to an IPC command structure.
 * @param cmd IPC command structure.
 * @param ipc_buffer_size IPC buffer size.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param index Index of buffer.
 */
void ipcAddSendSmart(IpcCommand* cmd, size_t ipc_buffer_size, const void* buffer, size_t size, u8 index);

/**
 * @brief Adds a smart-receive-buffer (buffer + static-receive-buffer pair) to an IPC command structure.
 * @param cmd IPC command structure.
 * @param ipc_buffer_size IPC buffer size.
 * @param buffer Address of the buffer.
 * @param size Size of the buffer.
 * @param index Index of buffer.
 */
void ipcAddRecvSmart(IpcCommand* cmd, size_t ipc_buffer_size, void* buffer, size_t size, u8 index);

/**
 * @brief Tags an IPC command structure to send the PID.
 * @param cmd IPC command structure.
 */
void ipcSendPid(IpcCommand* cmd);

/**
 * @brief Adds a copy-handle to be sent through an IPC command structure.
 * @param cmd IPC command structure.
 * @param h Handle to send.
 * @remark The receiving process gets a copy of the handle.
 */
void ipcSendHandleCopy(IpcCommand* cmd, Handle h);

/**
 * @brief Adds a move-handle to be sent through an IPC command structure.
 * @param cmd IPC command structure.
 * @param h Handle to send.
 * @remark The sending process loses ownership of the handle, which is transferred to the receiving process.
 */
void ipcSendHandleMove(IpcCommand* cmd, Handle h);

/**
 * @brief Prepares the header of an IPC command structure.
 * @param cmd IPC command structure.
 * @param sizeof_raw Size in bytes of the raw data structure to embed inside the IPC request
 * @return Pointer to the raw embedded data structure in the request, ready to be filled out.
 */
void* ipcPrepareHeader(IpcCommand* cmd, size_t sizeof_raw);

/**
 * @brief Dispatches an IPC request.
 * @param session IPC session handle.
 * @return Result code.
 */
Result ipcDispatch(Handle session);

///@}

///@name IPC response parsing
///@{

/// IPC parsed command (response) structure.
typedef struct {
    IpcCommandType CommandType;               ///< Type of the command  
    
    bool HasPid;                              ///< true if the 'Pid' field is filled out.
    u64  Pid;                                 ///< PID included in the response (only if HasPid is true)

    size_t NumHandles;                        ///< Number of handles copied.
    Handle Handles[IPC_MAX_OBJECTS];          ///< Handles.
    bool   WasHandleCopied[IPC_MAX_OBJECTS];  ///< true if the handle was moved, false if it was copied.

    bool   IsDomainRequest;                   ///< true if the the message is a Domain message.
    DomainMessageType InMessageType;          ///< Type of the domain message.
    u32    InMessageLength;                   ///< Size of rawdata (for domain messages).
    u32    InThisObjectId;                    ///< Object ID to call the command on (for domain messages).
    size_t InNumObjectIds;                    ///< Number of object IDs (for domain messages).
    u32    InObjectIds[IPC_MAX_OBJECTS];      ///< Object IDs (for domain messages).
    
    bool   IsDomainResponse;                  ///< true if the the message is a Domain response.
    size_t OutNumObjectIds;                   ///< Number of object IDs (for domain responses).
    u32    OutObjectIds[IPC_MAX_OBJECTS];     ///< Object IDs (for domain responses).

    size_t NumBuffers;                        ///< Number of buffers in the response.
    void*  Buffers[IPC_MAX_BUFFERS];          ///< Pointers to the buffers.
    size_t BufferSizes[IPC_MAX_BUFFERS];      ///< Sizes of the buffers.
    BufferType BufferTypes[IPC_MAX_BUFFERS];  ///< Types of the buffers.
    BufferDirection BufferDirections[IPC_MAX_BUFFERS]; ///< Direction of each buffer.

    size_t NumStatics;                        ///< Number of statics in the response.
    void*  Statics[IPC_MAX_BUFFERS];          ///< Pointers to the statics.
    size_t StaticSizes[IPC_MAX_BUFFERS];      ///< Sizes of the statics.
    u8     StaticIndices[IPC_MAX_BUFFERS];    ///< Indices of the statics.
    
    size_t NumStaticsOut;                     ///< Number of output statics available in the response.

    void*  Raw;                               ///< Pointer to the raw embedded data structure in the response.
    void*  RawWithoutPadding;                 ///< Pointer to the raw embedded data structure, without padding.
    size_t RawSize;                           ///< Size of the raw embedded data.
} IpcParsedCommand;

/**
 * @brief Parse an IPC command response into an IPC parsed command structure.
 * @param r IPC parsed command structure to fill in.
 * @return Result code.
 */
Result ipcParse(IpcParsedCommand* r);

/**
 * @brief Queries the size of an IPC pointer buffer.
 * @param session IPC session handle.
 * @param size Output variable in which to store the size.
 * @return Result code.
 */
Result ipcQueryPointerBufferSize(Handle session, size_t *size);

/**
 * @brief Closes the IPC session with proper clean up.
 * @param session IPC session handle.
 * @return Result code.
 */
Result ipcCloseSession(Handle session);

/**
 * @brief Clones an IPC session.
 * @param session IPC session handle.
 * @param unk Unknown.
 * @param new_session_out Output cloned IPC session handle.
 * @return Result code.
 */
Result ipcCloneSession(Handle session, u32 unk, Handle* new_session_out);

///@}

///@name IPC domain handling
///@{

/**
 * @brief Converts an IPC session handle into a domain.
 * @param session IPC session handle.
 * @param object_id_out Output variable in which to store the object ID.
 * @return Result code.
 */
Result ipcConvertSessionToDomain(Handle session, u32* object_id_out);

/**
 * @brief Adds an object ID to be sent through an IPC domain command structure.
 * @param cmd IPC domain command structure.
 * @param object_id Object ID to send.
 */
void ipcSendObjectId(IpcCommand* cmd, u32 object_id);

/**
 * @brief Prepares the header of an IPC command structure (domain version).
 * @param cmd IPC command structure.
 * @param sizeof_raw Size in bytes of the raw data structure to embed inside the IPC request
 * @param object_id Domain object ID.
 * @return Pointer to the raw embedded data structure in the request, ready to be filled out.
 */
void* ipcPrepareHeaderForDomain(IpcCommand* cmd, size_t sizeof_raw, u32 object_id);

/**
 * @brief Parse an IPC command request into an IPC parsed command structure (domain version).
 * @param r IPC parsed command structure to fill in.
 * @return Result code.
 */
Result ipcParseDomainRequest(IpcParsedCommand* r);

/**
 * @brief Parse an IPC command response into an IPC parsed command structure (domain version).
 * @param r IPC parsed command structure to fill in.
 * @param sizeof_raw Size in bytes of the raw data structure.
 * @return Result code.
 */
Result ipcParseDomainResponse(IpcParsedCommand* r, size_t sizeof_raw);

/**
 * @brief Closes a domain object by ID.
 * @param session IPC session handle.
 * @param object_id ID of the object to close.
 * @return Result code.
 */
Result ipcCloseObjectById(Handle session, u32 object_id);

///@}
