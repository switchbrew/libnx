/**
 * @file wlaninf.c
 * @brief WLAN InfraManager service IPC wrapper.
 * @author natinusala
 * @copyright libnx Authors
 */

#include "types.h"
#include "result.h"
#include "services/sm.h"
#include "services/wlaninf.h"
#include "arm/atomics.h"

static Service g_wlaninfSrv;
static u64 g_refCnt;

Result wlaninfInitialize(void) {
    Result rc;

    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_wlaninfSrv))
        return 0;

    rc = smGetService(&g_wlaninfSrv, "wlan:inf");

    if (R_FAILED(rc))
        wlaninfExit();

    return rc;
}

void wlaninfExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_wlaninfSrv);
    }
}

Result wlaninfGetState(WlanInfState* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_wlaninfSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;

    Result rc = serviceIpcDispatch(&g_wlaninfSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_wlaninfSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;
}

Result wlaninfGetRSSI(s32* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_wlaninfSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&g_wlaninfSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            s32 out;
        } *resp;

        serviceIpcParse(&g_wlaninfSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;
}
