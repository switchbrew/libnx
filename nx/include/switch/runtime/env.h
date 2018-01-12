// Copyright 2018 plutoo

void envParse(void* ctx, Handle main_thread);

Handle envGetMainThreadHandle(void);
bool envIsNso(void);

bool  envHasHeapOverride(void);
void* envGetHeapOverrideAddr(void);
u64   envGetHeapOverrideSize(void);
