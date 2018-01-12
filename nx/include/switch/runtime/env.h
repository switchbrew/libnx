// Copyright 2018 plutoo

void envParse(void* ctx, Handle main_thread);

Handle envGetMainThreadHandle(void);
bool envIsNso(void);

bool  envHasHeapOverride(void);
void* envGetHeapOverrideAddr(void);
u64   envGetHeapOverrideSize(void);

bool  envHasArgv(void);
u64   envGetArgc(void);
void* envGetArgv(void);

bool envIsSyscallHinted(u8 svc);

typedef void __attribute__((noreturn)) (*LoaderReturnFn)(int result_code);
