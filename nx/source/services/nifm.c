/**
 * @file nifm.c
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet
 * @copyright libnx Authors
 */

#include "services/nifm.h"
#include "arm/atomics.h"
#include "runtime/hosversion.h"
#include "services/applet.h"

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
    
    switch (appletGetAppletType()) {
    case AppletType_None:
        rc = smGetService(&g_nifmSrv, "nifm:s");
        break;

    case AppletType_Default:
    case AppletType_Application:
    case AppletType_SystemApplication:
    default:
        rc = smGetService(&g_nifmSrv, "nifm:u");
        break;

    case AppletType_SystemApplet:
    case AppletType_LibraryApplet:
    case AppletType_OverlayApplet:
        rc = smGetService(&g_nifmSrv, "nifm:a");
        break;
    }    

    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(3,0,0))
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
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;
}

Result nifmIsWirelessCommunicationEnabled(bool* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 17;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) 
            *out = resp->out != 0;
    }

    return rc;
}

Result nifmSetWirelessCommunicationEnabled(bool enable) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 value;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 16;
    raw->value = enable;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nifmIsEthernetCommunicationEnabled(bool* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) 
            *out = resp->out != 0;
    }

    return rc;
}

Result nifmIsAnyForegroundRequestAccepted(bool* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 22;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u8 out;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) 
            *out = resp->out != 0;
    }

    return rc;
}

Result nifmPutToSleep(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nifmWakeUp(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
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
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_nifmSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->param = in;

    Result rc = serviceIpcDispatch(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nifmSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreateSubservice(out, &g_nifmSrv, &r, 0);
    }

    return rc;
}

static Result _nifmCreateGeneralServiceOld(Service* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_nifmSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nifmSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreateSubservice(out, &g_nifmSrv, &r, 0);
    }

    return rc;
}
