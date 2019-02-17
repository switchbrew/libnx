#include <string.h>
#include "types.h"
#include "arm/atomics.h"
#include "runtime/hosversion.h"
#include "services/hid.h"
#include "services/applet.h"
#include "services/nfc.h"

static u64 g_refCnt;
static Service g_nfpuSrv;
static Service g_nfcuSrv;
static Service g_nfpuInterface;
static Service g_nfcuInterface;

// This is the data passed by every application this was tested with
static const NfpuInitConfig g_nfpuDefaultInitConfig = {
    .unk1 = 0x00000001000a0003,
    .reserved1 = {0},
    .unk2 = 0x0000000300040003,
    .reserved2 = {0},
};

static Result _nfpuCreateInterface(Service* srv, Service* out);
static Result _nfpuInterfaceInitialize(Service* srv, u64 cmd_id, u64 aruid, const NfpuInitConfig *config);
static Result _nfpuInterfaceFinalize(Service* srv, u64 cmd_id);

static Result _nfpuInterfaceCmdNoInOut(Service* srv, u64 cmd_id);
static Result _nfpuInterfaceCmdInIdNoOut(Service* srv, u64 cmd_id, HidControllerID id);
static Result _nfpuInterfaceCmdInIdOutEvent(Service* srv, u64 cmd_id, HidControllerID id, Event *out);
static Result _nfpuInterfaceCmdInIdOutBuffer(Service* srv, u64 cmd_id, HidControllerID id, void* buf, size_t buf_size);

Result nfpuInitialize(const NfpuInitConfig *config) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_nfpuInterface) && serviceIsActive(&g_nfcuInterface))
        return 0;

    if (config == NULL)
        config = &g_nfpuDefaultInitConfig;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    u64 aruid = 0;
    appletGetAppletResourceUserId(&aruid);

    // nfp:user
    Result rc = smGetService(&g_nfpuSrv, "nfp:user");

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfpuSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfpuCreateInterface(&g_nfpuSrv, &g_nfpuInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfpuInterfaceInitialize(&g_nfpuInterface, 0, aruid, config);

    // nfc:user
    if (R_SUCCEEDED(rc))
        rc = smGetService(&g_nfcuSrv, "nfc:user");

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfcuSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfpuCreateInterface(&g_nfcuSrv, &g_nfcuInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfpuInterfaceInitialize(&g_nfcuInterface, hosversionAtLeast(4,0,0) ? 0 : 400, aruid, config);

    if (R_FAILED(rc))
        nfpuExit();

    return rc;        
}

void nfpuExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        _nfpuInterfaceFinalize(&g_nfpuInterface, 1);
        _nfpuInterfaceFinalize(&g_nfcuInterface, hosversionAtLeast(4,0,0) ? 1 : 401);
        serviceClose(&g_nfpuInterface);
        serviceClose(&g_nfcuInterface);
        serviceClose(&g_nfpuSrv);
        serviceClose(&g_nfcuSrv);
    }
}

Service* nfpuGetInterface(void) {
    return &g_nfpuInterface;
}

static Result _nfpuCreateInterface(Service* srv, Service* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

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

        if (R_SUCCEEDED(rc))
            serviceCreateSubservice(out, srv, &r, 0);
    }

    return rc;
}

static Result _nfpuInterfaceCmdNoInOut(Service* srv, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

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

static Result _nfpuInterfaceCmdInIdNoOut(Service* srv, u64 cmd_id, HidControllerID id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = hidControllerIDToOfficial(id);

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

static Result _nfpuInterfaceCmdInIdOutEvent(Service* srv, u64 cmd_id, HidControllerID id, Event *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = hidControllerIDToOfficial(id);

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

        if (R_SUCCEEDED(rc))
            eventLoadRemote(out, r.Handles[0], false);
    }

    return rc;
}

static Result _nfpuInterfaceCmdInIdOutBuffer(Service* srv, u64 cmd_id, HidControllerID id, void* buf, size_t buf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvStatic(&c, buf, buf_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = hidControllerIDToOfficial(id);

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

static Result _nfpuInterfaceInitialize(Service* srv, u64 cmd_id, u64 aruid, const NfpuInitConfig *config) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, config, sizeof(NfpuInitConfig), BufferType_Normal);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 aruid;
        u64 zero;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->aruid = aruid;
    raw->zero = 0;

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

static Result _nfpuInterfaceFinalize(Service* srv, u64 cmd_id) {
    return _nfpuInterfaceCmdNoInOut(srv, cmd_id);
}

Result nfpuStartDetection(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(&g_nfpuInterface, 3, id);
}

Result nfpuStopDetection(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(&g_nfpuInterface, 4, id);
}

Result nfpuAttachActivateEvent(HidControllerID id, Event *out) {
    return _nfpuInterfaceCmdInIdOutEvent(&g_nfpuInterface, 17, id, out);
}

Result nfpuAttachDeactivateEvent(HidControllerID id, Event *out) {
    return _nfpuInterfaceCmdInIdOutEvent(&g_nfpuInterface, 18, id, out);
}

Result nfpuAttachAvailabilityChangeEvent(Event *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            eventLoadRemote(out, r.Handles[0], true);
    }

    return rc;
}

Result nfpuGetState(NfpuState *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 19;

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 state;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out)
            *out = resp->state;            
    }

    return rc;
}

