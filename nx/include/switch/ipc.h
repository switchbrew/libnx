typedef struct {
    size_t NumSend; // A
    size_t NumRecv; // B
    size_t NumStaticSend; // X
    size_t NumStaticRecv; // C
    void*  Buffers[8];
    size_t Sizes[8];
    u8     Flags[8];

    bool   SendPid;
    size_t NumHandles;
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
