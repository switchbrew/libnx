/**
 * @file lr.h
 * @brief Location Resolver (lr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/fs.h"

typedef struct {
    Service  s;
} LrLocationResolver;

typedef struct {
    Service  s;
} LrRegisteredLocationResolver;

Result lrInitialize(void);
void lrExit(void);
Service* lrGetServiceSession(void);

Result lrOpenLocationResolver(FsStorageId storage, LrLocationResolver* out);
Result lrOpenRegisteredLocationResolver(LrRegisteredLocationResolver* out);
// TODO: Other ILocationResolverManager commands

// ILocationResolver
Result lrLrResolveProgramPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrRedirectProgramPath(LrLocationResolver* lr, u64 tid, const char *path);
Result lrLrResolveApplicationControlPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrResolveApplicationHtmlDocumentPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrResolveDataPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrRedirectApplicationControlPath(LrLocationResolver* lr, u64 tid, u64 tid2, const char *path);
Result lrLrRedirectApplicationHtmlDocumentPath(LrLocationResolver* lr, u64 tid, u64 tid2, const char *path);
Result lrLrResolveApplicationLegalInformationPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrRedirectApplicationLegalInformationPath(LrLocationResolver* lr, u64 tid, u64 tid2, const char *path);
Result lrLrRefresh(LrLocationResolver* lr);

// IRegisteredLocationResolver
Result lrRegLrResolveProgramPath(LrRegisteredLocationResolver* reg, u64 tid, char *out);
// TODO: Other IRegisteredLocationResolver commands
