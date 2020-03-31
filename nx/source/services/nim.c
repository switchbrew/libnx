#include <string.h>
#include "service_guard.h"
#include "services/nim.h"

#include "runtime/hosversion.h"

static Service g_nimSrv;

NX_GENERATE_SERVICE_GUARD(nim);

Result _nimInitialize(void) {
    return smGetService(&g_nimSrv, "nim");
}

void _nimCleanup(void) {
    serviceClose(&g_nimSrv);
}

Service* nimGetServiceSession(void) {
    return &g_nimSrv;
}

Result nimDestroySystemUpdateTask(const NimSystemUpdateTaskId *task_id) {
    return serviceDispatchIn(&g_nimSrv, 1, *task_id);
}

Result nimListSystemUpdateTask(s32 *out_count, NimSystemUpdateTaskId *out_task_ids, size_t max_task_ids) {
    return serviceDispatchOut(&g_nimSrv, 2, *out_count,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { out_task_ids,  max_task_ids * sizeof(*out_task_ids) },
        },
    );
}
