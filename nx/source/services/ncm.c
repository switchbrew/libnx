#include <string.h>
#include "services/ncm.h"
#include "arm/atomics.h"

static Service g_ncmSrv;
static u64 g_ncmRefCnt;

Result ncmInitialize(void) {
    atomicIncrement64(&g_ncmRefCnt);

    if (serviceIsActive(&g_ncmSrv))
        return 0;

    return smGetService(&g_ncmSrv, "ncm");
}

void ncmExit(void) {
    if (atomicDecrement64(&g_ncmRefCnt) == 0) {
        serviceClose(&g_ncmSrv);
    }
}

Result ncmOpenContentStorage(FsStorageId storage, NcmContentStorage* out) {
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

Result ncmOpenContentMetaDatabase(FsStorageId storage, NcmContentMetaDatabase* out) {
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

Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmNcaId* outputId) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    
    Result rc = serviceIpcDispatch(&cs->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            NcmNcaId outputId;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(outputId, &resp->outputId, sizeof(NcmNcaId));
        }
    }
    
    return rc;	
}

Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmNcaId* registeredId, const NcmNcaId* placeholderId, u64 size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId registeredId;
        NcmNcaId placeholderId;
        u64 size;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    memcpy(&raw->registeredId, registeredId, sizeof(NcmNcaId));
    memcpy(&raw->placeholderId, placeholderId, sizeof(NcmNcaId));
    raw->size = size;
    
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

Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholderId) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId placeholderId;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    memcpy(&raw->placeholderId, placeholderId, sizeof(NcmNcaId));
    
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

Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholderId, u64 offset, const void* srcData, size_t srcDataSize) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, srcData, srcDataSize, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId placeholderId;
        u64 offset;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    memcpy(&raw->placeholderId, placeholderId, sizeof(NcmNcaId));
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

Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmNcaId* registeredId, const NcmNcaId* placeholderId) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId registeredId;
        NcmNcaId placeholderId;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    memcpy(&raw->registeredId, registeredId, sizeof(NcmNcaId));
    memcpy(&raw->placeholderId, placeholderId, sizeof(NcmNcaId));
    
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

Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmNcaId* registeredId) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId registeredId;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 6;
    memcpy(&raw->registeredId, registeredId, sizeof(NcmNcaId));
    
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

Result ncmContentStorageHas(NcmContentStorage* cs, const NcmNcaId* ncaId, bool* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId nca_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;
    memcpy(&raw->nca_id, ncaId, sizeof(NcmNcaId));
    
    Result rc = serviceIpcDispatch(&cs->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            bool out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->out;
        }
    }
    
    return rc;
}

Result ncmContentStorageGetPath(NcmContentStorage* cs, const NcmNcaId* ncaId, char* out, size_t outSize) {
    char out_path[FS_MAX_PATH-1] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, out_path, FS_MAX_PATH-1, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId nca_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    memcpy(&raw->nca_id, ncaId, sizeof(NcmNcaId));
    
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
            if (outSize > FS_MAX_PATH-1) outSize = FS_MAX_PATH-1;
            strncpy(out, out_path, outSize);
        }
    }
    
    return rc;
}

Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, const NcmNcaId* ncaId, char* out, size_t outSize) {
    char out_path[FS_MAX_PATH-1] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, out_path, FS_MAX_PATH-1, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId nca_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    memcpy(&raw->nca_id, ncaId, sizeof(NcmNcaId));
    
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
            if (outSize > FS_MAX_PATH-1) outSize = FS_MAX_PATH-1;
            strncpy(out, out_path, outSize);
        }
    }
    
    return rc;
}

Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    
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

