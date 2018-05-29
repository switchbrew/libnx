/**
 * @file ncm.h
 * @brief Content Manager (ncm) service IPC wrapper.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/fs.h"
#include "../services/sm.h"
typedef enum NCMContentType {
    NCMContentType_CNMT = 0,
    NCMContentType_Program = 1,
    NCMContentType_Data = 2,
    NCMContentType_Icon = 3,
    NCMContentType_Doc = 4,
    NCMContentType_Info = 5,
} NCMContentType;

typedef struct {
    Service s;
} NCMContentStorage;

typedef struct {
    Service s;
} NCMContentMetaDatabase;

typedef struct {
    u64 titleID;    // 0x0
    u32 version;    // 0x8
    u8  type;       // 0xc
    u8  flags;      // 0xd
    u8  padding[2]; // 0xe
} NCMMetaRecord;

typedef struct {
    char c[0x10];
} NCMNCAID;

Result ncmInitialize();
void ncmExit();

Result ncmOpenContentStorage(FsStorageId storage, NCMContentStorage* out);
Result ncmContentStorageGetPath(NCMContentStorage* cs, const NCMNCAID* ncaId, char* out);
Result ncmContentStorageGetSize(NCMContentStorage* cs, const NCMNCAID* ncaId, u64* out);
Result ncmContentStorageReadContentIdFile(NCMContentStorage* cs, const NCMNCAID* ncaId, u64 offset, char* outBuf, size_t bufSize);

Result ncmOpenContentMetaDatabase(FsStorageId storage, NCMContentMetaDatabase* out);
Result ncmContentMetaDatabaseGetLatestContentMetaKey(NCMContentMetaDatabase* db, u64 titleID, NCMMetaRecord* out);
Result ncmContentMetaDatabaseGetContentIdByType(NCMContentMetaDatabase* db, NCMContentType contentType, const NCMMetaRecord* record, NCMNCAID* out);