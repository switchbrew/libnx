#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/tmem.h"
#include "services/hiddbg.h"
#include "services/hid.h"
#include "services/sm.h"
#include "runtime/hosversion.h"

static Service g_hiddbgSrv;
static u64 g_hiddbgRefCnt;
static size_t g_hiddbgPtrbufsize;

static bool g_hiddbgHdlsInitialized;
static TransferMemory g_hiddbgHdlsTmem;

static const u32 g_hiddbgDeviceTypeInternalTable[] = {
    BIT(20),    // DeviceType 0 Invalid
    BIT(0*4+2), // DeviceType 1 JoyRight
    BIT(0*4+1), // DeviceType 2 JoyLeft
    BIT(1*4+0), // DeviceType 3 FullKey
    BIT(1*4+1), // DeviceType 4 JoyLeft
    BIT(1*4+2), // DeviceType 5 JoyRight
    BIT(8),     // DeviceType 6 FullKey
    BIT(11),    // DeviceType 7 LarkLeft (HVC)
    BIT(12),    // DeviceType 8 LarkRight (HVC)
    BIT(13),    // DeviceType 9 LarkLeft (NES)
    BIT(14),    // DeviceType 10 LarkRight (NES)
    BIT(15),    // DeviceType 11 Invalid
    BIT(16),    // DeviceType 12 Palma (Invalid for DeviceTypeInternal)
    BIT(9),     // DeviceType 13 FullKey
    BIT(20),    // DeviceType 14 Invalid
    BIT(10),    // DeviceType 15 FullKey
    BIT(18),    // DeviceType 16 Invalid
    BIT(19),    // DeviceType 17 Invalid
    BIT(20),    // DeviceType 18 Invalid
    BIT(21),    // DeviceType 19 ::HidDeviceTypeBits_System with HidControllerType |= TYPE_PROCONTROLLER.
    BIT(22),    // DeviceType 20 ::HidDeviceTypeBits_System with HidControllerType |= TYPE_JOYCON_PAIR.
    BIT(23),    // DeviceType 21 ::HidDeviceType System with HidControllerType |= TYPE_JOYCON_PAIR.
};

Result hiddbgInitialize(void) {
    atomicIncrement64(&g_hiddbgRefCnt);

    if (serviceIsActive(&g_hiddbgSrv))
        return 0;

    Result rc = smGetService(&g_hiddbgSrv, "hid:dbg");

    if (R_SUCCEEDED(rc)) rc = ipcQueryPointerBufferSize(g_hiddbgSrv.handle, &g_hiddbgPtrbufsize);

    if (R_FAILED(rc)) hiddbgExit();

    return rc;
}

void hiddbgExit(void) {
    if (atomicDecrement64(&g_hiddbgRefCnt) == 0) {
        serviceClose(&g_hiddbgSrv);
    }
}

Service* hiddbgGetServiceSession(void) {
    return &g_hiddbgSrv;
}

