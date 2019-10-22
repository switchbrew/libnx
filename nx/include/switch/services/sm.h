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
#include "../sf/service.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/**
 * @brief Closes a domain object by ID.
 * @param[in] s Service object, necessarily a domain or domain subservice.
 * @param object_id ID of the object to close.
 * @return Result code.
 */
DEPRECATED
static inline Result serviceCloseObjectById(Service* s, u32 object_id) {
    return ipcCloseObjectById(s->session, object_id);
}

/**
 * @brief Dispatches an IPC request to a service.
 * @param[in] s Service object.
 * @return Result code.
 */
DEPRECATED
static inline Result serviceIpcDispatch(Service* s) {
    return ipcDispatch(s->session);
}

/**
 * @brief Creates a subservice object from a parent service.
 * @param[out] s Service object.
 * @param[in] parent Parent service, possibly a domain or domain subservice.
 * @param[in] r Parsed IPC command containing handles/object IDs to create subservice from.
 * @param[in] i The index of the handle/object ID to create subservice from.
 */
DEPRECATED
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
DEPRECATED
static inline void serviceSendObject(Service* s, IpcCommand* cmd) {
    if (serviceIsDomain(s) || serviceIsDomainSubservice(s)) {
        ipcSendObjectId(cmd, s->object_id);
    }
}

/**
 * @brief Prepares the header of an IPC command structure for a service.
 * @param s Service to prepare message header for
 * @param cmd IPC command structure.
 * @param sizeof_raw Size in bytes of the raw data structure to embed inside the IPC request
 * @return Pointer to the raw embedded data structure in the request, ready to be filled out.
 */
DEPRECATED
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
DEPRECATED
static inline Result serviceIpcParse(Service* s, IpcParsedCommand* r, size_t sizeof_raw) {
    if (serviceIsDomain(s) || serviceIsDomainSubservice(s)) {
        return ipcParseDomainResponse(r, sizeof_raw);
    } else {
        return ipcParse(r);
    }
}

#pragma GCC diagnostic pop

/// Structure representing a service name (null terminated, remaining characters set to zero).
typedef struct SmServiceName {
    char name[8];
} SmServiceName;

/// Converts a service name into a 64-bit integer.
NX_CONSTEXPR u64 smServiceNameToU64(SmServiceName name)
{
    u64 ret = 0;
    __builtin_memcpy(&ret, &name, sizeof(u64));
    return ret;
}

/// Converts a 64-bit integer into a service name.
NX_CONSTEXPR SmServiceName smServiceNameFromU64(u64 name)
{
    SmServiceName ret = {0};
    __builtin_memcpy(&ret, &name, sizeof(SmServiceName));
    return ret;
}

/**
 * @brief Checks whether two service names are equal.
 * @param[in] a First name.
 * @param[in] b Second name.
 * @return Comparison result.
 */
NX_CONSTEXPR bool smServiceNamesAreEqual(SmServiceName a, SmServiceName b)
{
    return smServiceNameToU64(a) == smServiceNameToU64(b);
}

/**
 * @brief Encodes a service name string as a \ref SmServiceName structure.
 * @param[in] name Name of the service.
 * @return Encoded name.
 */
NX_CONSTEXPR SmServiceName smEncodeName(const char* name)
{
    SmServiceName name_encoded = {};
    unsigned len = __builtin_strlen(name);
#define __COPY_CHAR(_n) \
    if (len > _n) name_encoded.name[_n] = name[_n]
    __COPY_CHAR(0); __COPY_CHAR(1); __COPY_CHAR(2); __COPY_CHAR(3);
    __COPY_CHAR(4); __COPY_CHAR(5); __COPY_CHAR(6); __COPY_CHAR(7);
#undef __COPY_CHAR
    return name_encoded;
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
 * @brief Requests a service from SM, allowing overrides.
 * @param[out] service_out Service structure which will be filled in.
 * @param[in] name Name of the service to request.
 * @return Result code.
 */
Result smGetServiceWrapper(Service* service_out, SmServiceName name);

/**
 * @brief Requests a service from SM, as an IPC session handle directly
 * @param[out] handle_out Variable containing IPC session handle.
 * @param[in] name Name of the service to request.
 * @return Result code.
 */
Result smGetServiceOriginal(Handle* handle_out, SmServiceName name);

/**
 * @brief Requests a service from SM.
 * @param[out] service_out Service structure which will be filled in.
 * @param[in] name Name of the service to request (as a string).
 * @return Result code.
 */
NX_INLINE Result smGetService(Service* service_out, const char* name)
{
    return smGetServiceWrapper(service_out, smEncodeName(name));
}

/**
 * @brief Retrieves an overriden service in the homebrew environment.
 * @param[in] name Name of the service to request.
 * @return IPC session handle.
 */
Handle smGetServiceOverride(SmServiceName name);

/**
 * @brief Creates and registers a new service within SM.
 * @param[out] handle_out Variable containing IPC port handle.
 * @param[in] name Name of the service.
 * @param[in] is_light "Is light"
 * @param[in] max_sessions Maximum number of concurrent sessions that the service will accept.
 * @return Result code.
 */
Result smRegisterService(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions);

/**
 * @brief Unregisters a previously registered service in SM.
 * @param[in] name Name of the service.
 * @return Result code.
 */
Result smUnregisterService(SmServiceName name);

/**
 * @brief Gets the Service session used to communicate with SM.
 * @return Pointer to service session used to communicate with SM.
 */
Service *smGetServiceSession(void);

/**
 * @brief Overrides a service with a custom IPC service handle.
 * @param[in] name Name of the service.
 * @param[in] handle IPC session handle.
 */
void smAddOverrideHandle(SmServiceName name, Handle handle);
