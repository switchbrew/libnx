#include "types.h"
#include "arm/atomics.h"
#include "services/hid.h"
#include "services/applet.h"
#include "services/nfpu.h"

static u64 g_refCnt;
static Service g_nfpuSrv;
static Service g_nfpuInterface;

static Result _nfpuCreateInterface(void);
static Result _nfpuInterfaceInitialize(u64 aruid, const NfpuInitConfig *config);
static Result _nfpuInterfaceFinalize(void);

static Result _nfpuInterfaceCmdNoInOut(u64 cmd_id);
static Result _nfpuInterfaceCmdInIdNoOut(u64 cmd_id, HidControllerID id);
static Result _nfpuInterfaceCmdInIdOutEvent(u64 cmd_id, HidControllerID id, Event *out);
static Result _nfpuInterfaceCmdInIdOutBuffer(u64 cmd_id, HidControllerID id, void *buf, size_t buf_size);

static Result _nfpuInterfaceInitialize(u64 aruid, const NfpuInitConfig *config);
static Result _nfpuInterfaceFinalize(void);

// This is the data passed by every application this was tested with
static const NfpuInitConfig g_nfpuDefaultInitConfig = {
    .unk1 = 0x00000001000a0003,
    .reserved1 = {0},
    .unk2 = 0x0000000300040003,
    .reserved2 = {0},
};

const NfpuInitConfig *nfpuGetDefaultInitConfig(void) {
    return &g_nfpuDefaultInitConfig;
}

Result nfpuInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_nfpuInterface))
        return 0;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    u64 aruid = 0;
    appletGetAppletResourceUserId(&aruid);

    Result rc = smGetService(&g_nfpuSrv, "nfp:user");

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfpuSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfpuCreateInterface();

    if (R_SUCCEEDED(rc))
        rc = _nfpuInterfaceInitialize(aruid, &g_nfpuDefaultInitConfig);

    if (R_FAILED(rc))
        nfpuExit();

    return rc;        
}

void nfpuExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        _nfpuInterfaceFinalize();
        serviceClose(&g_nfpuInterface);
        serviceClose(&g_nfpuSrv);
    }
}

Service *nfpuGetInterface(void) {
    return &g_nfpuInterface;
}

static Result _nfpuCreateInterface(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_nfpuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_nfpuSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc))
            serviceCreateSubservice(&g_nfpuInterface, &g_nfpuSrv, &r, 0);
    }

    return rc;
}

static Result _nfpuInterfaceCmdNoInOut(u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

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

static Result _nfpuInterfaceCmdInIdNoOut(u64 cmd_id, HidControllerID id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = _hidControllerIDToOfficial(id);

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

static Result _nfpuInterfaceCmdInIdOutEvent(u64 cmd_id, HidControllerID id, Event *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = _hidControllerIDToOfficial(id);

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

static Result _nfpuInterfaceCmdInIdOutBuffer(u64 cmd_id, HidControllerID id, void *buf, size_t buf_size) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcAddRecvStatic(&c, buf, buf_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->id = _hidControllerIDToOfficial(id);

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

static Result _nfpuInterfaceInitialize(u64 aruid, const NfpuInitConfig *config) {
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

    raw = serviceIpcPrepareHeader(&g_nfpuInterface, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->aruid = aruid;
    raw->zero = 0;

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

static inline Result _nfpuInterfaceFinalize(void) {
    return _nfpuInterfaceCmdNoInOut(1);
}

inline Result nfpuStartDetection(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(3, id);
}

inline Result nfpuStopDetection(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(4, id);
}

inline Result nfpuAttachActivateEvent(HidControllerID id, Event *out) {
    return _nfpuInterfaceCmdInIdOutEvent(17, id, out);
}

inline Result nfpuAttachDeactivateEvent(HidControllerID id, Event *out) {
    return _nfpuInterfaceCmdInIdOutEvent(18, id, out);
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
            eventLoadRemote(out, r.Handles[0], false);
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
    raw->id = _hidControllerIDToOfficial(id);

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
    #define MAX_CONTROLLERS 9

    IpcCommand c;
    ipcInitialize(&c);

    u64 buf[MAX_CONTROLLERS] = {0};
    ipcAddRecvStatic(&c, buf, MAX_CONTROLLERS * sizeof(u64), 0);

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
        if (R_SUCCEEDED(rc) && count && out) {
            *count = resp->count;
            for (u32 i=0; i<((num_elements>MAX_CONTROLLERS) ? MAX_CONTROLLERS:num_elements); i++)
                out[i] = _hidOfficialToControllerID(buf[i]);
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
    raw->id = _hidControllerIDToOfficial(id);

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
    raw->id = _hidControllerIDToOfficial(id);
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

inline Result nfpuUnmount(HidControllerID id) {
    return _nfpuInterfaceCmdInIdNoOut(6, id);
}

inline Result nfpuGetTagInfo(HidControllerID id, NfpuTagInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(13, id, out, sizeof(NfpuTagInfo));
}

inline Result nfpuGetRegisterInfo(HidControllerID id, NfpuRegisterInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(14, id, out, sizeof(NfpuRegisterInfo));
}

inline Result nfpuGetCommonInfo(HidControllerID id, NfpuCommonInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(15, id, out, sizeof(NfpuCommonInfo));
}

inline Result nfpuGetModelInfo(HidControllerID id, NfpuModelInfo *out) {
    return _nfpuInterfaceCmdInIdOutBuffer(16, id, out, sizeof(NfpuModelInfo));
}