static Result _hiddbgCmdNoIO(u64 cmd_id) {
    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hiddbgCmdInU8NoOut(u64 cmd_id, u8 val) {
    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 val;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->val = val;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hiddbgCmdInU64NoOut(u64 cmd_id, u64 val) {
    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 val;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->val = val;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgUpdateControllerColor(u32 colorBody, u32 colorButtons, u64 UniquePadId) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 colorBody;
        u32 colorButtons;
        u64 UniquePadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 221;
    raw->colorBody = colorBody;
    raw->colorButtons = colorButtons;
    raw->UniquePadId = UniquePadId;

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgUpdateDesignInfo(u32 colorBody, u32 colorButtons, u32 colorLeftGrip, u32 colorRightGrip, u8 inval, u64 UniquePadId) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 colorBody;
        u32 colorButtons;
        u32 colorLeftGrip;
        u32 colorRightGrip;
        u8 inval;
        u64 UniquePadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 224;
    raw->colorBody = colorBody;
    raw->colorButtons = colorButtons;
    raw->colorLeftGrip = colorLeftGrip;
    raw->colorRightGrip = colorRightGrip;
    raw->inval = inval;
    raw->UniquePadId = UniquePadId;

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hiddbgReadSerialFlash(TransferMemory *tmem, u32 offset, u64 size, u64 UniquePadId) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 offset;
        u64 size;
        u64 UniquePadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 229;
    raw->offset = offset;
    raw->size = size;
    raw->UniquePadId = UniquePadId;

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgReadSerialFlash(u32 offset, void* buffer, size_t size, u64 UniquePadId) {
    Result rc=0;
    TransferMemory tmem;
    size_t sizealign = (size+0x1000) & ~0xfff;

    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = tmemCreate(&tmem, sizealign, Perm_Rw);
    if (R_FAILED(rc)) return rc;

    rc = _hiddbgReadSerialFlash(&tmem, offset, size, UniquePadId);
    if (R_SUCCEEDED(rc)) memcpy(buffer, tmem.src_addr, size);
    tmemClose(&g_hiddbgHdlsTmem);
    return rc;
}

Result hiddbgGetUniquePadDeviceTypeSetInternal(u64 UniquePadId, u32 *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 UniquePadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 234;
    raw->UniquePadId = UniquePadId;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) { //Pre-9.0.0 output is an u32, with [9.0.0+] it's an u8.
            if (hosversionBefore(9,0,0))
                *out = resp->out;
            else
                *out = resp->out & 0xFF;
        }
    }

    return rc;
}

Result hiddbgGetAbstractedPadHandles(u64 *AbstractedPadHandles, s32 count, s32 *total_entries) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvStatic(&c, AbstractedPadHandles, sizeof(u64)*count, 0);

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 301;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_entries;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

Result hiddbgGetAbstractedPadState(u64 AbstractedPadHandle, HiddbgAbstractedPadState *state) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AbstractedPadHandle;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 302;
    raw->AbstractedPadHandle = AbstractedPadHandle;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            HiddbgAbstractedPadState state;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && state) memcpy(state, &resp->state, sizeof(*state));
    }

    return rc;
}

Result hiddbgGetAbstractedPadsState(u64 *AbstractedPadHandles, HiddbgAbstractedPadState *states, s32 count, s32 *total_entries) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    ipcAddRecvStatic(&c, AbstractedPadHandles, sizeof(u64)*count, 0);
    ipcAddRecvSmart(&c, g_hiddbgPtrbufsize, states, sizeof(HiddbgAbstractedPadState)*count, 0);

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 303;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 total_entries;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

Result hiddbgSetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId, const HiddbgAbstractedPadState *state) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s8 AbstractedVirtualPadId;
        u8 pad[7];
        HiddbgAbstractedPadState state;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 321;
    raw->AbstractedVirtualPadId = AbstractedVirtualPadId;
    memcpy(&raw->state, state, sizeof(*state));

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgUnsetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s8 AbstractedVirtualPadId;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 322;
    raw->AbstractedVirtualPadId = AbstractedVirtualPadId;

    rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgUnsetAllAutoPilotVirtualPadState(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hiddbgCmdNoIO(323);
}

static u32 _hiddbgConvertDeviceTypeToDeviceTypeInternal(u8 deviceType) {
    if (deviceType >= sizeof(g_hiddbgDeviceTypeInternalTable)/sizeof(u32)) return g_hiddbgDeviceTypeInternalTable[0];
    return g_hiddbgDeviceTypeInternalTable[deviceType];
}

static u8 _hiddbgConvertDeviceTypeInternalToDeviceType(u32 deviceType) {
    for (u32 i=0; i<sizeof(g_hiddbgDeviceTypeInternalTable)/sizeof(u32); i++) {
        if (deviceType == g_hiddbgDeviceTypeInternalTable[i]) return i;
    }
    return 0;
}

static void _hiddbgConvertHdlsDeviceInfoToV7(HiddbgHdlsDeviceInfoV7 *out, const HiddbgHdlsDeviceInfo *in) {
    memset(out, 0, sizeof(*out));

    out->deviceTypeInternal = _hiddbgConvertDeviceTypeToDeviceTypeInternal(in->deviceType);
    out->singleColorBody = in->singleColorBody;
    out->singleColorButtons = in->singleColorButtons;
    out->npadInterfaceType = in->npadInterfaceType;
}

