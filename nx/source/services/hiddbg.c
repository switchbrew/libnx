#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include "kernel/tmem.h"
#include "services/hiddbg.h"
#include "runtime/hosversion.h"

static Service g_hiddbgSrv;

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

NX_GENERATE_SERVICE_GUARD(hiddbg);

Result _hiddbgInitialize(void) {
    return smGetService(&g_hiddbgSrv, "hid:dbg");
}

void _hiddbgCleanup(void) {
    serviceClose(&g_hiddbgSrv);
}

Service* hiddbgGetServiceSession(void) {
    return &g_hiddbgSrv;
}

static Result _hiddbgCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_hiddbgSrv, cmd_id);
}

static Result _hiddbgCmdInU8NoOut(u8 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hiddbgSrv, cmd_id, inval);
}

static Result _hiddbgCmdInBoolNoOut(bool inval, u32 cmd_id) {
    return _hiddbgCmdInU8NoOut(inval!=0, cmd_id);
}

static Result _hiddbgCmdInU64NoOut(u64 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hiddbgSrv, cmd_id, inval);
}

static Result _hiddbgCmdInHandle64NoOut(Handle handle, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hiddbgSrv, cmd_id, inval,
        .in_num_handles = 1,
        .in_handles = { handle },
    );
}

static Result _hiddbgCmdInTmemNoOut(TransferMemory *tmem, u32 cmd_id) {
    return _hiddbgCmdInHandle64NoOut(tmem->handle, tmem->size, cmd_id);
}

Result hiddbgSetDebugPadAutoPilotState(const HiddbgDebugPadAutoPilotState *state) {
    return serviceDispatchIn(&g_hiddbgSrv, 1, *state);
}

Result hiddbgUnsetDebugPadAutoPilotState(void) {
    return _hiddbgCmdNoIO(2);
}

Result hiddbgSetTouchScreenAutoPilotState(const HidTouchState *states, s32 count) {
    return serviceDispatch(&g_hiddbgSrv, 11,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { states, count*sizeof(HidTouchState) } },
    );
}

Result hiddbgUnsetTouchScreenAutoPilotState(void) {
    return _hiddbgCmdNoIO(12);
}

Result hiddbgSetMouseAutoPilotState(const HiddbgMouseAutoPilotState *state) {
    return serviceDispatchIn(&g_hiddbgSrv, 21, *state);
}

Result hiddbgUnsetMouseAutoPilotState(void) {
    return _hiddbgCmdNoIO(22);
}

Result hiddbgSetKeyboardAutoPilotState(const HiddbgKeyboardAutoPilotState *state) {
    return serviceDispatchIn(&g_hiddbgSrv, 31, *state);
}

Result hiddbgUnsetKeyboardAutoPilotState(void) {
    return _hiddbgCmdNoIO(32);
}

Result hiddbgDeactivateHomeButton(void) {
    return _hiddbgCmdNoIO(110);
}

Result hiddbgUpdateControllerColor(u32 colorBody, u32 colorButtons, HidsysUniquePadId unique_pad_id) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 colorBody;
        u32 colorButtons;
        HidsysUniquePadId unique_pad_id;
    } in = { colorBody, colorButtons, unique_pad_id };

    return serviceDispatchIn(&g_hiddbgSrv, 221, in);
}

Result hiddbgUpdateDesignInfo(u32 colorBody, u32 colorButtons, u32 colorLeftGrip, u32 colorRightGrip, u8 inval, HidsysUniquePadId unique_pad_id) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 colorBody;
        u32 colorButtons;
        u32 colorLeftGrip;
        u32 colorRightGrip;
        u8 inval;
        u8 pad[7];
        HidsysUniquePadId unique_pad_id;
    } in = { colorBody, colorButtons, colorLeftGrip, colorRightGrip, inval, {0}, unique_pad_id };

    return serviceDispatchIn(&g_hiddbgSrv, 224, in);
}