Result ncmContentStorageGetSize(NcmContentStorage* cs, const NcmNcaId* ncaId, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId nca_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    memcpy(&raw->nca_id, ncaId, sizeof(NcmNcaId));
    
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

Result ncmContentStorageDisableForcibly(NcmContentStorage* cs) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
    
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

Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, const NcmNcaId* ncaId, u64 offset, void* outBuf, size_t bufSize) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, outBuf, bufSize, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId nca_id;
        u64 offset;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;
    memcpy(&raw->nca_id, ncaId, sizeof(NcmNcaId));
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

Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, const NcmNcaId* ncaId, NcmRightsId* rightsIdOut, u32* keyGenerationOut) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        NcmNcaId nca_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;
    memcpy(&raw->nca_id, ncaId, sizeof(NcmNcaId));
    
    Result rc = serviceIpcDispatch(&cs->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            NcmRightsId rights_id;
            u32 key_generation;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (rightsIdOut) memcpy(rightsIdOut, &resp->rights_id, sizeof(NcmRightsId));
            if (keyGenerationOut) *keyGenerationOut = resp->key_generation;
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u64 inDataSize, const NcmContentMetaRecordsHeader* srcRecordsData) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, srcRecordsData, inDataSize, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        NcmMetaRecord meta_record;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    memcpy(&raw->meta_record, record, sizeof(NcmMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
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

Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u64 outDataSize, NcmContentMetaRecordsHeader* outRecordsData, u64* sizeRead) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, outRecordsData, outDataSize, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        NcmMetaRecord meta_record;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    memcpy(&raw->meta_record, record, sizeof(NcmMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size_read;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (sizeRead) *sizeRead = resp->size_read;
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmMetaRecord *record) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        NcmMetaRecord meta_record;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    memcpy(&raw->meta_record, record, sizeof(NcmMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
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

Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmContentType contentType, const NcmMetaRecord* record, NcmNcaId* out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 content_type;
        u32 padding;
        NcmMetaRecord meta_record;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->content_type = contentType;
    raw->padding = 0;
    memcpy(&raw->meta_record, record, sizeof(NcmMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            NcmNcaId nca_id;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            memcpy(out, &resp->nca_id, sizeof(NcmNcaId));
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u32 index, NcmContentRecord* contentRecordsOut, size_t contentRecordsBufSize, u32* numEntriesRead) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, contentRecordsOut, contentRecordsBufSize, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 index;
        NcmMetaRecord meta_record;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->index = index;
    memcpy(&raw->meta_record, record, sizeof(NcmMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 entries_read;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (numEntriesRead) *numEntriesRead = resp->entries_read;
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, u32 titleType, u64 titleIdExact, u64 titleIdLow, u64 titleIdHigh, NcmMetaRecord* metaRecordsOut, size_t metaRecordsBufSize, u32* numEntriesWritten, u32* numEntriesTotal) {
    IpcCommand c;
    ipcInitialize(&c);    
    ipcAddRecvBuffer(&c, metaRecordsOut, metaRecordsBufSize, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 titleType;
        u64 titleIdExact;
        u64 titleIdLow;
        u64 titleIdHigh;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->titleType = titleType;
    raw->titleIdExact = titleIdExact;
    raw->titleIdLow = titleIdLow;
    raw->titleIdHigh = titleIdHigh;
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 numEntriesTotal;
            u32 numEntriesWritten;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (numEntriesTotal)
                *numEntriesTotal = resp->numEntriesTotal;
            if (numEntriesWritten)
                *numEntriesWritten = resp->numEntriesWritten;
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, u64 titleId, NcmMetaRecord* out) {
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
    raw->title_id = titleId;
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            NcmMetaRecord record;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            memcpy(out, &resp->record, sizeof(NcmMetaRecord));
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, u8 filter, NcmApplicationContentMetaKey* outBuf, size_t outBufSize, u32* numEntriesWritten, u32* numEntriesTotal) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, outBuf, outBufSize, BufferType_Normal);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u8  filter;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;
    raw->filter = filter;
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 total_entries;
            u32 entries_written;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (numEntriesTotal) *numEntriesTotal = resp->total_entries;
            if (numEntriesWritten) *numEntriesWritten = resp->entries_written;
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, const NcmMetaRecord* record, bool* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        NcmMetaRecord meta_record;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    memcpy(&raw->meta_record, record, sizeof(NcmMetaRecord));
    
    Result rc = serviceIpcDispatch(&db->s);
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            bool out;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->out;
        }
    }
    
    return rc;
}

Result ncmContentMetaDatabaseDisableForcibly(NcmContentMetaDatabase* db) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;
    
    Result rc = serviceIpcDispatch(&db->s);
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

Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
    
    Result rc = serviceIpcDispatch(&db->s);
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
