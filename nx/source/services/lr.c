// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/lr.h"
#include "services/fs.h"
#include "services/sm.h"

static Service g_managerSrv;
static u64 g_managerRefCnt;

Result lrInitialize(void) {
    atomicIncrement64(&g_managerRefCnt);
     
    if (serviceIsActive(&g_managerSrv)) {
        return 0;
    }
    
    return smGetService(&g_managerSrv, "lr");
}

void lrExit(void) {
    if (atomicDecrement64(&g_managerRefCnt) == 0) {
        serviceClose(&g_managerSrv);
    }
}

Result lrOpenLocationResolver(FsStorageId storage, LrLocationResolver* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 storage_id; // Actually u8
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->storage_id = (u32)storage;
    
    Result rc = serviceIpcDispatch(&g_managerSrv);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }
    
    return rc;
}

Result lrOpenRegisteredLocationResolver(LrRegisteredLocationResolver* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    
    Result rc = serviceIpcDispatch(&g_managerSrv);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }
    
    return rc;
}

/* 
    All the LocationResolver/RegisteredLocationResolver "Resolve" commands have a common API.
    This is a helper function to perform the work for those funcs, given a command ID.
*/
static Result _lrResolvePath(Service* s, u64 cmd_id, u64 tid, char *out) {
    char out_path[FS_MAX_PATH] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, out_path, FS_MAX_PATH, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 tid;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->tid = tid;
    
    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            strncpy(out, out_path, FS_MAX_PATH);
        }
    }
    
    return rc;
}

/* 
    All the LocationResolver/RegisteredLocationResolver "Redirect" commands have a common API.
    This is a helper function to perform the work for those funcs, given a command ID.
*/
static Result _lrRedirectPath(Service* s, u64 cmd_id, u64 tid, const char *path) {
    char send_path[FS_MAX_PATH+1] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, FS_MAX_PATH, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 tid;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->tid = tid;
    
    strncpy(send_path, path, FS_MAX_PATH);
    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }
    
    return rc;
}


Result lrLrResolveProgramPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, 0, tid, out);
}

Result lrLrRedirectProgramPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return _lrRedirectPath(&lr->s, 1, tid, path);
}

Result lrLrResolveApplicationControlPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, 2, tid, out);
}

Result lrLrResolveApplicationHtmlDocumentPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, 3, tid, out);
}

Result lrLrResolveDataPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, 4, tid, out);
}

Result lrLrRedirectApplicationControlPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return _lrRedirectPath(&lr->s, 5, tid, path);
}

Result lrLrRedirectApplicationHtmlDocumentPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return _lrRedirectPath(&lr->s, 6, tid, path);
}

Result lrLrResolveLegalInformationPath(LrLocationResolver* lr, u64 tid, char *out) {
    return _lrResolvePath(&lr->s, 7, tid, out);
}

Result lrLrRedirectLegalInformationPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return _lrRedirectPath(&lr->s, 8, tid, path);
}

Result lrLrRefresh(LrLocationResolver* lr) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    
    Result rc = serviceIpcDispatch(&lr->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }
    
    return rc;
}


Result lrRegLrResolveProgramPath(LrRegisteredLocationResolver* reg, u64 tid, char *out) {
    return _lrResolvePath(&reg->s, 0, tid, out);
}
