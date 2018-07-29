/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108
 * @copyright libnx Authors
 */

#include "services/nifm.h"
#include "arm/atomics.h"

static Service g_nifmSrv;
static Service g_nifmIGS;
static u64 g_refCnt;

static Result _nifmCreateGeneralService(Service* out, u64 in);
static Result _nifmCreateGeneralServiceOld(Service* out);

Result nifmInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_nifmSrv))
        return 0;
		
    Result rc;
    rc = smGetService(&g_nifmSrv, "nifm:u");

    if (R_SUCCEEDED(rc)) {
        if (kernelAbove200())
            rc = _nifmCreateGeneralService(&g_nifmIGS, 0); // What does this parameter do?
        else
            rc = _nifmCreateGeneralServiceOld(&g_nifmIGS);
    }

    if (R_FAILED(rc))
        nifmExit();

    return rc;
}

void nifmExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_nifmIGS);
        serviceClose(&g_nifmSrv);
    }
}

Result nifmGetCurrentIpAddress(u32* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } * raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out;
        }* resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;
}

Result nifmGetInternetConnectionStatus(NifmInternetConnectionStatus* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } * raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 out;
        }* resp = r.Raw;

        rc = resp->result;
        out->wirelessCommunicationEnabled = resp->out & 1;
        out->ethernetCommunicationEnabled = (resp->out >> 1) & 1;
    }

    return rc;
}

static Result _nifmCreateGeneralService(Service* out, u64 in) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 param;
    } PACKED* raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->param = in;

    Result rc = serviceIpcDispatch(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        }* resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreate(out, r.Handles[0]);
    }

    return rc;
}

static Result _nifmCreateGeneralServiceOld(Service* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } PACKED* raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        }* resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreate(out, r.Handles[0]);
    }

    return rc;
}
