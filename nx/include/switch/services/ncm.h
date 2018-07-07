/**
 * @file ncm.h
 * @brief Content Manager (ncm) service IPC wrapper.
 * @author zhuowei & Adubbz
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/fs.h"
#include "../services/sm.h"

typedef enum  {
    NcmContentType_CNMT = 0,
    NcmContentType_Program = 1,
    NcmContentType_Data = 2,
    NcmContentType_Icon = 3,
    NcmContentType_Doc = 4,
    NcmContentType_Info = 5,
} NcmContentType;

typedef struct {
    Service s;
} NcmContentStorage;

typedef struct {
    Service s;
} NcmContentMetaDatabase;

typedef struct {
    u8 c[0x10];
} NcmNcaId;

typedef struct {
    u64 titleId;
    u32 version;
    u8  type;
    u8  flags;
    u8  padding[2];
} NcmMetaRecord;

typedef struct {
    NcmNcaId  ncaId;
    u8        size[0x6];
    u8        type;
    u8        padding;
} NcmContentRecord;

typedef struct {
    NcmMetaRecord metaRecord;
    u64           baseTitleId;
} NcmApplicationContentMetaKey;

typedef struct {
    u8 c[0x10];
} NcmRightsId;

Result ncmInitialize(void);
void ncmExit(void);

Result ncmOpenContentStorage(FsStorageId storage, NcmContentStorage* out);
Result ncmOpenContentMetaDatabase(FsStorageId storage, NcmContentMetaDatabase* out);

Result ncmContentStorageHas(NcmContentStorage* cs, const NcmNcaId* ncaId, bool* out);
Result ncmContentStorageGetPath(NcmContentStorage* cs, const NcmNcaId* ncaId, char* out, size_t outSize);
Result ncmContentStorageGetSize(NcmContentStorage* cs, const NcmNcaId* ncaId, u64* out);
Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, const NcmNcaId* ncaId, u64 offset, void* outBuf, size_t bufSize);
Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, const NcmNcaId* ncaId, NcmRightsId* rightsIdOut, u32* keyGenerationOut);

Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u64 contentRecordSize, NcmContentRecord* contentRecords);
Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u64 contentRecordSize, NcmContentRecord* contentRecordsOut, u64* sizeRead);
Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmMetaRecord *record);
Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmContentType contentType, const NcmMetaRecord* record, NcmNcaId* out);
Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u32 index, NcmContentRecord* contentRecordsOut, size_t contentRecordsBufSize, u32* numEntriesRead);
Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, u64 titleId, NcmMetaRecord* out);
Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, u8 filter, NcmApplicationContentMetaKey* outBuf, size_t outBufSize, u32* numEntriesWritten, u32* numEntriesTotal);
Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, const NcmMetaRecord* record, bool* out);
Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db);
