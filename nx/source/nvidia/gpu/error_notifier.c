#include <switch.h>

Result nverrCreate(NvErrorNotifier* t, NvGpu* parent) {
    Result rc;

    rc = nvioctlChannel_SetErrorNotifier(parent->gpu_channel.fd, 1);

    if (R_SUCCEEDED(rc)) {
        t->parent = parent;
        t->has_init = true;
    }

    return rc;
}

void nverrClose(NvErrorNotifier* t) {
    if (!t->has_init)
        return;

    nvioctlChannel_SetErrorNotifier(t->parent->gpu_channel.fd, 0);
}
