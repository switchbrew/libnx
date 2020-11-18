/**
 * @file news.h
 * @brief News service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */

#pragma once

#include "../kernel/event.h"
#include "../services/acc.h"
#include "../sf/service.h"
#include "../types.h"

typedef enum {
    NewsServiceType_Administrator = 0, ///< Initializes news:a
    NewsServiceType_Configuration = 1, ///< Initializes news:c
    NewsServiceType_Manager       = 2, ///< Initializes news:m
    NewsServiceType_Post          = 3, ///< Initializes news:p
    NewsServiceType_Viewer        = 4, ///< Initializes news:v

    NewsServiceType_Count
} NewsServiceType;

typedef struct {
    char name[0x20];
} NewsTopicName;

typedef struct {
    Service s;
} NewsNewlyArrivedEventHolder;

typedef struct {
    Service s;
} NewsDataService;

typedef struct {
    Service s;
} NewsDatabaseService;

typedef struct {
    Service s;
} NewsOverwriteEventHolder;

typedef struct {
    char news_id[0x18];
    char user_id[0x18];
    s64 received_at;
    s32 read;
    s32 newly;
    s32 displayed;
} NewsRecordV1;

typedef struct {
    char news_id[0x18];
    char user_id[0x18];
    NewsTopicName topic_id;
    s64 received_at;
    s64 pad_0;
    s32 decoration_type;
    s32 read;
    s32 newly;
    s32 displayed;
    s32 feedback;
    s32 pad_1;
    s32 extra_1;
    s32 extra_2;
} NewsRecord;

Result newsInitialize(NewsServiceType service_type);
void newsExit(void);

Service *newsGetServiceSession(void);

Result newsCreateNewlyArrivedEventHolder(NewsNewlyArrivedEventHolder *out);
Result newsCreateNewsDataService(NewsDataService *out);
Result newsCreateNewsDatabaseService(NewsDatabaseService *out);
Result newsCreateOverwriteEventHolder(NewsOverwriteEventHolder *out); ///< [2.0.0+]

Result newsPostLocalNews(const void *news, size_t size);
Result newsSetPassphrase(u64 program_id, const char *passphrase);
Result newsGetSubscriptionStatus(const char *filter, u32 *status);
Result newsGetTopicList(u32 channel, u32 *out_count, NewsTopicName *out, u32 max_count); ///< [3.0.0+]
Result newsGetSavedataUsage(u64 *current, u64 *total); ///< [6.0.0+]
Result newsIsSystemUpdateRequired(bool *out);
Result newsGetDatabaseVersion(u32 *version); ///< [10.0.0+]
Result newsRequestImmediateReception(const char *filter);
Result newsSetSubscriptionStatus(const char *filter, u32 status);
Result newsClearStorage(void);
Result newsClearSubscriptionStatusAll(void);
Result newsGetNewsDatabaseDump(void *buffer, u64 size, u64 *out);

void newsNewlyArrivedEventHolderClose(NewsNewlyArrivedEventHolder *srv);
Result newsNewlyArrivedEventHolderGet(NewsNewlyArrivedEventHolder *srv, Event *out);

void newsDataClose(NewsDataService *srv);
Result newsDataOpen(NewsDataService *srv, const char *file_name);
Result newsDataOpenWithNewsRecordV1(NewsDataService *srv, NewsRecordV1 *record);
Result newsDataRead(NewsDataService *srv, u64 *bytes_read, u64 offset, void *out, size_t out_size);
Result newsDataGetSize(NewsDataService *srv, u64 *size);
Result newsDataOpenWithNewsRecord(NewsDataService *srv, NewsRecord *record); ///< [6.0.0+]

void newsDatabaseClose(NewsDatabaseService *srv);
Result newsDatabaseGetListV1(NewsDatabaseService *srv, NewsRecordV1 *out, u32 max_count, const char *where, const char *order, u32 *count, u32 offset);
Result newsDatabaseCount(NewsDatabaseService *srv, const char *filter, u32 *count);
Result newsDatabaseGetList(NewsDatabaseService *srv, NewsRecord *out, u32 max_count, const char *where, const char *order, u32 *count, u32 offset); ///< [6.0.0+]

void newsOverwriteEventHolderClose(NewsOverwriteEventHolder *srv);
Result newsOverwriteEventHolderGet(NewsOverwriteEventHolder *srv, Event *out);
