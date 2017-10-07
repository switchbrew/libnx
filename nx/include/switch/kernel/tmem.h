typedef struct {
    Handle      MemHandle;
    size_t      Size;
    Permission  Perm;
    void*       SourceAddr;
    void*       MappedAddr;
} TransferMemory;

Result tmemCreate(TransferMemory* t, size_t size, Permission perm);
void   tmemLoadRemote(TransferMemory* t, Handle handle, size_t size, Permission perm);

Result tmemMap(TransferMemory* t);
Result tmemUnmap(TransferMemory* t);
void*  tmemGetAddr(TransferMemory* t);

Result tmemClose(TransferMemory* t);
