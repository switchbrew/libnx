// Copyright 2018 plutoo
#include "result.h"
#include "kernel/svc.h"
#include "kernel/wait.h"
#include "kernel/utimer.h"
#include "kernel/uevent.h"
#include "arm/counter.h"
#include "wait.h"
#include "utimer.h"
#include "uevent.h"
#include "../internal.h"

#define MAX_WAIT 0x40

typedef Result (*WaitImplFunc)(s32* idx_out, void* objects, size_t num_objects, u64 timeout);

static Result waitImpl(s32* idx_out, Waiter* objects, size_t num_objects, u64 timeout)
{
    if (num_objects > MAX_WAIT)
        return KERNELRESULT(OutOfRange); // same error returned by kernel

    Handle own_thread_handle = getThreadVars()->handle;
    Handle dummy_handle = own_thread_handle;
    Result rc;

    Handle handles[num_objects];
    u64 cur_tick = armGetSystemTick();

    s32 triggered_idx = -1;
    u64 waiters_added = 0;
    WaiterNode waiters[num_objects];

    u64 end_tick = UINT64_MAX;
    s32 end_tick_idx = -1;
    size_t i;

    if (timeout != UINT64_MAX)
        end_tick = armNsToTicks(timeout);

    for (i = 0; i < num_objects; i ++) {
        Waiter* obj = &objects[i];
        u64 timer_tick;
        bool added;

        switch (obj->type) {
            case WaiterType_Handle:
            case WaiterType_HandleWithClear:
                // Add (real) handle to the array.
                handles[i] = obj->handle;
                break;

            case WaiterType_UTimer:
                timer_tick = _utimerGetNextTick(obj->timer);

                // Skip timer if stopped.
                if (timer_tick != 0) {
                    // If the timer already signalled, we're done.
                    if (timer_tick < cur_tick) {
                        _utimerRecalculate(obj->timer, timer_tick);

                        *idx_out = i;
                        rc = 0;
                        goto clean_up;
                    }

                    // Override the user-supplied timeout if timer would fire before that.
                    if ((timer_tick - cur_tick) < end_tick) {
                        end_tick = timer_tick - cur_tick;
                        end_tick_idx = i;
                    }
                }

                // Always add a listener on the timer,
                // If the timer is started/stopped we want to detect that.
                _waiterNodeInitialize(&waiters[i], &obj->timer->waitable, own_thread_handle, i, &triggered_idx);
                _utimerAddListener(obj->timer, &waiters[i]);

                waiters_added |= 1UL << i;
                handles[i] = dummy_handle;
                break;

            case WaiterType_UEvent:
                // Try to add a listener to the event, if it hasn't already signalled.
                _waiterNodeInitialize(&waiters[i], &obj->event->waitable, own_thread_handle, i, &triggered_idx);
                added = _ueventAddListener(obj->event, &waiters[i]);

                // If the event already happened, we're done.
                if (!added) {
                    *idx_out = i;
                    rc = 0;
                    goto clean_up;
                }

                // If the event hasn't signalled, we added a listener.
                waiters_added |= 1UL << i;
                handles[i] = dummy_handle;
                break;
        }
    }

    // Do the actual syscall.
    rc = svcWaitSynchronization(idx_out, handles, num_objects, end_tick==UINT64_MAX ? UINT64_MAX : armTicksToNs(end_tick));

    if (R_SUCCEEDED(rc)) {
        // Wait succeded, so that means an object having a real handle was signalled.
        // Perform autoclear if needed.
        if (objects[*idx_out].type == WaiterType_HandleWithClear) {
            // Try to auto-clear the event. If it is not signalled, the kernel
            // will return an error and thus we need to retry the wait.
            rc = svcResetSignal(handles[*idx_out]);
            if (R_VALUE(rc) == KERNELRESULT(InvalidState))
                rc = KERNELRESULT(Cancelled);
        }
    } else if (R_VALUE(rc) == KERNELRESULT(TimedOut)) {
        // If we hit the user-supplied timeout, we return the timeout error back to caller.
        if (end_tick_idx == -1)
            goto clean_up;

        // If not, it means a timer triggered the timeout.
        _utimerRecalculate(objects[end_tick_idx].timer, end_tick + cur_tick);

        *idx_out = end_tick_idx;
        rc = 0;
    } else if (R_VALUE(rc) == KERNELRESULT(Cancelled)) {
        // If no listener filled in its own index, we return the cancelled error back to caller.
        // This only happens if user for some reason manually does a svcCancelSynchronization.
        // Check just in case.
        if (triggered_idx == -1)
            goto clean_up;

        // An event was signalled, or a timer was updated.
        // So.. which is it?
        switch (objects[triggered_idx].type) {
            default:
                break;

            case WaiterType_UEvent:
                // Try to auto-clear the event. If auto-clear is enabled but
                // the event is not signalled, that means the state of the
                // event has changed and thus we need to retry the wait.
                rc = _ueventTryAutoClear(objects[triggered_idx].event);
                if (R_SUCCEEDED(rc))
                    *idx_out = triggered_idx;
                break;

            case WaiterType_UTimer:
                // Timer state changed, so we need to retry the wait.
                rc = KERNELRESULT(Cancelled);
                break;
        }
    }

clean_up:
    // Remove listeners.
    for (i = 0; i < num_objects; i ++)
        if (waiters_added & (1UL << i))
            _waiterNodeRemove(&waiters[i]);

    return rc;
}

static Result _waitLoop(s32* idx_out, void* objects, size_t num_objects, u64 timeout, WaitImplFunc waitfunc)
{
    Result rc;
    bool has_timeout = timeout != UINT64_MAX;
    u64 deadline = 0;

    if (has_timeout)
        deadline = armGetSystemTick() + armNsToTicks(timeout); // timeout: ns->ticks

    do {
        u64 this_timeout = UINT64_MAX;
        if (has_timeout) {
            s64 remaining = deadline - armGetSystemTick();
            this_timeout = remaining > 0 ? armTicksToNs(remaining) : 0; // ticks->ns
        }

        rc = waitfunc(idx_out, objects, num_objects, this_timeout);
        if (has_timeout && R_VALUE(rc) == KERNELRESULT(TimedOut))
            break;
    } while (R_VALUE(rc) == KERNELRESULT(Cancelled));

    return rc;
}

Result waitN(s32* idx_out, Waiter* objects, size_t num_objects, u64 timeout)
{
    return _waitLoop(idx_out, objects, num_objects, timeout, (WaitImplFunc)waitImpl);
}

Result waitNHandle(s32* idx_out, Handle* handles, size_t num_handles, u64 timeout)
{
    return _waitLoop(idx_out, handles, num_handles, timeout, (WaitImplFunc)svcWaitSynchronization);
}
