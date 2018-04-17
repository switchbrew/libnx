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

Result lrGetLocationResolver(FsStorageId storage, LrLocationResolver* out) {
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

Result lrGetRegisteredLocationResolver(LrRegisteredLocationResolver* out) {
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

static Result lrUtilGetPath(Service* s, u64 cmd_id, u64 tid, char *out) {
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

static Result lrUtilSetPath(Service* s, u64 cmd_id, u64 tid, const char *path) {
    char send_path[FS_MAX_PATH] = {0};
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


Result lrLrGetProgramPath(LrLocationResolver* lr, u64 tid, char *out) {
    return lrUtilGetPath(&lr->s, 0, tid, out);
}

Result lrLrSetProgramPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return lrUtilSetPath(&lr->s, 1, tid, path);
}

Result lrLrGetUserControlPath(LrLocationResolver* lr, u64 tid, char *out) {
    return lrUtilGetPath(&lr->s, 2, tid, out);
}

Result lrLrGetControlPath(LrLocationResolver* lr, u64 tid, char *out) {
    return lrUtilGetPath(&lr->s, 4, tid, out);
}

Result lrLrSetControlPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return lrUtilSetPath(&lr->s, 5, tid, path);
}

Result lrLrGetDocHtmlPath(LrLocationResolver* lr, u64 tid, char *out) {
    return lrUtilGetPath(&lr->s, 3, tid, out);
}

Result lrLrSetDocHtmlPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return lrUtilSetPath(&lr->s, 6, tid, path);
}

Result lrLrGetInfoHtmlPath(LrLocationResolver* lr, u64 tid, char *out) {
    return lrUtilGetPath(&lr->s, 7, tid, out);
}

Result lrLrSetInfoHtmlPath(LrLocationResolver* lr, u64 tid, const char *path) {
    return lrUtilSetPath(&lr->s, 8, tid, path);
}

Result lrLrClearOverridePaths(LrLocationResolver* lr) {
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


Result lrRegLrGetProgramPath(LrRegisteredLocationResolver* reg, u64 tid, char *out) {
    return lrUtilGetPath(&reg->s, 0, tid, out);
}
