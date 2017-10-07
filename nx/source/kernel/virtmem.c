#include <switch.h>

static u8* g_VirtBase;

void virtmemSetup() {
    // TODO: Implement this.
}

void* virtmemReserve(size_t size) {
    void* ret = g_VirtBase;

    size = (size + 0xFFF) &~ 0xFFF;
    g_VirtBase += size + 0x1000;

    return ret;
}

void  virtmemFree(void* addr, size_t size) {
    (void) addr;
    (void) size;
}
