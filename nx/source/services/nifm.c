/**
 * @file nifm.c
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet
 * @copyright libnx Authors
 */

#include "services/nifm.h"
#include "arm/atomics.h"
#include "runtime/hosversion.h"

static NifmServiceType g_nifmServiceType = NifmServiceType_NotInitialized;

static Service g_nifmSrv;
static Service g_nifmIGS;
static u64 g_refCnt;

static Result _nifmCreateGeneralService(Service* out, u64 in);
static Result _nifmCreateGeneralServiceOld(Service* out);

void nifmSetServiceType(NifmServiceType serviceType) {
    g_nifmServiceType = serviceType;
}

Result nifmInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_nifmSrv))
        return 0;

    Result rc = 0;
    switch (g_nifmServiceType) {
        case NifmServiceType_NotInitialized:
        case NifmServiceType_User:
            g_nifmServiceType = NifmServiceType_User;
            rc = smGetService(&g_nifmSrv, "nifm:u");
            break;
        case NifmServiceType_System:
            rc = smGetService(&g_nifmSrv, "nifm:s");
            break;
        case NifmServiceType_Admin:
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
        g_nifmServiceType = NifmServiceType_NotInitialized;
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
    if (g_nifmServiceType < NifmServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

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
    raw->value = enable!= 0;

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

Result nifmGetInternetConnectionStatus(NifmInternetConnectionType* connectionType, u32* wifiStrength, NifmInternetConnectionStatus* connectionStatus)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nifmIGS, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;

    Result rc = serviceIpcDispatch(&g_nifmIGS);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u8 out1;
            u8 out2;
            u8 out3;
        } PACKED *resp;

        serviceIpcParse(&g_nifmIGS, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            if (connectionType)
                *connectionType = resp->out1;

            if (wifiStrength)
                *wifiStrength = resp->out2;

            if (connectionStatus)
                *connectionStatus = resp->out3;
        }
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