Result nfpuGetDeviceState(HidControllerID id, NfpuDeviceState *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;
    raw->id = hidControllerIDToOfficial(id);

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 state;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out)
            *out = resp->state;            
    }

    return rc;
}

Result nfpuListDevices(u32 *count, HidControllerID *out, size_t num_elements) {
    // This is the maximum number of controllers that can be connected to a console at a time
    // Incidentally, this is the biggest value official software (SSBU) was observed using
    size_t max_controllers = 9;
    if (num_elements > max_controllers)
        num_elements = max_controllers;

    IpcCommand c;
    ipcInitialize(&c);

    u64 buf[max_controllers];
    memset(buf, 0, max_controllers * sizeof(u64));
    ipcAddRecvStatic(&c, buf, max_controllers * sizeof(u64), 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 count;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && count)
            *count = resp->count;

        if (R_SUCCEEDED(rc) && out) {
            for (size_t i=0; i<num_elements; i++)
                out[i] = hidControllerIDFromOfficial(buf[i]);
        }
    }

    return rc;
}

Result nfpuGetNpadId(HidControllerID id, u32 *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;
    raw->id = hidControllerIDToOfficial(id);

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 npad_id;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out)
            *out = resp->npad_id;
    }

    return rc;
}

Result nfpuMount(HidControllerID id, NfpuDeviceType device_type, NfpuMountTarget mount_target) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
        u32 device_type;
        u32 mount_target;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->id = hidControllerIDToOfficial(id);
    raw->device_type = device_type;
    raw->mount_target = mount_target;

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;          
    }

    return rc;
}

Result nfpuUnmount(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(&g_nfpuInterface, 6, id);
}

Result nfpuGetTagInfo(HidControllerID id, NfpuTagInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(&g_nfpuInterface, 13, id, out, sizeof(NfpuTagInfo));
}

Result nfpuGetRegisterInfo(HidControllerID id, NfpuRegisterInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(&g_nfpuInterface, 14, id, out, sizeof(NfpuRegisterInfo));
}

Result nfpuGetCommonInfo(HidControllerID id, NfpuCommonInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(&g_nfpuInterface, 15, id, out, sizeof(NfpuCommonInfo));
}

Result nfpuGetModelInfo(HidControllerID id, NfpuModelInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(&g_nfpuInterface, 16, id, out, sizeof(NfpuModelInfo));
}

Result nfpuOpenApplicationArea(HidControllerID id, u32 app_id, u32 *npad_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
        u32 app_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;
    raw->id = hidControllerIDToOfficial(id);
    raw->app_id = app_id;

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 npad_id;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result; 

        if (R_SUCCEEDED(rc) && npad_id)
            *npad_id = resp->npad_id;         
    }

    return rc;
}

Result nfpuGetApplicationArea(HidControllerID id, void* buf, size_t buf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvBuffer(&c, buf, buf_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->id = hidControllerIDToOfficial(id);

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;          
    }

    return rc;
}

Result nfpuSetApplicationArea(HidControllerID id, const void* buf, size_t buf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buf, buf_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->id = hidControllerIDToOfficial(id);

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;          
    }

    return rc;
}

Result nfpuCreateApplicationArea(HidControllerID id, u32 app_id, const void* buf, size_t buf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddSendBuffer(&c, buf, buf_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
        u32 app_id;
    } PACKED *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->id = hidControllerIDToOfficial(id);
    raw->app_id = app_id;

    Result rc = serviceIpcDispatch(&g_nfpuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nfpuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;          
    }

    return rc;
}

Result nfpuFlush(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(&g_nfpuInterface, 10, id);
}

Result nfpuRestore(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(&g_nfpuInterface, 11, id);
}

Result nfpuIsNfcEnabled(bool *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfcuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(2,0,0) ? 3 : 403;

    Result rc = serviceIpcDispatch(&g_nfcuInterface);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 flag;
        } *resp;

        serviceIpcParse(&g_nfcuInterface, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out)
            *out = !!resp->flag;
    }

    return rc;
}
