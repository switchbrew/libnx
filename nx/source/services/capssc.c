#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/caps.h"
#include "services/capssc.h"
#include "services/sm.h"

static Service g_capsscSrv;
static u64 g_capsscRefCnt;

Result capsscInitialize(void) {
    Result rc=0;

    atomicIncrement64(&g_capsscRefCnt);

    if (serviceIsActive(&g_capsscSrv))
        return 0;

    if (hosversionBefore(2,0,0))
        rc = MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (R_SUCCEEDED(rc)) rc = smGetService(&g_capsscSrv, "caps:sc");

    if (R_FAILED(rc)) capsscExit();

    return rc;
}

void capsscExit(void) {
    if (atomicDecrement64(&g_capsscRefCnt) == 0)
        serviceClose(&g_capsscSrv);
}

Result capsscCaptureScreenshot(void* buf, size_t size, u32 inval, u64 width, u64 height, s64 buffer_count, s64 buffer_index, u64 timeout) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 inval;
        u64 width;
        u64 height;
        s64 buffer_count;
        s64 buffer_index;
        u64 timeout;
    } *raw;

    ipcAddRecvBuffer(&c, buf, size, 1);

    raw = serviceIpcPrepareHeader(&g_capsscSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->inval = inval;
    raw->width = width;
    raw->height = height;
    raw->buffer_count = buffer_count;
    raw->buffer_index = buffer_index;
    raw->timeout = timeout;

    Result rc = serviceIpcDispatch(&g_capsscSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_capsscSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

