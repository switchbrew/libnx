// Copyright 2018 plutoo
#include "result.h"
#include "kernel/svc.h"
#include "kernel/wait.h"
#include "kernel/waiter.h"
#include "kernel/utimer.h"
#include "kernel/uevent.h"
#include "arm/counter.h"
#include "../internal.h"

Result waitN(s32* idx_out, WaitObject* objects, size_t num_objects, u64 timeout)
{
    if (num_objects > MAX_WAIT)
        return MAKERESULT(Module_Libnx, LibnxError_TooManyWaitables);

    Handle dummy_handle = getThreadVars()->handle;

    Waiter waiter;
    _waiterCreate(&waiter);

    Handle handles[MAX_WAIT];
    u64 cur_tick = armGetSystemTick();

    u64 end_time = timeout;
    s32 end_time_idx = -1;
    size_t i;

    for (i=0; i<num_objects; i++)
    {
        WaitObject* obj = &objects[i];
        u64 timer_tick;

        switch (obj->type)
        {
        case WaitObjectType_UsermodeTimer:
            timer_tick = _utimerGetNextTime(obj->timer);

            // Skip timer if disabled.
            if (timer_tick == 0)
                break;

            // If the timer already signalled, we're done.
            if (timer_tick < cur_tick)
            {
                *idx_out = i;
                _utimerRecalculate(obj->timer, timer_tick);
                _waiterFree(&waiter, objects);
                return 0;
            }

            // Override the user-supplied timeout if timer would fire before that.
            if ((timer_tick - cur_tick) < end_time)
            {
                end_time = timer_tick - cur_tick;
                end_time_idx = i;
            }
            break;

        case WaitObjectType_UsermodeEvent:
            // If the event already happened, we're done.
            if (_ueventConsumeIfSignalled(obj->event))
            {
                *idx_out = i;
                _waiterFree(&waiter, objects);
                return 0;
            }

            // If not, add a listener.
            _waiterSubscribe(&waiter, obj->event);
            break;

        case WaitObjectType_Handle:
            break;
        }

        handles[i] = (obj->type == WaitObjectType_Handle) ? obj->handle : dummy_handle;
    }


    // Do the actual syscall.
    Result rc;
    rc = svcWaitSynchronization(idx_out, handles, num_objects, end_time);

    // Timeout-error?
    if (rc == 0xEA01)
    {
        // If the user-supplied timeout, we return the error back to them.
        if (end_time_idx == -1)
        {
            _waiterFree(&waiter, objects);
            return rc;
        }

        // If not, it means a timer was triggered.
        *idx_out = end_time_idx;
        _utimerRecalculate(objects[end_time_idx].timer, end_time + cur_tick);
        _waiterFree(&waiter, objects);
        return 0;
    }

    // Interrupted-error?
    if (rc == 0xEC01)
    {
        // An event was signalled.
        *idx_out = _waiterGetSignalledIndex(&waiter);
        _waiterFree(&waiter, objects);
        return 0;
    }

    _waiterFree(&waiter, objects);
    return rc;
}
