// Copyright 2018 SciresM
#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ro.h"

static Service g_roSrv, g_ro1Srv, g_dmntSrv;

NX_GENERATE_SERVICE_GUARD(ldrRo);
NX_GENERATE_SERVICE_GUARD(ro1);
NX_GENERATE_SERVICE_GUARD(roDmnt);

NX_INLINE Result _rosrvInitialize(Service* srv);
NX_INLINE Result _rosrvLoadNro(Service* srv, u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
NX_INLINE Result _rosrvUnloadNro(Service* srv, u64 nro_address);
NX_INLINE Result _rosrvLoadNrr(Service* srv, u64 cmd_id, u64 nrr_address, u64 nrr_size);
NX_INLINE Result _rosrvUnloadNrr(Service* srv, u64 nrr_address);

Result _ldrRoInitialize(void) {
    Result rc = smGetService(&g_roSrv, "ldr:ro");
    if (R_SUCCEEDED(rc))
        rc = _rosrvInitialize(&g_roSrv);
    return rc;
}

void _ldrRoCleanup(void) {
    serviceClose(&g_roSrv);
}

Service* ldrRoGetServiceSession(void) {
    return &g_roSrv;
}

Result _ro1Initialize(void) {
    if (hosversionBefore(7,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    Result rc = smGetService(&g_ro1Srv, "ro:1");
    if (R_SUCCEEDED(rc))
        rc = _rosrvInitialize(&g_ro1Srv);
    return rc;
}

void _ro1Cleanup(void) {
    serviceClose(&g_ro1Srv);
}

Service* ro1GetServiceSession(void) {
    return &g_ro1Srv;
}

Result _roDmntInitialize(void) {
    if (hosversionBefore(3,0,0)) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return smGetService(&g_dmntSrv, "ro:dmnt");
}

void _roDmntCleanup(void) {
    serviceClose(&g_dmntSrv);
}

Service* roDmntGetServiceSession(void) {
    return &g_dmntSrv;
}

Result _rosrvInitialize(Service* srv) {
    return serviceDispatch(srv, 4,
        .in_num_handles = 1,
        .in_handles = { CUR_PROCESS_HANDLE },
    );
}

Result _rosrvLoadNro(Service* srv, u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    const struct {
        u64 pid_placeholder;
        u64 nro_address;
        u64 nro_size;
        u64 bss_address;
        u64 bss_size;
    } in = { 0, nro_address, nro_size, bss_address, bss_size };
    return serviceDispatchInOut(srv, 0, in, *out_address, .in_send_pid = true);
}

Result _rosrvUnloadNro(Service* srv, u64 nro_address) {
    const struct {
        u64 pid_placeholder;
        u64 nro_address;
    } in = { 0, nro_address };
    return serviceDispatchIn(srv, 1, in, .in_send_pid = true);
}

Result _rosrvLoadNrr(Service* srv, u64 cmd_id, u64 nrr_address, u64 nrr_size) {
    const struct {
        u64 pid_placeholder;
        u64 nrr_address;
        u64 nrr_size;
    } in = { 0, nrr_address, nrr_size };
    return serviceDispatchIn(srv, cmd_id, in, .in_send_pid = true);
}

Result _rosrvUnloadNrr(Service* srv, u64 nrr_address) {
    const struct {
        u64 pid_placeholder;
        u64 nrr_address;
    } in = { 0, nrr_address };
    return serviceDispatchIn(srv, 3, in, .in_send_pid = true);
}

Result ldrRoLoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    return _rosrvLoadNro(&g_roSrv, out_address, nro_address, nro_size, bss_address, bss_size);
}

Result ldrRoUnloadNro(u64 nro_address) {
    return _rosrvUnloadNro(&g_roSrv, nro_address);
}

Result ldrRoLoadNrr(u64 nrr_address, u64 nrr_size) {
    return _rosrvLoadNrr(&g_roSrv, 2, nrr_address, nrr_size);
}

Result ldrRoUnloadNrr(u64 nrr_address) {
    return _rosrvUnloadNrr(&g_roSrv, nrr_address);
}

Result ldrRoLoadNrrEx(u64 nrr_address, u64 nrr_size) {
    if (hosversionBefore(7,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _rosrvLoadNrr(&g_roSrv, 10, nrr_address, nrr_size);
}

Result ro1LoadNro(u64* out_address, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    return _rosrvLoadNro(&g_ro1Srv, out_address, nro_address, nro_size, bss_address, bss_size);
}

Result ro1UnloadNro(u64 nro_address) {
    return _rosrvUnloadNro(&g_ro1Srv, nro_address);
}

Result ro1LoadNrr(u64 nrr_address, u64 nrr_size) {
    return _rosrvLoadNrr(&g_ro1Srv, 2, nrr_address, nrr_size);
}

Result ro1UnloadNrr(u64 nrr_address) {
    return _rosrvUnloadNrr(&g_ro1Srv, nrr_address);
}

Result ro1LoadNrrEx(u64 nrr_address, u64 nrr_size) {
    return _rosrvLoadNrr(&g_ro1Srv, 10, nrr_address, nrr_size);
}

Result roDmntGetProcessModuleInfo(u64 pid, LoaderModuleInfo *out_module_infos, size_t max_out_modules, s32 *num_out) {
    return serviceDispatchInOut(&g_dmntSrv, 0, pid, *num_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { out_module_infos,  max_out_modules * sizeof(*out_module_infos) },
        },
    );
}
