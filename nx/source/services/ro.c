// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/ro.h"
#include "services/sm.h"

static Service g_roSrv, g_ro1Srv, g_dmntSrv;
static u64 g_roRefCnt, g_ro1RefCnt, g_dmntRefCnt;

static Result _rosrvInitialize(Service* srv);
static Result _rosrvLoadNro(Service* srv, u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
static Result _rosrvUnloadNro(Service* srv, u64 nro_address);
static Result _rosrvLoadNrr(Service* srv, u64 cmd_id, u64 nrr_address, u64 nrr_size);
static Result _rosrvUnloadNrr(Service* srv, u64 nrr_address);

Result ldrRoInitialize(void) {
    atomicIncrement64(&g_roRefCnt);

    if (serviceIsActive(&g_roSrv))
        return 0;

    Result rc = smGetService(&g_roSrv, "ldr:ro");

    if (R_SUCCEEDED(rc)) {
        rc = _rosrvInitialize(&g_roSrv);
    }

    return rc;
}

void ldrRoExit(void) {
    if (atomicDecrement64(&g_roRefCnt) == 0)
        serviceClose(&g_roSrv);
}

Service* ldrRoGetServiceSession(void) {
    return &g_roSrv;
}

Result ro1Initialize(void) {
    if (hosversionBefore(7,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    atomicIncrement64(&g_ro1RefCnt);

    if (serviceIsActive(&g_ro1Srv))
        return 0;

    Result rc = smGetService(&g_ro1Srv, "ro:1");

    if (R_SUCCEEDED(rc)) {
        rc = _rosrvInitialize(&g_ro1Srv);
    }

    return rc;
}

void ro1Exit(void) {
    if (atomicDecrement64(&g_ro1RefCnt) == 0)
        serviceClose(&g_ro1Srv);
}

Service* ro1GetServiceSession(void) {
    return &g_ro1Srv;
}

Result roDmntInitialize(void) {
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    atomicIncrement64(&g_dmntRefCnt);

    if (serviceIsActive(&g_dmntSrv))
        return 0;

    return smGetService(&g_dmntSrv, "ro:dmnt");
}

void roDmntExit(void) {
    if (atomicDecrement64(&g_dmntRefCnt) == 0)
        serviceClose(&g_dmntSrv);
}

Service* roDmntGetServiceSession(void) {
    return &g_dmntSrv;
}

Result _rosrvInitialize(Service* srv) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendHandleCopy(&c, CUR_PROCESS_HANDLE);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result _rosrvLoadNro(Service* srv, u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 nro_address;
        u64 nro_size;
        u64 bss_address;
        u64 bss_size;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->pid = 0;
    raw->nro_address = nro_address;
    raw->nro_size = nro_size;
    raw->bss_address = bss_address;
    raw->bss_size = bss_size;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 out_address;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_address) *out_address = resp->out_address;
        }
    }

    return rc;
}

Result _rosrvUnloadNro(Service* srv, u64 nro_address) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 nro_address;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = 0;
    raw->nro_address = nro_address;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result _rosrvLoadNrr(Service* srv, u64 cmd_id, u64 nrr_address, u64 nrr_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 nrr_address;
        u64 nrr_size;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->pid = 0;
    raw->nrr_address = nrr_address;
    raw->nrr_size = nrr_size;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result _rosrvUnloadNrr(Service* srv, u64 nrr_address) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 nrr_address;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->pid = 0;
    raw->nrr_address = nrr_address;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result ldrRoLoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    return _rosrvLoadNro(&g_roSrv, out_address, nro_address, nro_size, bss_address, bss_size);
}

Result ldrRoUnloadNro(u64 nro_address) {
    return _rosrvUnloadNro(&g_roSrv, nro_address);
}

Result ldrRoLoadNrr(u64 nrr_address, u64 nrr_size) {
    return _rosrvLoadNrr(&g_roSrv, 2, nrr_address, nrr_size);
}

Result ldrRoUnloadNrr(u64 nrr_address) {
    return _rosrvUnloadNrr(&g_roSrv, nrr_address);
}

Result ldrRoLoadNrrEx(u64 nrr_address, u64 nrr_size) {
    if (hosversionBefore(7,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _rosrvLoadNrr(&g_roSrv, 10, nrr_address, nrr_size);
}

Result ro1LoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    return _rosrvLoadNro(&g_ro1Srv, out_address, nro_address, nro_size, bss_address, bss_size);
}

Result ro1UnloadNro(u64 nro_address) {
    return _rosrvUnloadNro(&g_ro1Srv, nro_address);
}

Result ro1LoadNrr(u64 nrr_address, u64 nrr_size) {
    return _rosrvLoadNrr(&g_ro1Srv, 2, nrr_address, nrr_size);
}

Result ro1UnloadNrr(u64 nrr_address) {
    return _rosrvUnloadNrr(&g_ro1Srv, nrr_address);
}

Result ro1LoadNrrEx(u64 nrr_address, u64 nrr_size) {
    return _rosrvLoadNrr(&g_ro1Srv, 10, nrr_address, nrr_size);
}

Result roDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, u32 *num_out) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, out_module_infos, max_out_modules * sizeof(*out_module_infos), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->pid = pid;

    Result rc = serviceIpcDispatch(&g_dmntSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 num_out;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && num_out != NULL) {
            *num_out = resp->num_out;
        }
    }

    return rc;
}
