#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/i2c.h"
#include "services/sm.h"

static Service g_i2cSrv;
static size_t g_i2cSrvPtrBufSize;
static u64 g_refCnt;

Result i2cInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_i2cSrv))
        return 0;

    rc = smGetService(&g_i2cSrv, "i2c");
    
    if (R_SUCCEEDED(rc)) rc = ipcQueryPointerBufferSize(g_i2cSrv.handle, &g_i2cSrvPtrBufSize);

    if (R_FAILED(rc)) i2cExit();

    return rc;
}

void i2cExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        g_i2cSrvPtrBufSize = 0;
        serviceClose(&g_i2cSrv);
    }
}

Result i2cOpenSession(I2cSession *out, I2cDevice dev) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 device;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_i2cSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->device = dev;

    Result rc = serviceIpcDispatch(&g_i2cSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_i2cSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_i2cSrv, &r, 0);
        }
    }

    return rc;
}

Result i2csessionSendAuto(I2cSession *s, void *buf, size_t size, I2cTransactionOption option) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendSmart(&c, g_i2cSrvPtrBufSize, buf, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 option;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 10;
    raw->option = option;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

void i2csessionClose(I2cSession *s) {
    serviceClose(&s->s);
}
