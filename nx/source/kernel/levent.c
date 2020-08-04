#include "result.h"
#include "arm/counter.h"
#include "kernel/svc.h"
#include "kernel/levent.h"

/*
    Possible states for a light event's counter:

    0 - Unsignaled without waiters
    1 - Unsignaled with waiters
    2 - Signaled
*/

NX_INLINE u32 _LoadExclusive(u32 *ptr) {
    u32 value;
    __asm__ __volatile__("ldaxr %w[value], %[ptr]" : [value]"=&r"(value) : [ptr]"Q"(*ptr) : "memory");
    return value;
}

NX_INLINE bool _StoreExclusive(u32 *ptr, u32 value) {
    u32 result;
    __asm__ __volatile__("stlxr %w[result], %w[value], %[ptr]" : [result]"=&r"(result) : [value]"r"(value), [ptr]"Q"(*ptr) : "memory");
    return result != 0;
}

NX_INLINE void _ClearExclusive(void) {
    __asm__ __volatile__("clrex" ::: "memory");
}

static bool _leventTryWait(u32* counter) {
    u32 val = __atomic_load_n(counter, __ATOMIC_SEQ_CST);
    return val == 2;
}

static bool _leventTryWaitAndClear(u32* counter) {
    u32 val;

    do {
        val = _LoadExclusive(counter);
        if (val != 2) {
            // State isn't "signaled", so we fail.
            _ClearExclusive();
            return false;
        }
    } while (_StoreExclusive(counter, 0));

    return true;
}

static bool _leventWait(u32* counter, bool clear, u64 timeout_ns) {
    const bool has_timeout = timeout_ns != UINT64_MAX;
    u64 deadline = 0;

    if (has_timeout) {
        deadline = armGetSystemTick() + armNsToTicks(timeout_ns); // timeout: ns->ticks
    }

    u32 val = __atomic_load_n(counter, __ATOMIC_SEQ_CST);

    while (val != 2) {
        if (val == 0) {
            // State is "unsignaled without waiters" -> transition to "unsignaled with waiters"
            do {
                val = _LoadExclusive(counter);
                if (val != 0) {
                    // Unexpected state -> try again
                    _ClearExclusive();
                    break;
                }
            } while (_StoreExclusive(counter, (val = 1)));
        }
        else if (val == 1) {
            // State is "unsignaled with waiters" -> let's actually wait!
            s64 this_timeout = -1;
            if (has_timeout) {
                s64 remaining = deadline - armGetSystemTick();
                if (remaining <= 0)
                    return false; // whoops, timed out
                this_timeout = armTicksToNs(remaining); // ticks->ns
            }
            Result res = svcWaitForAddress(counter, ArbitrationType_WaitIfEqual, 1, this_timeout);
            if (R_FAILED(res)) {
                if (R_VALUE(res) == KERNELRESULT(TimedOut))
                    return false; // whoops, timed out
                if (R_VALUE(res) != KERNELRESULT(InvalidState))
                    svcBreak(BreakReason_Assert, 0, 0); // should not happen
            }

            if (clear) {
                // If clear is requested, transition to "unsignaled without waiters"
                do {
                    val = _LoadExclusive(counter);
                    if (val != 2) {
                        // Unexpected state -> try again
                        _ClearExclusive();
                        break;
                    }
                } while (_StoreExclusive(counter, 0));
            }
            else {
                // Otherwise the wait is done
                val = 2;
            }
        }
        else {
            // Invalid state - should not happen
            svcBreak(BreakReason_Assert, 0, 0);
        }
    }

    return true;
}

static Result _leventSignalImpl(u32* counter, SignalType type, s32 count) {
    u32 val;
    bool try_again;

    // Attempt to transition to "signaled" state
    do {
        try_again = true;
        val = _LoadExclusive(counter);
        if (val != 0) {
            // We are not in "unsignaled without waiters" state, so we can't use a RMW loop.
            _ClearExclusive();
            if (val == 1) {
                // State is "unsignaled with waiters" -> let's do it using the arbiter instead!
                Result res = svcSignalToAddress(counter, type, 1, count);
                if (R_SUCCEEDED(res))
                    return 0;
                if (R_VALUE(res) != KERNELRESULT(InvalidState))
                    return res; // shouldn't happen
                continue; // unexpected state -> try the RMW loop again
            }
            else if (val == 2) {
                // State is already "signaled" - so we don't need to do anything
                return 0;
            }
            else {
                // Invalid state - should not happen
                return KERNELRESULT(InvalidState);
            }
        }
        try_again = _StoreExclusive(counter, 2);
    } while (try_again);

    return 0;
}

static void _leventSignal(u32* counter, bool autoclear) {
    Result res;
    if (autoclear) {
        res = _leventSignalImpl(counter, SignalType_SignalAndModifyBasedOnWaitingThreadCountIfEqual, 1);
    } else {
        res = _leventSignalImpl(counter, SignalType_SignalAndIncrementIfEqual, -1);
    }

    if (R_FAILED(res))
        svcBreak(BreakReason_Assert, 0, 0); // should not happen
}

static void _leventClear(u32* counter) {
    u32 val;
    do {
        val = _LoadExclusive(counter);
        if (val != 2) {
            // State isn't "signaled" so we do nothing
            _ClearExclusive();
            break;
        }
    } while (_StoreExclusive(counter, 0));
}

bool leventWait(LEvent* le, u64 timeout_ns) {
    return _leventWait(&le->counter, le->autoclear, timeout_ns);
}

bool leventTryWait(LEvent* le) {
    if (le->autoclear) {
        return _leventTryWaitAndClear(&le->counter);
    } else {
        return _leventTryWait(&le->counter);
    }
}

void leventSignal(LEvent* le) {
    _leventSignal(&le->counter, le->autoclear);
}

void leventClear(LEvent* le) {
    _leventClear(&le->counter);
}
