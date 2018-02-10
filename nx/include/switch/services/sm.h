/**
 * @file sm.h
 * @brief Service manager (sm) IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/svc.h"
#include "../ipc.h"

/// Service type.
typedef enum {
    ServiceType_Uninitialized, ///< Uninitialized service.
    ServiceType_Normal,        ///< Normal service.
    ServiceType_Override       ///< Service overriden in the homebrew environment.
} ServiceType;

/// Service object structure.
typedef struct {
    Handle handle;
    ServiceType type;
} Service;

/**
 * @brief Returns whether a service is overriden in the homebrew environment.
 * @param[in] s Service object.
 * @return true if overriden.
 */
static inline bool serviceIsOverride(Service* s) {
    return s->type == ServiceType_Override;
}

/**
 * @brief Returns whether a service has been initialized.
 * @param[in] s Service object.
 * @return true if initialized.
 */
static inline bool serviceIsActive(Service* s) {
    return s->type != ServiceType_Uninitialized;
}

/**
 * @brief Dispatches an IPC request to a service.
 * @param[in] s Service object.
 * @return Result code
 */
static inline Result serviceIpcDispatch(Service* s) {
    return ipcDispatch(s->handle);
}

/**
 * @brief Creates a service object from an IPC session handle.
 * @param[in] s Service object.
 * @param[in] h IPC session handle.
 */
static inline void serviceCreate(Service* s, Handle h) {
    s->handle = h;
    s->type = ServiceType_Normal;
}

/**
 * @brief Closes a service.
 * @param[in] s Service object.
 */
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

/**
 * @brief Initializes SM.
 * @return Result code.
 * @note This function is already called in the default application startup code (before main() is called).
 */
Result smInitialize(void);

/**
 * @brief Uninitializes SM.
 * @return Result code.
 * @note This function is already handled in the default application exit code (after main() returns).
 */
void   smExit(void);

/**
 * @brief Requests a service from SM.
 * @param[out] service_out Service structure which will be filled in.
 * @param[in] name Name of the service to request.
 * @return Result code.
 */
Result smGetService(Service* service_out, const char* name);

/**
 * @brief Requests a service from SM, as an IPC session handle directly
 * @param[out] handle_out Variable containing IPC session handle.
 * @param[in] name Name of the service to request.
 * @return Result code.
 */
Result smGetServiceOriginal(Handle* handle_out, u64 name);

/**
 * @brief Retrieves an overriden service in the homebrew environment.
 * @param[in] name Name of the service to request (as 64-bit integer).
 * @return IPC session handle.
 */
Handle smGetServiceOverride(u64 name);

/**
 * @brief Creates and registers a new service within SM.
 * @param[out] handle_out Variable containing IPC port handle.
 * @param[in] name Name of the service.
 * @param[in] is_light "Is light"
 * @param[in] max_sessions Maximum number of concurrent sessions that the service will accept.
 * @return Result code.
 */
Result smRegisterService(Handle* handle_out, const char* name, bool is_light, int max_sessions);

/**
 * @brief Unregisters a previously registered service in SM.
 * @param[in] name Name of the service.
 * @return Result code.
 */
Result smUnregisterService(const char* name);

/**
 * @brief Check whether SM is initialized.
 * @return true if initialized.
 */
bool   smHasInitialized(void);

/**
 * @brief Encodes a service name as a 64-bit integer.
 * @param[in] name Name of the service.
 * @return Encoded name.
 */ 
u64    smEncodeName(const char* name);

/**
 * @brief Overrides a service with a custom IPC service handle.
 * @param[in] name Name of the service (as 64-bit integer).
 * @param[in] handle IPC session handle.
 */
void   smAddOverrideHandle(u64 name, Handle handle);