Result hiddbgAcquireOperationEventHandle(Event* out_event, bool autoclear, HidsysUniquePadId unique_pad_id) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = serviceDispatchIn(&g_hiddbgSrv, 228, unique_pad_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _hiddbgReadSerialFlash(TransferMemory *tmem, u32 offset, u64 size, HidsysUniquePadId unique_pad_id) {
    const struct {
        u32 offset;
        u32 pad;
        u64 size;
        HidsysUniquePadId unique_pad_id;
    } in = { offset, 0, size, unique_pad_id };

    return serviceDispatchIn(&g_hiddbgSrv, 229, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
}

static Result _hiddbgWriteSerialFlash(TransferMemory *tmem, u32 offset, u64 tmem_size, u64 size, HidsysUniquePadId unique_pad_id) {
    const struct {
        u32 offset;
        u32 pad;
        u64 tmem_size;
        u64 size;
        HidsysUniquePadId unique_pad_id;
    } in = { offset, 0, tmem_size, size, unique_pad_id };

    return serviceDispatchIn(&g_hiddbgSrv, 230, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
}

// sdk-nso doesn't use hiddbgAcquireOperationEventHandle/hiddbgGetOperationResult in the *SerialFlash impl, those are used seperately by the user. [9.0.0+] sdk-nso no longer exposes *SerialFlash and related functionality.
Result hiddbgReadSerialFlash(u32 offset, void* buffer, size_t size, HidsysUniquePadId unique_pad_id) {
    Result rc=0;
    Event tmpevent={0};
    TransferMemory tmem;
    size_t sizealign = (size+0x1000) & ~0xfff;

    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = tmemCreate(&tmem, sizealign, Perm_Rw);
    if (R_FAILED(rc)) return rc;

    rc = hiddbgAcquireOperationEventHandle(&tmpevent, true, unique_pad_id); // *Must* be used before _hiddbgReadSerialFlash.
    if (R_SUCCEEDED(rc)) rc = _hiddbgReadSerialFlash(&tmem, offset, size, unique_pad_id);
    if (R_SUCCEEDED(rc)) rc = eventWait(&tmpevent, UINT64_MAX);
    if (R_SUCCEEDED(rc)) rc = hiddbgGetOperationResult(unique_pad_id);
    if (R_SUCCEEDED(rc)) memcpy(buffer, tmem.src_addr, size);
    eventClose(&tmpevent);
    tmemClose(&tmem);
    return rc;
}

Result hiddbgWriteSerialFlash(u32 offset, void* buffer, size_t tmem_size, size_t size, HidsysUniquePadId unique_pad_id) {
    Result rc=0;
    Event tmpevent={0};
    TransferMemory tmem;

    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    rc = tmemCreateFromMemory(&tmem, buffer, tmem_size, Perm_R);
    if (R_FAILED(rc)) return rc;

    rc = hiddbgAcquireOperationEventHandle(&tmpevent, true, unique_pad_id); // *Must* be used before _hiddbgWriteSerialFlash.
    if (R_SUCCEEDED(rc)) rc = _hiddbgWriteSerialFlash(&tmem, offset, tmem_size, size, unique_pad_id);
    if (R_SUCCEEDED(rc)) rc = eventWait(&tmpevent, UINT64_MAX);
    if (R_SUCCEEDED(rc)) rc = hiddbgGetOperationResult(unique_pad_id);
    eventClose(&tmpevent);
    tmemClose(&tmem);
    return rc;
}

Result hiddbgGetOperationResult(HidsysUniquePadId unique_pad_id) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _hiddbgCmdInU64NoOut(unique_pad_id.id, 231);
}

Result hiddbgGetUniquePadDeviceTypeSetInternal(HidsysUniquePadId unique_pad_id, u32 *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = serviceDispatchInOut(&g_hiddbgSrv, 234, unique_pad_id, tmp);
    if (R_SUCCEEDED(rc) && out) { // Pre-9.0.0 output is an u32, with [9.0.0+] it's an u8.
        if (hosversionBefore(9,0,0))
            *out = tmp;
        else
            *out = tmp & 0xFF;
    }
    return rc;
}

Result hiddbgGetAbstractedPadHandles(HiddbgAbstractedPadHandle *handles, s32 count, s32 *total_out) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_hiddbgSrv, 301, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { handles, count*sizeof(HiddbgAbstractedPadHandle) } },
    );
}

