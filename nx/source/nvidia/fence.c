#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "kernel/event.h"
#include "runtime/hosversion.h"
#include "services/nv.h"
#include "nvidia/fence.h"

static u32 g_ctrl_fd = -1;
static u64 g_refCnt;

static u64 g_NvEventUsedMask;
static Event g_NvEvents[64];

static int _nvGetEventSlot(void)
{
    int slot;
    u64 new_mask;
    u64 cur_mask = __atomic_load_n(&g_NvEventUsedMask, __ATOMIC_SEQ_CST);
    do {
        slot = __builtin_ffs(~cur_mask)-1;
        if (slot < 0) break;
        new_mask = cur_mask | ((u64)1 << slot);
    } while (!__atomic_compare_exchange_n(&g_NvEventUsedMask, &cur_mask, new_mask, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

    return slot;
}

static void _nvFreeEventSlot(int slot)
{
    u64 new_mask;
    u64 cur_mask = __atomic_load_n(&g_NvEventUsedMask, __ATOMIC_SEQ_CST);
    do
        new_mask = cur_mask &~ ((u64)1 << slot);
    while (!__atomic_compare_exchange_n(&g_NvEventUsedMask, &cur_mask, new_mask, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}

static Event* _nvGetEvent(int event_id)
{
    Result rc;
    Event* event = &g_NvEvents[event_id];
    if (event->revent != INVALID_HANDLE)
        return event;

    rc = nvioctlNvhostCtrl_EventRegister(g_ctrl_fd, event_id);
    if (R_FAILED(rc))
        return NULL;

    rc = nvQueryEvent(g_ctrl_fd, 0x10000000 | event_id, event);
    if (R_FAILED(rc)) {
        nvioctlNvhostCtrl_EventUnregister(g_ctrl_fd, event_id);
        return NULL;
    }

    return event;
}

static void _nvFreeEvent(int event_id)
{
    Event* event = &g_NvEvents[event_id];
    if (event->revent == INVALID_HANDLE)
        return;

    eventClose(event);
    nvioctlNvhostCtrl_EventUnregister(g_ctrl_fd, event_id);
}

Result nvFenceInit(void)
{
    Result rc;

    if (atomicIncrement64(&g_refCnt) > 0)
        return 0;

    rc = nvOpen(&g_ctrl_fd, "/dev/nvhost-ctrl");

    if (R_FAILED(rc))
        g_ctrl_fd = -1;

    return rc;
}

void nvFenceExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0) {
        for (int i = 0; i < 64; i ++)
            _nvFreeEvent(i);
        if (g_ctrl_fd != -1)
            nvClose(g_ctrl_fd);
        g_ctrl_fd = -1;
    }
}

static Result _nvFenceEventWaitCommon(Event* event, u32 event_id, s32 timeout_us)
{
    u64 timeout_ns = U64_MAX;
    if (timeout_us >= 0)
        timeout_ns = (u64)1000*timeout_us;
    Result rc = eventWait(event, timeout_ns);
    if ((rc & 0x3FFFFF) == 0xEA01) { // timeout
        nvioctlNvhostCtrl_EventSignal(g_ctrl_fd, event_id);
        rc = MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_Timeout);
    }
    return rc;
}

static Result _nvFenceWait_200(NvFence* f, s32 timeout_us)
{
    Result rc = MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_InsufficientMemory);
    int event_id = _nvGetEventSlot();
    if (event_id >= 0) {
        Event* event = _nvGetEvent(event_id);
        if (event) {
            rc = nvioctlNvhostCtrl_EventWaitAsync(g_ctrl_fd, f->id, f->value, timeout_us, event_id);
            if (rc == MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_Timeout))
                rc = _nvFenceEventWaitCommon(event, 0x10000000 | event_id, timeout_us);
        }
        _nvFreeEventSlot(event_id);
    }
    return rc;
}

static Result _nvFenceWait_100(NvFence* f, s32 timeout_us)
{
    u32 event_id;
    Result rc = nvioctlNvhostCtrl_EventWait(g_ctrl_fd, f->id, f->value, timeout_us, 0, &event_id);
    if (rc == MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_Timeout) && timeout_us) {
        Event event;
        rc = nvQueryEvent(g_ctrl_fd, event_id, &event);
        if (R_SUCCEEDED(rc)) {
            rc = _nvFenceEventWaitCommon(&event, event_id, timeout_us);
            eventClose(&event);
        }
    }
    return rc;
}

Result nvFenceWait(NvFence* f, s32 timeout_us)
{
    if (hosversionAtLeast(2,0,0))
        return _nvFenceWait_200(f, timeout_us);
    else
        return _nvFenceWait_100(f, timeout_us);
}

Result nvMultiFenceWait(NvMultiFence* mf, s32 timeout_us)
{
    // TODO: properly respect timeout
    Result rc = 0;
    for (u32 i = 0; i < mf->num_fences; i ++) {
        rc = nvFenceWait(&mf->fences[i], timeout_us);
        if (R_FAILED(rc))
            break;
    }
    return rc;
}
