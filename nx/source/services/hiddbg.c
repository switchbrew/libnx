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

static bool g_hiddbgHdlsInitialized;
static TransferMemory g_hiddbgHdlsTmem;

Result hiddbgInitialize(void) {
    atomicIncrement64(&g_hiddbgRefCnt);

    if (serviceIsActive(&g_hiddbgSrv))
        return 0;

    Result rc = smGetService(&g_hiddbgSrv, "hid:dbg");
    if (R_FAILED(rc))
        return rc;

    return 0;
}

void hiddbgExit(void) {
    if (atomicDecrement64(&g_hiddbgRefCnt) == 0) {
        serviceClose(&g_hiddbgSrv);
    }
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
    if (state) memcpy(state, g_hiddbgHdlsTmem.src_addr, sizeof(*state));
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

    memcpy(g_hiddbgHdlsTmem.src_addr, state, sizeof(*state));
    return _hiddbgCmdNoIO(329);
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
        HiddbgHdlsState state;
        u64 handle;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_hiddbgSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 332;
    memcpy(&raw->state, state, sizeof(*state));
    raw->handle = HdlsHandle;

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

