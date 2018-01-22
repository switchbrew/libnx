#pragma once
#include "types.h"
#include "kernel/svc.h"
#include "ipc.h"

typedef enum {
    ServiceType_Uninitialized,
    ServiceType_Normal,
    ServiceType_Override
} ServiceType;

typedef struct {
    Handle handle;
    ServiceType type;
} Service;

static inline bool serviceIsOverride(Service* s) {
    return s->type == ServiceType_Override;
}

static inline bool serviceIsActive(Service* s) {
    return s->type != ServiceType_Uninitialized;
}

static inline Result serviceIpcDispatch(Service* s) {
    return ipcDispatch(s->handle);
}

static inline void serviceCreate(Service* s, Handle h) {
    s->handle = h;
    s->type = ServiceType_Normal;
}

static inline void serviceClose(Service* s) {
    switch (s->type) {

    case ServiceType_Normal:
        svcCloseHandle(s->handle);
        break;

    case ServiceType_Override:
        // Don't close because we don't own the overridden handle.
        break;

    case ServiceType_Uninitialized:
        break;
    }

    s->type = ServiceType_Uninitialized;
}

Result smInitialize(void);
void   smExit(void);
Result smGetService(Service* service_out, const char* name);
Result smGetServiceOriginal(Handle* handle_out, u64 name);
Handle smGetServiceOverride(u64 name);
Result smRegisterService(Handle* handle_out, const char* name, bool is_light, int max_sessions);
Result smUnregisterService(const char* name);
bool   smHasInitialized(void);
u64    smEncodeName(const char* name);
void   smAddOverrideHandle(u64 name, Handle handle);
