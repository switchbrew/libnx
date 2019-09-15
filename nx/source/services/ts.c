#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/sm.h"
#include "services/ts.h"

static Service g_tsSrv;
static u64 g_tsRefCnt;

Result tsInitialize(void) {
    Result rc=0;

    atomicIncrement64(&g_tsRefCnt);

    if (serviceIsActive(&g_tsSrv))
        return 0;

    rc = smGetService(&g_tsSrv, "ts");

    return rc;
}

void tsExit(void) {
    if (atomicDecrement64(&g_tsRefCnt) == 0)
        serviceClose(&g_tsSrv);
}

Service* tsGetServiceSession(void) {
    return &g_tsSrv;
}

static Result _tsCmdInU8Out32(u8 inval, u32 *out, u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 inval;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_tsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->inval = inval;

    Result rc = serviceIpcDispatch(&g_tsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out;
        } *resp;

        serviceIpcParse(&g_tsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->out;
    }

    return rc;
}

Result tsGetTemperatureRange(TsLocation location, s32 *min_temperature, s32 *max_temperature) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 location;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_tsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->location = location;

    Result rc = serviceIpcDispatch(&g_tsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            s32 min_temperature;
            s32 max_temperature;
        } *resp;

        serviceIpcParse(&g_tsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && min_temperature) *min_temperature = resp->min_temperature;
        if (R_SUCCEEDED(rc) && max_temperature) *max_temperature = resp->max_temperature;
    }

    return rc;
}

Result tsGetTemperature(TsLocation location, s32 *temperature) {
    return _tsCmdInU8Out32(location, (u32*)temperature, 1);
}

Result tsGetTemperatureMilliC(TsLocation location, s32 *temperature) {
    return _tsCmdInU8Out32(location, (u32*)temperature, 3);
}

