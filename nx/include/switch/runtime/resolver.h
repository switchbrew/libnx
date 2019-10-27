#pragma once
#include "../types.h"

/// Fetches the last resolver Switch result code of the current thread.
Result resolverGetLastResult(void);

/// Retrieves a handle used to cancel the next resolver command on the current thread.
u32 resolverGetCancelHandle(void);

/// Retrieves whether service discovery is enabled for resolver commands on the current thread.
bool resolverGetEnableServiceDiscovery(void);

/// [5.0.0+] Retrieves whether the DNS cache is used to resolve queries on the current thread (not implemented).
bool resolverGetEnableDnsCache(void);

/// Enables or disables service discovery for the current thread.
void resolverSetEnableServiceDiscovery(bool enable);

/// [5.0.0+] Enables or disables the usage of the DNS cache on the current thread (not implemented).
void resolverSetEnableDnsCache(bool enable);

/// Cancels a previous resolver command (handle obtained with \ref resolverGetCancelHandle prior to calling the command).
Result resolverCancel(u32 handle);

/// [5.0.0+] Removes a hostname from the DNS cache (not implemented).
Result resolverRemoveHostnameFromCache(const char* hostname);

/// [5.0.0+] Removes an IP address from the DNS cache (not implemented).
Result resolverRemoveIpAddressFromCache(u32 ip);
