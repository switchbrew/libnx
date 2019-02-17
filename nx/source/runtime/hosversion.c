#include "runtime/hosversion.h"

static u32 g_hosVersion;

u32 hosversionGet(void)
{
    return __atomic_load_n(&g_hosVersion, __ATOMIC_SEQ_CST);
}

void hosversionSet(u32 version)
{
    __atomic_store_n(&g_hosVersion, version, __ATOMIC_SEQ_CST);
}
