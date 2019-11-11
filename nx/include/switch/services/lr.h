/**
 * @file lr.h
 * @brief Location Resolver (lr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/ncm_types.h"

typedef struct {
    Service  s;
} LrLocationResolver;

typedef struct {
    Service  s;
} LrRegisteredLocationResolver;

/// Initialize lr.
Result lrInitialize(void);

/// Exit lr.
void lrExit(void);

/// Gets the Service object for the actual lr service session.
Service* lrGetServiceSession(void);

Result lrOpenLocationResolver(NcmStorageId storage, LrLocationResolver* out);
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
