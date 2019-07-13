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

typedef enum {
    NcmContentMetaType_SystemProgram        = 0x01,
    NcmContentMetaType_SystemData           = 0x02,
    NcmContentMetaType_SystemUpdate         = 0x03,
    NcmContentMetaType_BootImagePackage     = 0x04,
    NcmContentMetaType_BootImagePackageSafe = 0x05,
    NcmContentMetaType_Application          = 0x80,
    NcmContentMetaType_Patch                = 0x81,
    NcmContentMetaType_AddOnContent         = 0x82,
    NcmContentMetaType_Delta                = 0x83,
} NcmContentMetaType;

typedef enum {
    NcmContentMetaAttribute_Exfat      = (1 << 0),
    NcmContentMetaAttribute_Rebootless = (1 << 1),
} NcmContentMetaAttribute;

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
    u16 numExtraBytes;     ///< Size of optional struct that comes after this one.
    u16 numContentRecords; ///< Number of NcmContentRecord entries after the extra bytes.
    u16 numMetaRecords;    ///< Number of NcmMetaRecord entries that come after the NcmContentRecords.
    u16 padding;           ///< Always zero.
} NcmContentMetaRecordsHeader;

typedef struct {
    NcmMetaRecord metaRecord;
    u64           baseTitleId;
} NcmApplicationContentMetaKey;

Result ncmInitialize(void);
void ncmExit(void);
Service* ncmGetServiceSession(void);

Result ncmOpenContentStorage(FsStorageId storage, NcmContentStorage* out);
Result ncmOpenContentMetaDatabase(FsStorageId storage, NcmContentMetaDatabase* out);

Result ncmContentStorageGeneratePlaceHolderId(NcmContentStorage* cs, NcmNcaId* outputId);
Result ncmContentStorageCreatePlaceHolder(NcmContentStorage* cs, const NcmNcaId* registeredId, const NcmNcaId* placeholderId, u64 size);
Result ncmContentStorageDeletePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholderId);
Result ncmContentStorageWritePlaceHolder(NcmContentStorage* cs, const NcmNcaId* placeholderId, u64 offset, const void* srcData, size_t srcDataSize);
Result ncmContentStorageRegister(NcmContentStorage* cs, const NcmNcaId* registeredId, const NcmNcaId* placeholderId);
Result ncmContentStorageDelete(NcmContentStorage* cs, const NcmNcaId* registeredId);
Result ncmContentStorageHas(NcmContentStorage* cs, const NcmNcaId* ncaId, bool* out);
Result ncmContentStorageGetPath(NcmContentStorage* cs, const NcmNcaId* ncaId, char* out, size_t outSize);
Result ncmContentStorageGetPlaceHolderPath(NcmContentStorage* cs, const NcmNcaId* ncaId, char* out, size_t outSize);
Result ncmContentStorageCleanupAllPlaceHolder(NcmContentStorage* cs);
Result ncmContentStorageGetSize(NcmContentStorage* cs, const NcmNcaId* ncaId, u64* out);
Result ncmContentStorageDisableForcibly(NcmContentStorage* cs);
Result ncmContentStorageReadContentIdFile(NcmContentStorage* cs, const NcmNcaId* ncaId, u64 offset, void* outBuf, size_t bufSize);
Result ncmContentStorageGetRightsIdFromContentId(NcmContentStorage* cs, const NcmNcaId* ncaId, FsRightsId* rightsIdOut, u32* keyGenerationOut);

Result ncmContentMetaDatabaseSet(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u64 inDataSize, const NcmContentMetaRecordsHeader* srcRecordsData);
Result ncmContentMetaDatabaseGet(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u64 outDataSize, NcmContentMetaRecordsHeader* outRecordsData, u64* sizeRead);
Result ncmContentMetaDatabaseRemove(NcmContentMetaDatabase* db, const NcmMetaRecord *record);
Result ncmContentMetaDatabaseGetContentIdByType(NcmContentMetaDatabase* db, NcmContentType contentType, const NcmMetaRecord* record, NcmNcaId* out);
Result ncmContentMetaDatabaseListContentInfo(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u32 index, NcmContentRecord* contentRecordsOut, size_t contentRecordsBufSize, u32* numEntriesRead);
Result ncmContentMetaDatabaseList(NcmContentMetaDatabase* db, u32 titleType, u64 titleIdExact, u64 titleIdLow, u64 titleIdHigh, NcmMetaRecord* metaRecordsOut, size_t metaRecordsBufSize, u32* numEntriesWritten, u32* numEntriesTotal);
Result ncmContentMetaDatabaseGetLatestContentMetaKey(NcmContentMetaDatabase* db, u64 titleId, NcmMetaRecord* out);
Result ncmContentMetaDatabaseListApplication(NcmContentMetaDatabase* db, u8 filter, NcmApplicationContentMetaKey* outBuf, size_t outBufSize, u32* numEntriesWritten, u32* numEntriesTotal);
Result ncmContentMetaDatabaseHas(NcmContentMetaDatabase* db, const NcmMetaRecord* record, bool* out);
Result ncmContentMetaDatabaseDisableForcibly(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseCommit(NcmContentMetaDatabase* db);
Result ncmContentMetaDatabaseGetAttributes(NcmContentMetaDatabase* db, const NcmMetaRecord* record, u8* out);
