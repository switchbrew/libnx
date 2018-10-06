#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"

Result nvioctlNvhostCtrl_SyncptRead(u32 fd, u32 id, u32* out)
{
    struct {
        __nv_in  u32 syncpt_id;
        __nv_out u32 value;
    } data;

    memset(&data, 0, sizeof(data));
    data.syncpt_id = id;

    Result rc;

    rc = nvIoctl(fd, _NV_IOWR(0x00, 0x14, data), &data);

    if (R_SUCCEEDED(rc)) {
        *out = data.value;
    }

    return rc;
}

Result nvioctlNvhostCtrl_SyncptIncr(u32 fd, u32 id)
{
    struct {
        __nv_in u32 syncpt_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.syncpt_id = id;

    return nvIoctl(fd, _NV_IOWR(0x00, 0x15, data), &data);
}

Result nvioctlNvhostCtrl_SyncptWait(u32 fd, u32 id, u32 threshold, u32 timeout)
{
    struct {
        __nv_in u32 syncpt_id;
        __nv_in u32 threshold;
        __nv_in u32 timeout;
    } data;

    // TODO: Which field is out?
    memset(&data, 0, sizeof(data));
    data.syncpt_id = id;
    data.threshold = threshold;
    data.timeout = timeout;

    return nvIoctl(fd, _NV_IOWR(0x00, 0x16, data), &data);
}

Result nvioctlNvhostCtrl_EventSignal(u32 fd, u32 event_id)
{
    struct {
        __nv_in u32 event_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.event_id = event_id;

    return nvIoctl(fd, _NV_IOWR(0x00, 0x1C, data), &data);
}

Result nvioctlNvhostCtrl_EventWait(u32 fd, u32 syncpt_id, u32 threshold, s32 timeout, u32 event_id, u32 *out)
{
    Result rc = 0;

    struct {
        __nv_in u32 syncpt_id;
        __nv_in u32 threshold;
        __nv_in s32 timeout;
        __nv_inout u32 value;
    } data;

    memset(&data, 0, sizeof(data));
    data.syncpt_id = syncpt_id;
    data.threshold = threshold;
    data.timeout = timeout;
    data.value = event_id;

    rc = nvIoctl(fd, _NV_IOWR(0x00, 0x1D, data), &data);
    *out = data.value;

    return rc;
}

Result nvioctlNvhostCtrl_EventWaitAsync(u32 fd, u32 syncpt_id, u32 threshold, s32 timeout, u32 event_id)
{
    struct {
        __nv_in u32 syncpt_id;
        __nv_in u32 threshold;
        __nv_in s32 timeout;
        __nv_in u32 event_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.syncpt_id = syncpt_id;
    data.threshold = threshold;
    data.timeout = timeout;
    data.event_id = event_id;

    return nvIoctl(fd, _NV_IOWR(0x00, 0x1E, data), &data);
}

Result nvioctlNvhostCtrl_EventRegister(u32 fd, u32 event_id)
{
    struct {
        __nv_in u32 event_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.event_id = event_id;

    return nvIoctl(fd, _NV_IOWR(0x00, 0x1F, data), &data);
}

Result nvioctlNvhostCtrl_EventUnregister(u32 fd, u32 event_id)
{
    struct {
        __nv_in u32 event_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.event_id = event_id;

    return nvIoctl(fd, _NV_IOWR(0x00, 0x20, data), &data);
}
