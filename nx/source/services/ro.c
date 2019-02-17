// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/ro.h"
#include "services/sm.h"

static Service g_roSrv, g_dmntSrv;
static u64 g_roRefCnt, g_dmntRefCnt;

static Result _ldrRoInitialize(void);

Result ldrRoInitialize(void) {
    atomicIncrement64(&g_roRefCnt);

    if (serviceIsActive(&g_roSrv))
        return 0;

    Result rc = smGetService(&g_roSrv, "ldr:ro");

    if (R_SUCCEEDED(rc)) {
        rc = _ldrRoInitialize();
    }

    return rc;
}

void ldrRoExit(void) {
    if (atomicDecrement64(&g_roRefCnt) == 0)
        serviceClose(&g_roSrv);
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

Result ldrRoLoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
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

    raw = serviceIpcPrepareHeader(&g_roSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->pid = 0;
    raw->nro_address = nro_address;
    raw->nro_size = nro_size;
    raw->bss_address = bss_address;
    raw->bss_size = bss_size;

    Result rc = serviceIpcDispatch(&g_roSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 out_address;
        } *resp;

        serviceIpcParse(&g_roSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (out_address) *out_address = resp->out_address;
        }
    }

    return rc;
}

Result ldrRoUnloadNro(u64 nro_address) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 nro_address;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_roSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = 0;
    raw->nro_address = nro_address;

    Result rc = serviceIpcDispatch(&g_roSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_roSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result ldrRoLoadNrr(u64 nrr_address, u64 nrr_size) {
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

    raw = serviceIpcPrepareHeader(&g_roSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->pid = 0;
    raw->nrr_address = nrr_address;
    raw->nrr_size = nrr_size;

    Result rc = serviceIpcDispatch(&g_roSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_roSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result ldrRoUnloadNrr(u64 nrr_address) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 nrr_address;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_roSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->pid = 0;
    raw->nrr_address = nrr_address;

    Result rc = serviceIpcDispatch(&g_roSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_roSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result _ldrRoInitialize(void) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendHandleCopy(&c, CUR_PROCESS_HANDLE);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_roSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_roSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_roSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result roDmntGetModuleInfos(u64 pid, LoaderModuleInfo *out_module_infos, size_t out_size, u32 *num_out) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvStatic(&c, out_module_infos, out_size, 0);

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
