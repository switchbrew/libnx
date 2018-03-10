// Copyright 2018 plutoo
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/event.h"

Result eventCreate(Event* t)
{
    Result rc;

    rc = svcCreateEvent(&t->revent, &t->wevent);

    if (R_FAILED(rc)) {
        t->revent = INVALID_HANDLE;
        t->wevent = INVALID_HANDLE;
    }

    return rc;
}

void eventLoadRemote(Event* t, Handle handle) {
    t->revent = handle;
}

Result eventWait(Event* t, u64 timeout)
{
    Result rc;

    rc = svcWaitSynchronizationSingle(t->revent, timeout);

    if (R_SUCCEEDED(rc)) {
        rc = svcResetSignal(t->revent);
    }

    return rc;
}

Result eventFire(Event* t) {
    return svcSignalEvent(t->wevent);
}

Result eventClear(Event* t)
{
    if (t->wevent != INVALID_HANDLE)
        return svcClearEvent(t->wevent);

    if (t->revent != INVALID_HANDLE)
        return svcClearEvent(t->revent);

    return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
}

void eventClose(Event* t)
{
    if (t->revent != INVALID_HANDLE)
        svcCloseHandle(t->revent);
    if (t->wevent != INVALID_HANDLE)
        svcCloseHandle(t->wevent);

    t->revent = INVALID_HANDLE;
    t->wevent = INVALID_HANDLE;
}