static void _hiddbgConvertHdlsDeviceInfoFromV7(HiddbgHdlsDeviceInfo *out, const HiddbgHdlsDeviceInfoV7 *in) {
    memset(out, 0, sizeof(*out));

    out->deviceType = _hiddbgConvertDeviceTypeInternalToDeviceType(in->deviceTypeInternal);
    out->npadInterfaceType = in->npadInterfaceType;
    out->singleColorBody = in->singleColorBody;
    out->singleColorButtons = in->singleColorButtons;
    //Leave out color*Grip at zero since V7 doesn't have those.
}

static void _hiddbgConverHiddbgHdlsStateToV7(HiddbgHdlsStateV7 *out, const HiddbgHdlsState *in) {
    memset(out, 0, sizeof(*out));

    out->powerConnected = (in->flags & BIT(0)) != 0;
    out->flags = (in->flags & BIT(1)) != 0;
    out->batteryCharge = in->batteryCharge;
    out->buttons = in->buttons;
    memcpy(out->joysticks, in->joysticks, sizeof(in->joysticks));
    out->unk_x20 = in->unk_x20;
}

static void _hiddbgConverHiddbgHdlsStateFromV7(HiddbgHdlsState *out, const HiddbgHdlsStateV7 *in) {
    memset(out, 0, sizeof(*out));

    out->batteryCharge = in->batteryCharge;
    out->flags = (in->powerConnected & 1) | ((in->flags & 1)<<1);
    out->buttons = in->buttons;
    memcpy(out->joysticks, in->joysticks, sizeof(in->joysticks));
    out->unk_x20 = in->unk_x20;
}

static void _hiddbgConvertHdlsStateListToV7(HiddbgHdlsStateListV7 *out, const HiddbgHdlsStateList *in) {
    s32 count;
    memset(out, 0, sizeof(*out));
    out->total_entries = in->total_entries;
    count = out->total_entries > 0x10 ? 0x10 : out->total_entries;

    for (s32 i=0; i<count; i++) {
        out->entries[i].HdlsHandle = in->entries[i].HdlsHandle;
        _hiddbgConvertHdlsDeviceInfoToV7(&out->entries[i].device, &in->entries[i].device);
        _hiddbgConverHiddbgHdlsStateToV7(&out->entries[i].state, &in->entries[i].state);
    }
}

static void _hiddbgConvertHdlsStateListFromV7(HiddbgHdlsStateList *out, const HiddbgHdlsStateListV7 *in) {
    s32 count;
    memset(out, 0, sizeof(*out));
    out->total_entries = in->total_entries;
    count = out->total_entries > 0x10 ? 0x10 : out->total_entries;

    for (s32 i=0; i<count; i++) {
        out->entries[i].HdlsHandle = in->entries[i].HdlsHandle;
        _hiddbgConvertHdlsDeviceInfoFromV7(&out->entries[i].device, &in->entries[i].device);
        _hiddbgConverHiddbgHdlsStateFromV7(&out->entries[i].state, &in->entries[i].state);
    }
}

static Result _hiddbgAttachHdlsWorkBuffer(TransferMemory *tmem) {
    IpcCommand c;
    ipcInitialize(&c);

    ipcSendHandleCopy(&c, tmem->handle);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 324;
    raw->size = tmem->size;

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgAttachHdlsWorkBuffer(void) {
    Result rc=0;

    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);


    rc = tmemCreate(&g_hiddbgHdlsTmem, 0x1000, Perm_Rw);
    if (R_FAILED(rc)) return rc;

    rc = _hiddbgAttachHdlsWorkBuffer(&g_hiddbgHdlsTmem);
    if (R_FAILED(rc)) tmemClose(&g_hiddbgHdlsTmem);
    if (R_SUCCEEDED(rc)) g_hiddbgHdlsInitialized = true;
    return rc;
}

Result hiddbgReleaseHdlsWorkBuffer(void) {
    Result rc=0;

    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    g_hiddbgHdlsInitialized = false;

    rc = _hiddbgCmdNoIO(325);
    tmemClose(&g_hiddbgHdlsTmem);
    return rc;
}

