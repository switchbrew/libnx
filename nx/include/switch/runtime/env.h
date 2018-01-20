// Copyright 2018 plutoo

typedef struct {
    u32 Key;
    u32 Flags;
    u64 Value[2];
} ConfigEntry;

enum {
    EntryFlag_IsMandatory = BIT(0),
};

enum {
    EntryType_EndOfList=0,
    EntryType_MainThreadHandle=1,
    EntryType_NextLoadPath=2,
    EntryType_OverrideHeap=3,
    EntryType_OverrideService=4,
    EntryType_Argv=5,
    EntryType_SyscallAvailableHint=6,
    EntryType_AppletType=7,
    EntryType_AppletWorkaround=8,
    EntryType_StdioSockets=9,
    EntryType_ProcessHandle=10,
    EntryType_LastLoadResult=11
};

typedef void NORETURN (*LoaderReturnFn)(int result_code);

void envSetup(void* ctx, Handle main_thread, LoaderReturnFn saved_lr);

Handle envGetMainThreadHandle(void);
bool envIsNso(void);

bool  envHasHeapOverride(void);
void* envGetHeapOverrideAddr(void);
u64   envGetHeapOverrideSize(void);

bool  envHasArgv(void);
u64   envGetArgc(void);
void* envGetArgv(void);

bool envIsSyscallHinted(u8 svc);

Handle envGetOwnProcessHandle(void);

LoaderReturnFn envGetExitFuncPtr(void);

Result envSetNextLoad(const char* path, const char* argv);
