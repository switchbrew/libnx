#pragma once
#include "../types.h"

#include <sys/socket.h> // For socklen_t, etc.

/// Configuration structure for sfdnsres.
typedef struct {
    size_t serialized_out_addrinfos_max_size;   ///< For getaddrinfo.
    size_t serialized_out_hostent_max_size;     ///< For gethostbyname/gethostbyaddr.
    bool bypass_nsd;                            ///< For DNS requests: bypass the Name Server Daemon.
    int timeout;                                ///< For DNS requests: timeout or 0.
} SfdnsresConfig;

/// Result values returned by the DNS request commands.
typedef struct {
    int ret;                        ///< Return value (error code).
    int errno_;                     ///< Errno.
    ssize_t out_serialized_size;    ///< Size of the serialized output buffer or -1 (?).
} SfdnsresRequestResults;

// SetDnsAddressesPrivate & GetDnsAddressPrivate are stubbed

Result sfdnsresGetHostByName(SfdnsresRequestResults *ret, const SfdnsresConfig *config, void *out_he_serialized, const char *name);
Result sfdnsresGetHostByAddr(SfdnsresRequestResults *ret, const SfdnsresConfig *config, void *out_he_serialized, const void *addr, socklen_t len, int type);

Result sfdnsresGetHostStringError(char *str, size_t str_size);
Result sfdnsresGetGaiStringError(char *str, size_t str_size);

Result sfdnsresGetAddrInfo(SfdnsresRequestResults *ret, const SfdnsresConfig *config, const char *node, const char *service,
                           const void *hints_serialized, size_t hints_serialized_size, void *res_serialized);
Result sfdnsresGetNameInfo(SfdnsresRequestResults *ret, const SfdnsresConfig *config,
                           const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen,
                           char *serv, size_t servlen, int flags);

Result sfdnsresRequestCancelHandle(u32 *out_handle);
/// Bug: always sets errno ?
Result sfdnsresCancelSocketCall(SfdnsresRequestResults *ret, u32 handle);
/// Bug: always sets errno ?
Result sfdnsresCancelAllSocketCalls(SfdnsresRequestResults *ret);
Result sfdnsresClearDnsIpServerAddressArray(void); 
