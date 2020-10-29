#include "kernel/svc.h"
#include "runtime/diag.h"

__attribute__((weak)) void diagAbortWithResult(Result res)
{
    svcBreak(BreakReason_Panic, (uintptr_t)&res, sizeof(res));
    __builtin_unreachable();
}
