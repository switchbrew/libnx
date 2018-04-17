/**
 * @file lr.h
 * @brief Location Resolver (lr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/fs.h"

typedef struct {
    Service  s;
} LrLocationResolver;

typedef struct {
    Service  s;
} LrRegisteredLocationResolver;

Result lrInitialize(void);
void lrExit(void);

Result lrGetLocationResolver(FsStorageId storage, LrLocationResolver* out);
Result lrGetRegisteredLocationResolver(LrRegisteredLocationResolver* out);
// TODO: Other ILocationResolverManager commands

// ILocationResolver
Result lrLrGetProgramPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrSetProgramPath(LrLocationResolver* lr, u64 tid, const char *path);
Result lrLrGetControlPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrGetUserControlPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrSetControlPath(LrLocationResolver* lr, u64 tid, const char *path);
Result lrLrGetDocHtmlPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrSetDocHtmlPath(LrLocationResolver* lr, u64 tid, const char *path);
Result lrLrGetInfoHtmlPath(LrLocationResolver* lr, u64 tid, char *out);
Result lrLrSetInfoHtmlPath(LrLocationResolver* lr, u64 tid, const char *path);
Result lrLrClearOverridePaths(LrLocationResolver* lr);

// IRegisteredLocationResolver
Result lrRegLrGetProgramPath(LrRegisteredLocationResolver* reg, u64 tid, char *out);
// TODO: Other IRegisteredLocationResolver commands
