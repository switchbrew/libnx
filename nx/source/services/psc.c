/**
 * @file psc.h
 * @brief PSC service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "services/psc.h"
#include "services/sm.h"
#include "runtime/hosversion.h"

static Service g_pscSrv;
static u64 g_refCnt;

static Result _pscPmModuleInitialize(PscPmModule *module, u16 module_id, const u16 *dependencies, size_t dependency_count, bool autoclear);
static Result _pscPmModuleAcknowledge(PscPmModule *module);
static Result _pscPmModuleAcknowledgeEx(PscPmModule *module, PscPmState state);

Result pscInitialize(void) {
    Result rc = 0;

    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_pscSrv))
        return rc;

    rc = smGetService(&g_pscSrv, "psc:m");

    if (R_SUCCEEDED(rc)) {
        rc = serviceConvertToDomain(&g_pscSrv);
    }

    if (R_FAILED(rc)) pscExit();

    return rc;
}

void pscExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_pscSrv);
    }
}

Result pscGetPmModule(PscPmModule *out, u16 module_id, const u16 *dependencies, size_t dependency_count, bool autoclear) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pscSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_pscSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pscSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->srv, &g_pscSrv, &r, 0);

            rc = _pscPmModuleInitialize(out, module_id, dependencies, dependency_count, autoclear);
            if (R_FAILED(rc)) {
                serviceClose(&out->srv);
            }
        }
    }

    return rc;
}

Result _pscPmModuleInitialize(PscPmModule *module, u16 module_id, const u16 *dependencies, size_t dependency_count, bool autoclear) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, dependencies, dependency_count * sizeof(u16), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 module_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&module->srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->module_id = module_id;

    Result rc = serviceIpcDispatch(&module->srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&module->srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(&module->event, r.Handles[0], autoclear);
            module->module_id = module_id;
        }
    }

    return rc;
}

Result pscPmModuleGetRequest(PscPmModule *module, PscPmState *out_state, u32 *out_flags) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&module->srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&module->srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 state;
            u32 flags;
        } *resp;

        serviceIpcParse(&module->srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_state = (PscPmState)resp->state;
            *out_flags = resp->flags;
        }
    }

    return rc;
}

Result pscPmModuleAcknowledge(PscPmModule *module, PscPmState state) {
    if (hosversionAtLeast(6,0,0)) {
        return _pscPmModuleAcknowledgeEx(module, state);
    } else {
        return _pscPmModuleAcknowledge(module);
    }
}

Result pscPmModuleFinalize(PscPmModule *module) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&module->srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;

    Result rc = serviceIpcDispatch(&module->srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&module->srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _pscPmModuleAcknowledge(PscPmModule *module) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&module->srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&module->srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&module->srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _pscPmModuleAcknowledgeEx(PscPmModule *module, PscPmState state) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 state;
    } *raw;

    raw = serviceIpcPrepareHeader(&module->srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    raw->state = state;

    Result rc = serviceIpcDispatch(&module->srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&module->srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
