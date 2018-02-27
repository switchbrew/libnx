// Copyright 2018 plutoo
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/condvar.h"
#include "../internal.h"

void condvarInit(CondVar* c, Mutex* m) {
    c->tag = 0;
    c->mutex = m;
}

Result condvarWaitTimeout(CondVar* c, u64 timeout) {
    Result rc;

    rc = svcWaitProcessWideKeyAtomic((u32*) c->mutex, &c->tag, getThreadVars()->handle, timeout);

    // On timeout, we need to acquire it manually.
    if (rc == 0xEA01)
        mutexLock(c->mutex);

    return rc;
}

Result condvarWake(CondVar* c, int num) {
    return svcSignalProcessWideKey((u32*) &c->tag, num);
}
