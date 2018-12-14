// Copyright 2018 plutoo
#include "result.h"
#include "kernel/svc.h"
#include "kernel/wait.h"
#include "kernel/utimer.h"
#include "kernel/uevent.h"
#include "arm/counter.h"
#include "utimer.h"
#include "uevent.h"
#include "wait.h"
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
                _utimerAddListener(
                    obj->timer, &waiters[i], i, &triggered_idx,
                    own_thread_handle);

                waiters_added |= 1ULL << i;
                break;

            case WaiterType_UEvent:
                // Try to add a listener to the event, if it hasn't already signalled.
                added = _ueventAddListener(
                    obj->event, &waiters[i], i, &triggered_idx,
                    own_thread_handle);

                // If the event already happened, we're done.
                if (!added) {
                    *idx_out = i;
                    rc = 0;
                    goto clean_up;
                }

                // If the event hasn't signalled, we added a listener.
                waiters_added |= 1ULL << i;
                break;

            case WaiterType_Handle:
                break;
        }

        // Add handle for i:th object.
        // If that object has no handle, add a dummy handle.
        handles[i] = (obj->type == WaiterType_Handle) ? obj->handle : dummy_handle;
    }

    // Do the actual syscall.
    rc = svcWaitSynchronization(idx_out, handles, num_objects, end_tick==UINT64_MAX ? UINT64_MAX : armTicksToNs(end_tick));
    rc = R_VALUE(rc);

    if (rc == KERNELRESULT(TimedOut)) {
        // If we hit the user-supplied timeout, we return the timeout error back to caller.
        if (end_tick_idx == -1)
            goto clean_up;

        // If not, it means a timer triggered the timeout.
        _utimerRecalculate(objects[end_tick_idx].timer, end_tick + cur_tick);

        *idx_out = end_tick_idx;
        rc = 0;
    } else if (rc == KERNELRESULT(Cancelled)) {
        // If no listener filled in its own index, we return the interrupt error back to caller.
        // This only happens if user for some reason manually does a svcCancelSynchronization.
        // Check just in case.
        if (triggered_idx == -1)
            goto clean_up;

        // An event was signalled, or a timer was updated.
        // So.. which is it?
        switch (waiters[triggered_idx].type) {
            case WaiterNodeType_Event:
                _ueventTryAutoClear(waiters[triggered_idx].parent_event);

                *idx_out = triggered_idx;
                rc = 0;
                break;

            case WaiterNodeType_Timer:
                rc = KERNELRESULT(Cancelled);
                break;
        }
    }

clean_up:
    // Remove listeners.
    for (i = 0; i < num_objects; i ++)
        if (waiters_added & (1ULL << i))
            _waiterNodeFree(&waiters[i]);

    return rc;
}

static Result _waitLoop(WaitImplFunc wait, s32* idx_out, void* objects, size_t num_objects, u64 timeout)
{
    Result rc;
    do {
        u64 cur_tick = armGetSystemTick();
        rc = wait(idx_out, objects, num_objects, timeout);
        rc = R_VALUE(rc);

        if (rc == KERNELRESULT(Cancelled)) {
            // On timer stop/start an interrupt is sent to listeners.
            // It means the timer state has changed, and we should restart the wait.

            // Adjust timeout..
            if (timeout != -1) {
                u64 time_spent = armTicksToNs(armGetSystemTick() - cur_tick);

                if (time_spent < timeout)
                    timeout -= time_spent;
                else
                    rc = KERNELRESULT(TimedOut);
            }
        }
    } while (rc == KERNELRESULT(Cancelled));

    return rc;
}

Result waitN(s32* idx_out, Waiter* objects, size_t num_objects, u64 timeout)
{
    return _waitLoop((WaitImplFunc)waitImpl, idx_out, objects, num_objects, timeout);
}

Result waitNHandle(s32* idx_out, Handle* handles, size_t num_handles, u64 timeout)
{
    return _waitLoop((WaitImplFunc)svcWaitSynchronization, idx_out, handles, num_handles, timeout);
}
