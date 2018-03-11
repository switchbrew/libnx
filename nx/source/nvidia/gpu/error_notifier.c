#include <switch.h>

Result nverrCreate(NvErrorNotifier* t, NvGpu* parent)
{
    Result rc;
    Handle handle;

    rc = nvQueryEvent(parent->gpu_channel.fd, NvEventId_Gpu_ErrorNotifier, &handle);

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(&t->event, handle);
        rc = nvioctlChannel_SetErrorNotifier(parent->gpu_channel.fd, 1);
    }

    if (R_SUCCEEDED(rc)) {
        t->parent = parent;
        t->has_init = true;
    }

    return rc;
}

void nverrClose(NvErrorNotifier* t)
{
    if (!t->has_init)
        return;

    nvioctlChannel_SetErrorNotifier(t->parent->gpu_channel.fd, 0);
    eventClose(&t->event);
}

Result nverrWait(NvErrorNotifier* t, u64 timeout) {
    return eventWait(&t->event, timeout);
}

Result nverrGetError(NvErrorNotifier* t, NvError* out) {
    return nvioctlChannel_GetErrorNotification(t->parent->gpu_channel.fd, out);
}
