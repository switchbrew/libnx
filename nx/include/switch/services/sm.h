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
#include "../sf/service.h"
#include "../sf/tipc.h"

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

/// Same as \ref smRegisterService, but always using cmif serialization.
Result smRegisterServiceCmif(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions);

/// Same as \ref smRegisterService, but always using tipc serialization.
Result smRegisterServiceTipc(Handle* handle_out, SmServiceName name, bool is_light, s32 max_sessions);

/**
 * @brief Unregisters a previously registered service in SM.
 * @param[in] name Name of the service.
 * @return Result code.
 */
Result smUnregisterService(SmServiceName name);

/// Same as \ref smUnregisterService, but always using cmif serialization.
Result smUnregisterServiceCmif(SmServiceName name);

/// Same as \ref smUnregisterService, but always using tipc serialization.
Result smUnregisterServiceTipc(SmServiceName name);

/**
 * @brief Detaches the current SM session.
 * @note After this function is called, the rest of the SM API cannot be used.
 * @note Only available on [11.0.0-11.0.1], or Atmosphère.
 */
Result smDetachClient(void);

/// Same as \ref smDetachClient, but always using cmif serialization.
Result smDetachClientCmif(void);

/// Same as \ref smDetachClient, but always using tipc serialization.
Result smDetachClientTipc(void);

/**
 * @brief Gets the Service session used to communicate with SM.
 * @return Pointer to service session used to communicate with SM.
 */
Service *smGetServiceSession(void);

/**
 * @brief Gets the TipcService session used to communicate with SM.
 * @return Pointer to tipc service session used to communicate with SM.
 * @note Only available on [12.0.0+], or Atmosphère.
 */
TipcService *smGetServiceSessionTipc(void);

/**
 * @brief Overrides a service with a custom IPC service handle.
 * @param[in] name Name of the service.
 * @param[in] handle IPC session handle.
 */
void smAddOverrideHandle(SmServiceName name, Handle handle);
