/**
 * @file avm.h
 * @brief AVM services IPC wrapper. Only available on [6.0.0+].
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../sf/service.h"

typedef struct {
    u64 application_id;
    u32 version;
    u32 required;
} AvmVersionListEntry;

typedef struct {
    u64 application_id;
    u32 version;
} AvmRequiredVersionEntry;

typedef struct {
    Service s;
} AvmVersionListImporter;

Result avmInitialize(void);
void avmExit(void);

Service *avmGetServiceSession(void);

Result avmGetHighestAvailableVersion(u64 id_1, u64 id_2, u32 *version);
Result avmGetHighestRequiredVersion(u64 id_1, u64 id_2, u32 *version);
Result avmGetVersionListEntry(u64 application_id, AvmVersionListEntry *entry);
Result avmGetVersionListImporter(AvmVersionListImporter *out);
Result avmGetLaunchRequiredVersion(u64 application_id, u32 *version);
Result avmUpgradeLaunchRequiredVersion(u64 application_id, u32 version);
Result avmPushLaunchVersion(u64 application_id, u32 version);
Result avmListVersionList(AvmVersionListEntry *buffer, size_t count, u32 *out);
Result avmListRequiredVersion(AvmRequiredVersionEntry *buffer, size_t count, u32 *out);

void avmVersionListImporterClose(AvmVersionListImporter *srv);
Result avmVersionListImporterSetTimestamp(AvmVersionListImporter *srv, u64 timestamp);
Result avmVersionListImporterSetData(AvmVersionListImporter *srv, const AvmVersionListEntry *entries, u32 count);
Result avmVersionListImporterFlush(AvmVersionListImporter *srv);
