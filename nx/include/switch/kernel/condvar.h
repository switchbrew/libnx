// Copyright 2018 plutoo
#include "../types.h"
#include "../kernel/mutex.h"

typedef struct {
    u32    tag;
    Mutex* mutex;
} CondVar;

void condvarInit(CondVar* c, Mutex* m);

// When the wait-functions return, the mutex is acquired.
void condvarWaitTimeout(CondVar* c, u64 timeout);
void condvarWait(CondVar* c);

Result condvarWake(CondVar* c, int num);
Result condvarWakeOne(CondVar* c);
Result condvarWakeAll(CondVar* c);
