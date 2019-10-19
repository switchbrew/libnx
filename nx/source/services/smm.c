#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/smm.h"
#include "runtime/hosversion.h"

static Service g_smManagerSrv;

NX_GENERATE_SERVICE_GUARD(smManager);

Result _smManagerInitialize(void) {
    return smGetService(&g_smManagerSrv, "sm:m");
}

void _smManagerCleanup(void) {
    serviceClose(&g_smManagerSrv);
}

Service* smManagerGetServiceSession(void) {
    return &g_smManagerSrv;
}

Result smManagerRegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size) {
    return serviceDispatchIn(&g_smManagerSrv, 0, pid,
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

Result smManagerUnregisterProcess(u64 pid) {
    return serviceDispatchIn(&g_smManagerSrv, 1, pid);
}
