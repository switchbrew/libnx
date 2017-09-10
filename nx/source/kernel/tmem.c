#include <switch.h>

Result tmemCreate(TransferMemory* t, size_t size, Permission perm) {
    t->Size = size;

    Result rc = 0;
    void* addr = heapAllocPages(size);

    if (addr == NULL)
        rc = -1;

    if (R_SUCCEEDED(rc)) {
        rc = svcCreateTransferMemory(&t->MemHandle, addr, size, perm);
    }

    return rc;
}

Result tmemClose(TransferMemory* t) {
    return svcCloseHandle(t->MemHandle);
}
