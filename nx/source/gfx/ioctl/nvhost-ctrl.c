#include <switch.h>
#include <string.h>

Result nvioctlNvhostCtrl_EventSignal(u32 fd, u32 event_id) {
    struct {
        __in u32 event_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.event_id = event_id;

    return nvIoctl(fd, _IOWR(0x00, 0x1C, data), &data);
}

Result nvioctlNvhostCtrl_EventWait(u32 fd, u32 syncpt_id, u32 threshold, s32 timeout, u32 event_id, u32 *out)
{
    Result rc = 0;

    struct {
        __in u32 syncpt_id;
        __in u32 threshold;
        __in s32 timeout;
        __inout u32 value;
    } data;

    memset(&data, 0, sizeof(data));
    data.syncpt_id = syncpt_id;
    data.threshold = threshold;
    data.timeout = timeout;
    data.value = event_id;

    rc = nvIoctl(fd, _IOWR(0x00, 0x1D, data), &data);

    if (R_SUCCEEDED(rc))
        *out = data.value;

    return rc;
}

Result nvioctlNvhostCtrl_EventRegister(u32 fd, u32 event_id) {
    struct {
        __in u32 event_id;
    } data;

    memset(&data, 0, sizeof(data));
    data.event_id = event_id;

    return nvIoctl(fd, _IOWR(0x40, 0x1F, data), &data);
}
