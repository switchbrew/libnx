#pragma once
#include "../types.h"
#include "../kernel/mutex.h"
#include "../kernel/condvar.h"

#define NX_SESSION_MGR_MAX_SESSIONS 16

typedef struct SessionMgr
{
    Handle sessions[NX_SESSION_MGR_MAX_SESSIONS];
    u32 num_sessions;
    u32 free_mask;
    Mutex mutex;
    CondVar condvar;
    bool is_waiting;
} SessionMgr;

Result sessionmgrCreate(SessionMgr* mgr, Handle root_session, u32 num_sessions);
void sessionmgrClose(SessionMgr* mgr);
int sessionmgrAttachClient(SessionMgr* mgr);
void sessionmgrDetachClient(SessionMgr* mgr, int slot);

NX_CONSTEXPR Handle sessionmgrGetClientSession(SessionMgr* mgr, int slot)
{
    return mgr->sessions[slot];
}
