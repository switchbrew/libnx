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
    bool has_timeout = timeout != UINT64_MAX;
    u64 deadline = 0;

    if (t->revent == INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (has_timeout)
        deadline = armGetSystemTick() + armNsToTicks(timeout); // timeout: ns->ticks

    do {
        do {
            u64 this_timeout = UINT64_MAX;
            if (has_timeout) {
                s64 remaining = deadline - armGetSystemTick();
                this_timeout = remaining > 0 ? armTicksToNs(remaining) : 0; // ticks->ns
            }

            rc = svcWaitSynchronizationSingle(t->revent, this_timeout);
            if (has_timeout && R_VALUE(rc) == KERNELRESULT(TimedOut))
                return rc;
        } while (R_VALUE(rc) == KERNELRESULT(Cancelled));

        if (R_FAILED(rc))
            break;

        if (t->autoclear)
            rc = svcResetSignal(t->revent);
    } while (R_VALUE(rc) == KERNELRESULT(InvalidState));

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
