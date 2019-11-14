#include "kernel/svc.h"
#include "sf/cmif.h"
#include "sf/sessionmgr.h"

Result sessionmgrCreate(SessionMgr* mgr, Handle root_session, u32 num_sessions) {
    if (root_session == INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (num_sessions < 1 || num_sessions > NX_SESSION_MGR_MAX_SESSIONS)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (mgr->sessions[0] != INVALID_HANDLE)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    __builtin_memset(mgr, 0, sizeof(*mgr));
    mgr->sessions[0] = root_session;
    mgr->num_sessions = num_sessions;
    mgr->free_mask = (1U << num_sessions) - 1U;

    Result rc = 0;
    for (u32 i = 1; R_SUCCEEDED(rc) && i < num_sessions; i ++)
        rc = cmifCloneCurrentObject(root_session, &mgr->sessions[i]);

    return rc;
}

void sessionmgrClose(SessionMgr* mgr) {
    if (mgr->sessions[0] == INVALID_HANDLE)
        return;

    mgr->sessions[0] = INVALID_HANDLE;
    for (u32 i = 1; i < mgr->num_sessions; i ++) {
        if (mgr->sessions[i] != INVALID_HANDLE) {
            cmifMakeCloseRequest(armGetTls(), 0);
            svcSendSyncRequest(mgr->sessions[i]);
            svcCloseHandle(mgr->sessions[i]);
            mgr->sessions[i] = INVALID_HANDLE;
        }
    }
}

int sessionmgrAttachClient(SessionMgr* mgr) {
    mutexLock(&mgr->mutex);
    int slot;
    for (;;) {
        slot = __builtin_ffs(mgr->free_mask)-1;
        if (slot >= 0) break;
        mgr->is_waiting = true;
        condvarWait(&mgr->condvar, &mgr->mutex);
    }
    mgr->free_mask &= ~(1U << slot);
    mutexUnlock(&mgr->mutex);
    return slot;
}

void sessionmgrDetachClient(SessionMgr* mgr, int slot) {
    mutexLock(&mgr->mutex);
    mgr->free_mask |= 1U << slot;
    if (mgr->is_waiting) {
        mgr->is_waiting = false;
        condvarWakeOne(&mgr->condvar);
    }
    mutexUnlock(&mgr->mutex);
}