Result hiddbgGetAbstractedPadState(HiddbgAbstractedPadHandle handle, HiddbgAbstractedPadState *state) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_hiddbgSrv, 302, handle, *state);
}

Result hiddbgGetAbstractedPadsState(HiddbgAbstractedPadHandle *handles, HiddbgAbstractedPadState *states, s32 count, s32 *total_out) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_hiddbgSrv, 303, *total_out,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
        },
        .buffers = {
            { handles, count*sizeof(HiddbgAbstractedPadHandle) },
            { states, count*sizeof(HiddbgAbstractedPadState) },
        },
    );
}

Result hiddbgSetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId, const HiddbgAbstractedPadState *state) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s8 AbstractedVirtualPadId;
        u8 pad[7];
        HiddbgAbstractedPadState state;
    } in = { AbstractedVirtualPadId, {0}, *state };

    return serviceDispatchIn(&g_hiddbgSrv, 321, in);
}

Result hiddbgUnsetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId) {
    if (hosversionBefore(5,0,0) || hosversionAtLeast(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hiddbgCmdInU8NoOut(AbstractedVirtualPadId, 322);
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
    //Leave color*Grip at zero since V7 doesn't have those.
}

static void _hiddbgConvertHiddbgHdlsStateToV7(HiddbgHdlsStateV7 *out, const HiddbgHdlsState *in) {
    memset(out, 0, sizeof(*out));

    out->is_powered = (in->flags & BIT(0)) != 0;
    out->flags = (in->flags & BIT(1)) != 0;
    out->battery_level = in->battery_level;
    out->buttons = in->buttons;
    memcpy(&out->analog_stick_l, &in->analog_stick_l, sizeof(in->analog_stick_l));
    memcpy(&out->analog_stick_r, &in->analog_stick_r, sizeof(in->analog_stick_r));
    out->unk_x20 = in->unk_x20;
}

static void _hiddbgConvertHiddbgHdlsStateFromV7(HiddbgHdlsState *out, const HiddbgHdlsStateV7 *in) {
    memset(out, 0, sizeof(*out));

    out->battery_level = in->battery_level;
    out->flags = (in->is_powered & 1) | ((in->flags & 1)<<1);
    out->buttons = in->buttons;
    memcpy(&out->analog_stick_l, &in->analog_stick_l, sizeof(in->analog_stick_l));
    memcpy(&out->analog_stick_r, &in->analog_stick_r, sizeof(in->analog_stick_r));
    out->unk_x20 = in->unk_x20;
}

static void _hiddbgConvertHdlsStateListToV7(HiddbgHdlsStateListV7 *out, const HiddbgHdlsStateList *in) {
    s32 count;
    memset(out, 0, sizeof(*out));
    out->total_entries = in->total_entries;
    count = out->total_entries > 0x10 ? 0x10 : out->total_entries;

    for (s32 i=0; i<count; i++) {
        out->entries[i].handle = in->entries[i].handle;
        _hiddbgConvertHdlsDeviceInfoToV7(&out->entries[i].device, &in->entries[i].device);
        _hiddbgConvertHiddbgHdlsStateToV7(&out->entries[i].state, &in->entries[i].state);
    }
}

static void _hiddbgConvertHdlsStateListFromV7(HiddbgHdlsStateList *out, const HiddbgHdlsStateListV7 *in) {
    s32 count;
    memset(out, 0, sizeof(*out));
    out->total_entries = in->total_entries;
    count = out->total_entries > 0x10 ? 0x10 : out->total_entries;

    for (s32 i=0; i<count; i++) {
        out->entries[i].handle = in->entries[i].handle;
        _hiddbgConvertHdlsDeviceInfoFromV7(&out->entries[i].device, &in->entries[i].device);
        _hiddbgConvertHiddbgHdlsStateFromV7(&out->entries[i].state, &in->entries[i].state);
    }
}

static Result _hiddbgAttachHdlsWorkBuffer(TransferMemory *tmem) {
    return _hiddbgCmdInTmemNoOut(tmem, 324);
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

Result hiddbgIsHdlsVirtualDeviceAttached(HiddbgHdlsHandle handle, bool *out) {
    Result rc = 0;

    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _hiddbgCmdNoIO(327);
    if (R_FAILED(rc)) return rc;
    if (out) {
        *out = false;
        if (hosversionBefore(9,0,0)) {
            HiddbgHdlsStateListV7 *stateList = (HiddbgHdlsStateListV7*)(g_hiddbgHdlsTmem.src_addr);
            for (s32 i=0; i<stateList->total_entries; i++) {
                if (stateList->entries[i].handle.handle == handle.handle) {
                    *out = true;
                    break;
                }
            }
        }
        else {
            HiddbgHdlsStateList *stateList = (HiddbgHdlsStateList*)(g_hiddbgHdlsTmem.src_addr);
            for (s32 i=0; i<stateList->total_entries; i++) {
                if (stateList->entries[i].handle.handle == handle.handle) {
                    *out = true;
                    break;
                }
            }
        }
    }
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
    return _hiddbgCmdInBoolNoOut(flag, 328);
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

static Result _hiddbgAttachHdlsVirtualDeviceV7(HiddbgHdlsHandle *handle, const HiddbgHdlsDeviceInfoV7 *info) {
    return serviceDispatchInOut(&g_hiddbgSrv, 330, *info, *handle);
}

static Result _hiddbgAttachHdlsVirtualDevice(HiddbgHdlsHandle *handle, const HiddbgHdlsDeviceInfo *info) {
    return serviceDispatchInOut(&g_hiddbgSrv, 330, *info, *handle);
}

Result hiddbgAttachHdlsVirtualDevice(HiddbgHdlsHandle *handle, const HiddbgHdlsDeviceInfo *info) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(9,0,0)) {
        HiddbgHdlsDeviceInfoV7 infov7;
        _hiddbgConvertHdlsDeviceInfoToV7(&infov7, info);
        return _hiddbgAttachHdlsVirtualDeviceV7(handle, &infov7);
    }
    else
        return _hiddbgAttachHdlsVirtualDevice(handle, info);
}

Result hiddbgDetachHdlsVirtualDevice(HiddbgHdlsHandle handle) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _hiddbgCmdInU64NoOut(handle.handle, 331);
}

static Result _hiddbgSetHdlsStateV7(HiddbgHdlsHandle handle, const HiddbgHdlsState *state) {
    struct {
        HiddbgHdlsStateV7 state;
        HiddbgHdlsHandle handle;
    } in = { .handle = handle };
    _hiddbgConvertHiddbgHdlsStateToV7(&in.state, state);

    return serviceDispatchIn(&g_hiddbgSrv, 332, in);
}

static Result _hiddbgSetHdlsState(HiddbgHdlsHandle handle, const HiddbgHdlsState *state) {
    const struct {
        HiddbgHdlsHandle handle;
        HiddbgHdlsState state;
    } in = { handle, *state };

    return serviceDispatchIn(&g_hiddbgSrv, 332, in);
}

Result hiddbgSetHdlsState(HiddbgHdlsHandle handle, const HiddbgHdlsState *state) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!g_hiddbgHdlsInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (hosversionBefore(9,0,0))
        return _hiddbgSetHdlsStateV7(handle, state);
    else
        return _hiddbgSetHdlsState(handle, state);
}

