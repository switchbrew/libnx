#include "types.h"
#include "result.h"
#include "kernel/ipc.h"
#include "services/sm.h"
#include "services/sfdnsres.h"

#include <string.h>

static Result _sfdnsresDispatchCommand(IpcParsedCommand *r, IpcCommand *c, const void *raw, size_t raw_size) {
    Service sfdnsres;

    Result rc = smGetService(&sfdnsres, "sfdnsres");
    if(R_FAILED(rc)) goto cleanup;

    memcpy(ipcPrepareHeader(c, raw_size), raw, raw_size);

    rc = serviceIpcDispatch(&sfdnsres);
    if(R_FAILED(rc)) goto cleanup;

    ipcParse(r);

cleanup:
    serviceClose(&sfdnsres);
    return rc;
}

static Result _sfdnsresDispatchDnsRequest(IpcCommand *c, SfdnsresRequestResults *ret, const void *raw, size_t raw_size, bool has_serialized_data_out) {
    Result rc;
    IpcParsedCommand r;
    ipcSendPid(c);

    rc = _sfdnsresDispatchCommand(&r, c, raw, raw_size);
    if(R_FAILED(rc)) return rc;

    struct {
        u64 magic;
        u64 result;
        int ret;
        int errno_;
        int out_serialized_size; // OOB if !has_serialized_data
    } *resp = r.Raw;

    rc = resp->result;
    if(R_FAILED(rc)) return rc;

    ret->ret = resp->ret;
    ret->errno_ = resp->errno_;
    ret->out_serialized_size = has_serialized_data_out ? resp->out_serialized_size : 0;

    return rc;
}

static Result _sfdnsresDnsRequestCommand(IpcCommand *c, u64 cmd_id, SfdnsresRequestResults *ret, const SfdnsresConfig *config, bool has_serialized_data_out, int *arg) {
    struct {
        u64 magic;
        u64 cmd_id;
        int arg;
        int timeout;
        u64 pid_placeholder;
    } raw;

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = cmd_id;
    raw.arg = arg == NULL ? (config->bypass_nsd ? 0 : 1) : *arg;
    raw.timeout = config->timeout;
    raw.pid_placeholder = 0;

    return _sfdnsresDispatchDnsRequest(c, ret, &raw, sizeof(raw), has_serialized_data_out);
}

static Result _sfdnsresErrorStringGetterCommand(u64 cmd_id, int err, char *str, size_t str_size) {
    IpcCommand c;
    Result rc;
    IpcParsedCommand r;

    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, str, str_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        int err;
    } raw;

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = cmd_id;
    raw.err = err;

    rc = _sfdnsresDispatchCommand(&r, &c, &raw, sizeof(raw));
    if(R_FAILED(rc)) return rc;

    struct {
        u64 magic;
        u64 result;
    } *resp = r.Raw;

    rc = resp->result;
    return rc;
}

Result sfdnsresGetHostByName(SfdnsresRequestResults *ret, const SfdnsresConfig *config, void *out_he_serialized, const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, name, name == NULL ? 0 : strlen(name) + 1, 0);
    ipcAddRecvBuffer(&c, out_he_serialized, config->serialized_out_hostent_max_size, 0);

    return _sfdnsresDnsRequestCommand(&c, 2, ret, config, true, NULL);
}

Result sfdnsresGetHostByAddr(SfdnsresRequestResults *ret, const SfdnsresConfig *config, void *out_he_serialized, const void *addr, socklen_t len, int type) {
    IpcCommand c;
    struct {
        u64 magic;
        u64 cmd_id;
        socklen_t len; // wtf nintendo
        int type;
        int timeout;
        u64 pid_placeholder;
    } raw;

    ipcInitialize(&c);
    ipcAddSendBuffer(&c, addr, addr == NULL ? 0 : len, 0);
    ipcAddRecvBuffer(&c, out_he_serialized, config->serialized_out_hostent_max_size, 0);

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = 3;
    raw.len = len;
    raw.type = type;
    raw.timeout = config->timeout;
    raw.pid_placeholder = 0;

    return _sfdnsresDispatchDnsRequest(&c, ret, &raw, sizeof(raw), true);
}

