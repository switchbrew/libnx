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
#include "../kernel/ipc.h"

/// Service type.
typedef enum {
    ServiceType_Uninitialized,      ///< Uninitialized service.
    ServiceType_Normal,             ///< Normal service.
    ServiceType_Domain,             ///< Domain.
    ServiceType_DomainSubservice,   ///< Domain subservice;
    ServiceType_Override,           ///< Service overriden in the homebrew environment.
} ServiceType;

/// Service object structure.
typedef struct {
    Handle handle;
    u32 object_id;
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
 * @brief Returns whether a service is a domain.
 * @param[in] s Service object.
 * @return true if a domain.
 */
static inline bool serviceIsDomain(Service* s) {
    return s->type == ServiceType_Domain;
}

/**
 * @brief Returns whether a service is a domain subservice.
 * @param[in] s Service object.
 * @return true if a domain subservice.
 */
static inline bool serviceIsDomainSubservice(Service* s) {
    return s->type == ServiceType_DomainSubservice;
}

/**
 * @brief For a domain/domain subservice, return the associated object ID.
 * @param[in] s Service object, necessarily a domain or domain subservice.
 * @return The object ID.
 */
static inline u32 serviceGetObjectId(Service* s) {
    return s->object_id;
}

/**
 * @brief Closes a domain object by ID.
 * @param[in] s Service object, necessarily a domain or domain subservice.
 * @param object_id ID of the object to close.
 * @return Result code.
 */
static inline Result serviceCloseObjectById(Service* s, u32 object_id) {
    return ipcCloseObjectById(s->handle, object_id);
}

/**
 * @brief Dispatches an IPC request to a service.
 * @param[in] s Service object.
 * @return Result code.
 */
static inline Result serviceIpcDispatch(Service* s) {
    return ipcDispatch(s->handle);
}

/**
 * @brief Creates a service object from an IPC session handle.
 * @param[out] s Service object.
 * @param[in] h IPC session handle.
 */
static inline void serviceCreate(Service* s, Handle h) {
    s->handle = h;
    s->type = ServiceType_Normal;
    s->object_id = IPC_INVALID_OBJECT_ID;
}

/**
 * @brief Creates a domain subservice object from a parent service.
 * @param[out] s Service object.
 * @param[in] parent Parent service, necessarily a domain or domain subservice.
 * @param[in] object_id Object ID for this subservice.
 */
static inline void serviceCreateDomainSubservice(Service* s, Service* parent, u32 object_id) {
    s->handle = parent->handle;
    s->type = ServiceType_DomainSubservice;
    s->object_id = object_id;
}

/**
 * @brief Creates a subservice object from a parent service.
 * @param[out] s Service object.
 * @param[in] parent Parent service, possibly a domain or domain subservice.
 * @param[in] r Parsed IPC command containing handles/object IDs to create subservice from.
 * @param[in] i The index of the handle/object ID to create subservice from.
 */
static inline void serviceCreateSubservice(Service* s, Service* parent, IpcParsedCommand* r, int i) {
    if (r->IsDomainResponse) {
        return serviceCreateDomainSubservice(s, parent, r->OutObjectIds[i]);
    } else {
        return serviceCreate(s, r->Handles[i]);
    }
}

/**
 * @brief Sends a service object with the specified cmd. This only supports domains.
 * @param[in] s Service object to send.
 * @param[in] cmd IPC command structure.
 */
static inline void serviceSendObject(Service* s, IpcCommand* cmd) {
    if (serviceIsDomain(s) || serviceIsDomainSubservice(s)) {
        ipcSendObjectId(cmd, s->object_id);
    }
}

/**
 * @brief Converts a regular service to a domain.
 * @param[in] s Service object.
 * @return Result code.
 */
static inline Result serviceConvertToDomain(Service* s) {
    Result rc = 0;
    if (serviceIsOverride(s)) {
        rc = ipcCloneSession(s->handle, 1, &s->handle);
        if (R_FAILED(rc)) {
            return rc;
        }
        s->type = ServiceType_Normal;
    }
    rc = ipcConvertSessionToDomain(s->handle, &s->object_id);
    if (R_SUCCEEDED(rc)) {
        s->type = ServiceType_Domain;
    }
    return rc;
}

/**
 * @brief Closes a service.
 * @param[in] s Service object.
 */
static inline void serviceClose(Service* s) {
    switch (s->type) {

    case ServiceType_Normal:
    case ServiceType_Domain:
        ipcCloseSession(s->handle);
        svcCloseHandle(s->handle);
        break;

    case ServiceType_DomainSubservice:
        serviceCloseObjectById(s, s->object_id);
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
 * @brief Prepares the header of an IPC command structure for a service.
 * @param s Service to prepare message header for
 * @param cmd IPC command structure.
 * @param sizeof_raw Size in bytes of the raw data structure to embed inside the IPC request
 * @return Pointer to the raw embedded data structure in the request, ready to be filled out.
 */
static inline void* serviceIpcPrepareHeader(Service* s, IpcCommand* cmd, size_t sizeof_raw) {
    if (serviceIsDomain(s) || serviceIsDomainSubservice(s)) {
        return ipcPrepareHeaderForDomain(cmd, sizeof_raw, serviceGetObjectId(s));
    } else {
        return ipcPrepareHeader(cmd, sizeof_raw);
    }
}

/**
 * @brief Parse an IPC command response into an IPC parsed command structure for a service.
 * @param s Service to prepare message header for
 * @param r IPC parsed command structure to fill in.
 * @param sizeof_raw Size in bytes of the raw data structure.
 * @return Result code.
 */
static inline Result serviceIpcParse(Service* s, IpcParsedCommand* r, size_t sizeof_raw) {
    if (serviceIsDomain(s) || serviceIsDomainSubservice(s)) {
        return ipcParseDomainResponse(r, sizeof_raw);
    } else {
        return ipcParse(r);
    }
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
 * @brief Gets the Service session used to communicate with SM.
 * @return Pointer to service session used to communicate with SM.
 */
Service *smGetServiceSession(void);

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
