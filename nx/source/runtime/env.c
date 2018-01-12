// Copyright 2018 plutoo
#include <switch.h>

static bool   g_isNso = false;
static Handle g_mainThreadHandle = INVALID_HANDLE;
static LoaderReturnFn  g_loaderRetAddr = NULL;
static void*  g_overrideHeapAddr = NULL;
static u64    g_overrideHeapSize = 0;
static u64    g_overrideArgc = 0;
static void*  g_overrideArgv = NULL;
static u64    g_syscallHints[0x80/8];

typedef struct {
    u32 Key;
    u32 Flags;
    u64 Value[2];
} ConfigEntry;

enum {
    IsMandatory = BIT(0),
};

enum {
    EntryType_EndOfList=0,
    EntryType_MainThreadHandle=1,
    EntryType_LoaderReturnAddr=2,
    EntryType_OverrideHeap=3,
    EntryType_OverrideService=4,
    EntryType_Argv=5,
    EntryType_SyscallAvailableHint=6,
    EntryType_AppletType=7
};

void envParse(void* ctx, Handle main_thread)
{
    if (main_thread != INVALID_HANDLE)
    {
        g_mainThreadHandle = main_thread;
        g_isNso = true;

        // For NSO we assume kernelhax thus access to all syscalls.
        memset((void*) &g_syscallHints, 0xFF, sizeof(g_syscallHints));

        return;
    }

    ConfigEntry* ent = ctx;

    while (ent->Key != EntryType_EndOfList)
    {
        switch (ent->Key)
        {
        case EntryType_MainThreadHandle:
            g_mainThreadHandle = ent->Value[0];
            break;

        case EntryType_LoaderReturnAddr:
            g_loaderRetAddr = (void*) ent->Value[0];
            break;

        case EntryType_OverrideHeap:
            g_overrideHeapAddr = (void*) ent->Value[0];
            g_overrideHeapSize = ent->Value[1];
            break;

        case EntryType_OverrideService:
            smAddOverrideHandle(ent->Value[0], ent->Value[1]);
            break;

        case EntryType_Argv:
            g_overrideArgc = ent->Value[0];
            g_overrideArgv = (void*) ent->Value[1];
            break;

        case EntryType_SyscallAvailableHint: {
            int i, j;

            for (i=0; i<2; i++) for (j=0; j<8; j++)
            {
                u8 svc = ent->Value[i] >> (8*j);

                if (svc < 0x80)
                    g_syscallHints[svc/64] |= 1llu << (svc%64);
            }

            break;
        }

        case EntryType_AppletType:
            // TODO
            break;

        default:
            if (ent->Flags & IsMandatory)
            {
                // Encountered unknown but mandatory key, bail back to loader.
                g_loaderRetAddr(MAKERESULT(346, 100 + ent->Key));
            }

            break;
        }

        ent++;
    }

}

Handle envGetMainThreadHandle(void) {
    if (g_mainThreadHandle != INVALID_HANDLE) {
        return g_mainThreadHandle;
    }

    fatalSimple(MAKERESULT(MODULE_LIBNX, LIBNX_HANDLETOOEARLY));
}

bool envIsNso(void) {
    return g_isNso;
}

bool envHasHeapOverride(void) {
    return g_overrideHeapAddr != NULL;
}

void* envGetHeapOverrideAddr(void) {
    return g_overrideHeapAddr;
}

u64 envGetHeapOverrideSize(void) {
    return g_overrideHeapSize;
}

bool envHasArgv(void) {
    return g_overrideArgv != NULL;
}

u64 envGetArgc(void) {
    return g_overrideArgc;
}

void* envGetArgv(void) {
    return g_overrideArgv;
}

bool envIsSyscallHinted(u8 svc) {
    return !!(g_syscallHints[svc/64] & (1llu << (svc%64)));
}