Result hiddbgDumpHdlsNpadAssignmentState(HiddbgHdlsNpadAssignment *state) {
    Result rc=0;

    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _hiddbgCmdNoIO(326);
    if (R_FAILED(rc)) return rc;
    if (state) memcpy(state, g_hiddbgHdlsTmem.src_addr, sizeof(*state));
    return rc;
}

Result hiddbgDumpHdlsStates(HiddbgHdlsStateList *state) {
    Result rc=0;

    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _hiddbgCmdNoIO(327);
    if (R_FAILED(rc)) return rc;
    if (state) {
        if (hosversionBefore(9,0,0)) {
            HiddbgHdlsStateListV7 statev7;
            memcpy(&statev7, g_hiddbgHdlsTmem.src_addr, sizeof(statev7));
            _hiddbgConvertHdlsStateListFromV7(state, &statev7);
        }
        else
            memcpy(state, g_hiddbgHdlsTmem.src_addr, sizeof(*state));
    }
    return rc;
}

Result hiddbgApplyHdlsNpadAssignmentState(const HiddbgHdlsNpadAssignment *state, bool flag) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (state==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memcpy(g_hiddbgHdlsTmem.src_addr, state, sizeof(*state));
    return _hiddbgCmdInU8NoOut(328, flag!=0);
}

Result hiddbgApplyHdlsStateList(const HiddbgHdlsStateList *state) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (state==NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (hosversionBefore(9,0,0)) {
        HiddbgHdlsStateListV7 statev7;
        _hiddbgConvertHdlsStateListToV7(&statev7, state);
        memcpy(g_hiddbgHdlsTmem.src_addr, &statev7, sizeof(statev7));
    }
    else
        memcpy(g_hiddbgHdlsTmem.src_addr, state, sizeof(*state));

    return _hiddbgCmdNoIO(329);
}

static Result _hiddbgAttachHdlsVirtualDeviceV7(u64 *HdlsHandle, const HiddbgHdlsDeviceInfoV7 *info) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        HiddbgHdlsDeviceInfoV7 info;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 330;
    raw->info = *info;

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 handle;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && HdlsHandle) *HdlsHandle = resp->handle;
    }

    return rc;
}

static Result _hiddbgAttachHdlsVirtualDevice(u64 *HdlsHandle, const HiddbgHdlsDeviceInfo *info) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        HiddbgHdlsDeviceInfo info;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 330;
    raw->info = *info;

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 handle;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && HdlsHandle) *HdlsHandle = resp->handle;
    }

    return rc;
}

Result hiddbgAttachHdlsVirtualDevice(u64 *HdlsHandle, const HiddbgHdlsDeviceInfo *info) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(9,0,0)) {
        HiddbgHdlsDeviceInfoV7 infov7;
        _hiddbgConvertHdlsDeviceInfoToV7(&infov7, info);
        return _hiddbgAttachHdlsVirtualDeviceV7(HdlsHandle, &infov7);
    }
    else
        return _hiddbgAttachHdlsVirtualDevice(HdlsHandle, info);
}

Result hiddbgDetachHdlsVirtualDevice(u64 HdlsHandle) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _hiddbgCmdInU64NoOut(331, HdlsHandle);
}

static Result _hiddbgSetHdlsState(u64 HdlsHandle, const HiddbgHdlsState *state) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        union {
            struct {
                HiddbgHdlsStateV7 state;
                u64 handle;
            } v7; // [7.0.0-8.1.0]
            struct {
                u64 handle;
                HiddbgHdlsState state;
            } v9; // [9.0.0+]
        };
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 332;
    if (hosversionBefore(9,0,0)) {
        _hiddbgConverHiddbgHdlsStateToV7(&raw->v7.state, state);
        raw->v7.handle = HdlsHandle;
    }
    else {
        raw->v9.handle = HdlsHandle;
        memcpy(&raw->v9.state, state, sizeof(*state));
    }

    Result rc = serviceIpcDispatch(&g_hiddbgSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hiddbgSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hiddbgSetHdlsState(u64 HdlsHandle, const HiddbgHdlsState *state) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _hiddbgSetHdlsState(HdlsHandle, state);
}

