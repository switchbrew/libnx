// Copyright 2018 plutoo
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/cond_var.h"
#include "../internal.h"

void condvarInit(CondVar* c, Mutex* m) {
    c->tag = 0;
    c->mutex = m;
}

void condvarWaitTimeout(CondVar* c, u64 timeout) {
    Result rc;

    rc = svcWaitProcessWideKeyAtomic(&c->tag, (u32*) c->mutex, getThreadVars()->handle, timeout);

    // On timeout, we need to acquire it manually.
    if (rc == 0xEA01)
        mutexLock(c->mutex);
}

void condvarWait(CondVar* c) {
    return condvarWaitTimeout(c, -1);
}

Result condvarWake(CondVar* c, int num) {
    return svcSignalProcessWideKey((u32*) &c->tag, num);
}

Result condvarWakeOne(CondVar* c) {
    return condvarWake(c, 1);
}

Result condvarWakeAll(CondVar* c) {
    return condvarWake(c, -1);
}