Result sfdnsresGetHostStringError(int err, char *str, size_t str_size) {
    return _sfdnsresErrorStringGetterCommand(4, err, str, str_size);
}

Result sfdnsresGetGaiStringError(int err, char *str, size_t str_size) {
    return _sfdnsresErrorStringGetterCommand(5, err, str, str_size);
}

Result sfdnsresGetAddrInfo(SfdnsresRequestResults *ret, const SfdnsresConfig *config, const char *node, const char *service,
                           const void *hints_serialized, size_t hints_serialized_size, void *res_serialized) {
    IpcCommand c;
    size_t node_size = node == NULL ? 0 : strlen(node) + 1;
    size_t service_size = service == NULL ? 0 : strlen(service) + 1;
    hints_serialized_size = hints_serialized == NULL ? 0 : hints_serialized_size;

    ipcInitialize(&c);
    ipcAddSendBuffer(&c, node, node_size, 0);
    ipcAddSendBuffer(&c, service, service_size, 0);
    ipcAddSendBuffer(&c, hints_serialized, hints_serialized_size, 0);

    ipcAddRecvBuffer(&c, res_serialized, config->serialized_out_hostent_max_size, 0);

    return _sfdnsresDnsRequestCommand(&c, 6, ret, config, true, NULL);
}

Result sfdnsresGetNameInfo(SfdnsresRequestResults *ret, const SfdnsresConfig *config,
                           const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen,
                           char *serv, size_t servlen, int flags) {
    IpcCommand c;

    salen = sa == NULL ? 0 : salen;
    hostlen = host == NULL ? 0 : hostlen;
    servlen = serv == NULL ? 0 : servlen;

    ipcInitialize(&c);
    ipcAddSendBuffer(&c, sa, salen, 0);

    ipcAddRecvBuffer(&c, host, hostlen, 0);
    ipcAddRecvBuffer(&c, serv, servlen, 0);

    return _sfdnsresDnsRequestCommand(&c, 7, ret, config, false, &flags);
}

Result sfdnsresRequestCancelHandle(u32 *out_handle) {
    Result rc;
    IpcCommand c;
    IpcParsedCommand r;
    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid_placeholder;
    } raw;

    ipcInitialize(&c);
    ipcSendPid(&c);

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = 8;
    raw.pid_placeholder = 0;

    rc = _sfdnsresDispatchCommand(&r, &c, &raw, sizeof(raw));
    if(R_FAILED(rc)) return rc;

    struct {
        u64 magic;
        u64 result;
        u32 handle;
    } *resp = r.Raw;

    rc = resp->result;
    if(R_FAILED(rc)) return rc;

    *out_handle = resp->handle;

    return rc;
}

/// Bug: always sets errno ?
Result sfdnsresCancelSocketCall(SfdnsresRequestResults *ret, u32 handle) {
    IpcCommand c;
    struct {
        u64 magic;
        u64 cmd_id;
        u32 handle;
        u64 pid_placeholder;
    } raw;

    ipcInitialize(&c);

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = 9;
    raw.handle = handle;
    raw.pid_placeholder = 0;

    return _sfdnsresDispatchDnsRequest(&c, ret, &raw, sizeof(raw), false);
}

/// Bug: always sets errno ?
Result sfdnsresCancelAllSocketCalls(SfdnsresRequestResults *ret) {
    IpcCommand c;
    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid_placeholder;
    } raw;

    ipcInitialize(&c);

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = 10;
    raw.pid_placeholder = 0;

    return _sfdnsresDispatchDnsRequest(&c, ret, &raw, sizeof(raw), false);
}

Result sfdnsresClearDnsIpServerAddressArray(void) {
    Result rc;
    IpcCommand c;
    IpcParsedCommand r;
    struct {
        u64 magic;
        u64 cmd_id;
    } raw;

    ipcInitialize(&c);

    raw.magic = SFCI_MAGIC;
    raw.cmd_id = 11;

    rc = _sfdnsresDispatchCommand(&r, &c, &raw, sizeof(raw));
    if(R_FAILED(rc)) return rc;

    struct {
        u64 magic;
        u64 result;
    } *resp = r.Raw;

    rc = resp->result;
    return rc;
}
