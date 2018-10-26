// Copyright 2018 plutoo
#include "types.h"
#include "result.h"
#include "arm/counter.h"
#include "kernel/svc.h"
#include "kernel/event.h"

Result eventCreate(Event* t, bool autoclear)
{
    Result rc;

    rc = svcCreateEvent(&t->wevent, &t->revent);
    t->autoclear = autoclear;

    if (R_FAILED(rc)) {
        t->revent = INVALID_HANDLE;
        t->wevent = INVALID_HANDLE;
    }

    return rc;
}

void eventLoadRemote(Event* t, Handle handle, bool autoclear)
{
    t->revent = handle;
    t->wevent = INVALID_HANDLE;
    t->autoclear = autoclear;
}

Result eventWait(Event* t, u64 timeout)
{
    Result rc;
    u64 deadline = 0;

    if (t->revent == INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (timeout != U64_MAX)
        deadline = armGetSystemTick() + timeout * 12 / 625; // timeout: ns->ticks

    do {
        do {
            s64 this_timeout = -1;
            if (deadline) {
                this_timeout = deadline - armGetSystemTick();
                this_timeout = (this_timeout >= 0 ? this_timeout : 0) * 625 / 12; // ticks->ns
            }

            rc = svcWaitSynchronizationSingle(t->revent, this_timeout);
            if (deadline && (rc & 0x3FFFFF) == 0xEA01)
                return rc; // timeout.
        } while (R_FAILED(rc));

        if (t->autoclear)
            rc = svcResetSignal(t->revent);
    } while ((rc & 0x3FFFFF) == 0xFA01);

    return rc;
}

Result eventFire(Event* t)
{
    if (t->wevent == INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return svcSignalEvent(t->wevent);
}

Result eventClear(Event* t)
{
    if (t->wevent != INVALID_HANDLE)
        return svcClearEvent(t->wevent);

    if (t->revent != INVALID_HANDLE)
        return svcResetSignal(t->revent);

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
