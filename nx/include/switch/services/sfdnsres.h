/**
 * @file sfdnsres.h
 * @brief Domain name resolution service IPC wrapper. Please use the standard <netdb.h> interface instead.
 * @author TuxSH
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

// SetDnsAddressesPrivateRequest & GetDnsAddressPrivateRequest are stubbed

Result sfdnsresGetHostByNameRequest(u32 cancel_handle, bool use_nsd, const char *name, u32 *h_errno_, u32 *errno_, void *out_buffer, size_t out_buffer_size, u32 *out_serialized_size);
Result sfdnsresGetHostByAddrRequest(const void *in_addr, size_t in_addr_len, u32 type, u32 cancel_handle, u32 *h_errno_, u32 *errno_, void *out_buffer, size_t out_buffer_size, u32 *out_serialized_size);
Result sfdnsresGetHostStringErrorRequest(u32 err, char *out_str, size_t out_str_size);
Result sfdnsresGetGaiStringErrorRequest(u32 err, char *out_str, size_t out_str_size);
Result sfdnsresGetAddrInfoRequest(u32 cancel_handle, bool use_nsd, const char *node, const char *service, const void *in_hints, size_t in_hints_size, void *out_buffer, size_t out_buffer_size, u32 *errno_, s32 *ret, u32 *out_serialized_size);
Result sfdnsresGetNameInfoRequest(u32 flags, const void *in_sa, size_t in_sa_size, char *out_host, size_t out_host_size, char *out_serv, size_t out_serv_len, u32 cancel_handle, u32 *errno_, s32 *ret);
Result sfdnsresGetCancelHandleRequest(u32 *out_handle);
Result sfdnsresCancelRequest(u32 handle);
