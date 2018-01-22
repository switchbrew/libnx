// Copyright 2018 plutoo
#include <string.h>
#include <switch/runtime/env.h>
#include <switch/services/sm.h>
#include <switch/services/fatal.h>

static bool   g_isNso = false;
static Handle g_mainThreadHandle = INVALID_HANDLE;
static LoaderReturnFn g_loaderRetAddr = NULL;
static void*  g_overrideHeapAddr = NULL;
static u64    g_overrideHeapSize = 0;
static u64    g_overrideArgc = 0;
static void*  g_overrideArgv = NULL;
static u64    g_syscallHints[2];
static Handle g_processHandle = INVALID_HANDLE;
static char*  g_nextLoadPath = NULL;
static char*  g_nextLoadArgv = NULL;

extern __attribute__((weak)) u32 __nx_applet_type;

void envSetup(void* ctx, Handle main_thread, LoaderReturnFn saved_lr)
{
    // If we're running under NSO, we should just call svcExitProcess.
    // Otherwise we should return to loader via LR.
    if (saved_lr == NULL) {
        g_loaderRetAddr = (LoaderReturnFn) &svcExitProcess;
    }
    else {
        g_loaderRetAddr = saved_lr;
    }

    // Detect NSO environment.
    if (main_thread != INVALID_HANDLE)
    {
        g_mainThreadHandle = main_thread;
        g_isNso = true;

        // For NSO we assume kernelhax thus access to all syscalls.
        g_syscallHints[0] = g_syscallHints[1] = -1ull;

        return;
    }

    // Parse NRO config entries.
    ConfigEntry* ent = ctx;

    while (ent->Key != EntryType_EndOfList)
    {
        switch (ent->Key)
        {
        case EntryType_MainThreadHandle:
            g_mainThreadHandle = ent->Value[0];
            break;

        case EntryType_NextLoadPath:
            g_nextLoadPath = (char*) ent->Value[0];
            g_nextLoadArgv = (char*) ent->Value[1];
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

        case EntryType_SyscallAvailableHint:
            g_syscallHints[0] = ent->Value[0];
            g_syscallHints[1] = ent->Value[1];
            break;

        case EntryType_AppletType:
            __nx_applet_type = ent->Value[0];
            break;

        case EntryType_ProcessHandle:
            g_processHandle = ent->Value[0];
            break;

        default:
            if (ent->Flags & EntryFlag_IsMandatory)
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

    fatalSimple(MAKERESULT(Module_Libnx, LibnxError_HandleTooEarly));
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
    return !!(g_syscallHints[svc/64] & (1ull << (svc%64)));
}

Handle envGetOwnProcessHandle(void) {
    return g_processHandle;
}

LoaderReturnFn envGetExitFuncPtr(void) {
    return g_loaderRetAddr;
}

Result envSetNextLoad(const char* path, const char* argv)
{
    if (g_nextLoadPath == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    strcpy(g_nextLoadPath, path);

    if (g_nextLoadArgv != NULL)
    {
        if (argv == NULL)
            g_nextLoadArgv[0] = '\0';
        else
            strcpy(g_nextLoadArgv, argv);
    }

    return 0;
}

bool envHasNextLoad(void) {
    return g_nextLoadPath != NULL;
}
