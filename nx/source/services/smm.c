#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/smm.h"
#include "runtime/hosversion.h"

static Service g_smManagerCmifSrv;
static TipcService g_smManagerTipcSrv;

NX_GENERATE_SERVICE_GUARD(smManagerCmif);
NX_GENERATE_SERVICE_GUARD(smManagerTipc);

NX_INLINE bool _smManagerUseTipc(void) {
    return hosversionIsAtmosphere() || hosversionAtLeast(12,0,0);
}

Result smManagerInitialize(void) {
    if (_smManagerUseTipc()) {
        return smManagerTipcInitialize();
    } else {
        return smManagerCmifInitialize();
    }
}

void smManagerExit(void) {
    if (_smManagerUseTipc()) {
        return smManagerTipcExit();
    } else {
        return smManagerCmifExit();
    }
}

Result smManagerRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size) {
    if (_smManagerUseTipc()) {
        return smManagerTipcRegisterProcess(pid, acid_sac, acid_sac_size, aci0_sac, aci0_sac_size);
    } else {
        return smManagerCmifRegisterProcess(pid, acid_sac, acid_sac_size, aci0_sac, aci0_sac_size);
    }
}

Result smManagerUnregisterProcess(u64 pid) {
    if (_smManagerUseTipc()) {
        return smManagerTipcUnregisterProcess(pid);
    } else {
        return smManagerCmifUnregisterProcess(pid);
    }
}

Result _smManagerCmifInitialize(void) {
    if (_smManagerUseTipc()) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return smGetService(&g_smManagerCmifSrv, "sm:m");
}

void _smManagerCmifCleanup(void) {
    serviceClose(&g_smManagerCmifSrv);
}

Service* smManagerCmifGetServiceSession(void) {
    return &g_smManagerCmifSrv;
}

Result smManagerCmifRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size) {
    return serviceDispatchIn(&g_smManagerCmifSrv, 0, pid,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
        },
        .buffers = {
            { acid_sac, acid_sac_size },
            { aci0_sac, aci0_sac_size },
        },
    );
}

Result smManagerCmifUnregisterProcess(u64 pid) {
    return serviceDispatchIn(&g_smManagerCmifSrv, 1, pid);
}

Result _smManagerTipcInitialize(void) {
    if (!_smManagerUseTipc()) return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Handle handle;
    Result rc = smGetServiceOriginal(&handle, smEncodeName("sm:m"));

    if (R_SUCCEEDED(rc)) {
        tipcCreate(&g_smManagerTipcSrv, handle);
    }

    return rc;
}

void _smManagerTipcCleanup(void) {
    tipcClose(&g_smManagerTipcSrv);
}

TipcService* smManagerTipcGetServiceSession(void) {
    return &g_smManagerTipcSrv;
}

Result smManagerTipcRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size) {
    return tipcDispatchIn(&g_smManagerTipcSrv, 0, pid,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
        },
        .buffers = {
            { acid_sac, acid_sac_size },
            { aci0_sac, aci0_sac_size },
        },
    );
}

Result smManagerTipcUnregisterProcess(u64 pid) {
    return tipcDispatchIn(&g_smManagerTipcSrv, 1, pid);
}