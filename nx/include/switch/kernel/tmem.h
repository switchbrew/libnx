typedef struct {
    Handle MemHandle;
    size_t Size;
} TransferMemory;

typedef enum {
    PERM_NONE = 0,
    PERM_R    = 1,
    PERM_RW   = 3
} Permission;

Result tmemCreate(TransferMemory* t, size_t size, Permission perm);
Result tmemClose(TransferMemory* t);

