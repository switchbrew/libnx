// Copyright 2018 plutoo
#include <switch.h>

static bool   g_isNso = false;
static Handle g_mainThreadHandle = INVALID_HANDLE;
static void*  g_loaderRetAddr = NULL;
static void*  g_overrideHeapAddr = NULL;
static u64    g_overrideHeapSize = 0;

typedef struct {
    u32 Key;
    u32 Flags;
    u64 Value[2];
} ConfigEntry;

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
            // TODO
            break;

        case EntryType_Argv:
            // TODO
            break;

        case EntryType_SyscallAvailableHint:
            // TODO
            break;

        case EntryType_AppletType:
            // TODO
            break;

        default:
            // TODO
            break;
        }

        ent++;
    }

}

Handle envGetMainThreadHandle(void) {
    if (g_mainThreadHandle != INVALID_HANDLE) {
        return g_mainThreadHandle;
    }

    fatalSimple(1);
}

bool envIsNso(void) {
    return g_isNso;
}
