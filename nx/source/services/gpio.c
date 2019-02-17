#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "services/gpio.h"
#include "services/sm.h"

static Service g_gpioSrv;
static u64 g_refCnt;

Result gpioInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_gpioSrv))
        return 0;

    rc = smGetService(&g_gpioSrv, "gpio");
    
    if (R_FAILED(rc)) gpioExit();

    return rc;
}

void gpioExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_gpioSrv);
    }
}

Result gpioOpenSession(GpioPadSession *out, GpioPadName name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 name;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_gpioSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->name = name;

    Result rc = serviceIpcDispatch(&g_gpioSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_gpioSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, &g_gpioSrv, &r, 0);
        }
    }

    return rc;
}

Result gpioPadSetDirection(GpioPadSession *p, GpioDirection dir) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 dir;
    } *raw;

    raw = serviceIpcPrepareHeader(&p->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->dir = dir;

    Result rc = serviceIpcDispatch(&p->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&p->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result gpioPadGetDirection(GpioPadSession *p, GpioDirection *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&p->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(&p->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 dir;
        } *resp;

        serviceIpcParse(&p->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out = (GpioDirection)resp->dir;
        }
    }

    return rc;
}

Result gpioPadSetValue(GpioPadSession *p, GpioValue val) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 val;
    } *raw;

    raw = serviceIpcPrepareHeader(&p->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->val = val;

    Result rc = serviceIpcDispatch(&p->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 val;
        } *resp;

        serviceIpcParse(&p->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result gpioPadGetValue(GpioPadSession *p, GpioValue *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&p->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;

    Result rc = serviceIpcDispatch(&p->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 val;
        } *resp;

        serviceIpcParse(&p->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out = (GpioValue)resp->val;
        }
    }

    return rc;
}

void gpioPadClose(GpioPadSession *p) {
    serviceClose(&p->s);
}
