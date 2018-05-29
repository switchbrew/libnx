#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/fs.h"
#include "services/sm.h"
#include "services/ncm.h"

static Service g_ncmSrv;

static u64 g_ncmRefCnt;

Result ncmInitialize() {
    atomicIncrement64(&g_ncmRefCnt);
    Result rc = smGetService(&g_ncmSrv, "ncm");
    return rc;
}

void ncmExit() {
    if (atomicDecrement64(&g_ncmRefCnt) == 0) {
        serviceClose(&g_ncmSrv);
    }
}

Result ncmOpenContentStorage(FsStorageId storage, NCMContentStorage* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 storage_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->storage_id = (u32)storage;
    
    Result rc = serviceIpcDispatch(&g_ncmSrv);
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

Result ncmContentStorageGetPath(NCMContentStorage* cs, const NCMNCAID* ncaId, char* out) {
    char out_path[FS_MAX_PATH] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, out_path, FS_MAX_PATH, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NCMNCAID ncaId;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    memcpy(&raw->ncaId, ncaId, sizeof(NCMNCAID));
    
    Result rc = serviceIpcDispatch(&cs->s);

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

Result ncmContentStorageGetSize(NCMContentStorage* cs, const NCMNCAID* ncaId, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NCMNCAID ncaId;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    memcpy(&raw->ncaId, ncaId, sizeof(NCMNCAID));
    
    Result rc = serviceIpcDispatch(&cs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->size;
        }
    }
    
    return rc;
}

Result ncmContentStorageReadContentIdFile(NCMContentStorage* cs, const NCMNCAID* ncaId, u64 offset, char* outBuf, size_t bufSize) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, outBuf, bufSize, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NCMNCAID ncaId;
        u64 offset;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;
    memcpy(&raw->ncaId, ncaId, sizeof(NCMNCAID));
    raw->offset = offset;
    
    Result rc = serviceIpcDispatch(&cs->s);

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
 
Result ncmOpenContentMetaDatabase(FsStorageId storage, NCMContentMetaDatabase* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 storage_id; // Actually u8
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->storage_id = (u32)storage;
    
    Result rc = serviceIpcDispatch(&g_ncmSrv);
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

Result ncmContentMetaDatabaseGetLatestContentMetaKey(NCMContentMetaDatabase* db, u64 titleID, NCMMetaRecord* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 title_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;
    raw->title_id = titleID;
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            NCMMetaRecord record;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            memcpy(out, &resp->record, sizeof(NCMMetaRecord));
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseGetContentIdByType(NCMContentMetaDatabase* db, NCMContentType contentType, const NCMMetaRecord* record, NCMNCAID* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 contentType;
        u32 padding0;
        NCMMetaRecord metaRecord;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->contentType = contentType;
    raw->padding0 = 0;
    memcpy(&raw->metaRecord, record, sizeof(NCMMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            NCMNCAID ncaId;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            memcpy(out, &resp->ncaId, sizeof(NCMNCAID));
        }
    }
    
    return rc;
}