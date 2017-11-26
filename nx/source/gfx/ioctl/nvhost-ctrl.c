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

Result nvioctlNvhostCtrl_EventWait(u32 fd, u32 unk0, u32 unk1, s32 timeout, u32 event_id, u32 *out)
{
    Result rc = 0;

    struct {
        __in u32 unk0;
        __in u32 unk1;
        __in s32 timeout;
        union {
            __in  u32 event;
            __out u32 result;
        };
    } data;

    memset(&data, 0, sizeof(data));
    data.unk0 = unk0;
    data.unk1 = unk1;
    data.timeout = timeout;
    data.event = event_id;

    rc = nvIoctl(fd, _IOWR(0x00, 0x1D, data), &data);

    if (R_SUCCEEDED(rc))
        *out = data.result;

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
