#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/i2c.h"
#include "runtime/hosversion.h"

static Service g_i2cSrv;

NX_GENERATE_SERVICE_GUARD(i2c);

Result _i2cInitialize(void) {
    return smGetService(&g_i2cSrv, "i2c");
}

void _i2cCleanup(void) {
    serviceClose(&g_i2cSrv);
}

Service* i2cGetServiceSession(void) {
    return &g_i2cSrv;
}

Result i2cOpenSession(I2cSession *out, I2cDevice dev) {
    const u32 in = dev;
    return serviceDispatchIn(&g_i2cSrv, 1, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result i2csessionSendAuto(I2cSession *s, const void *buf, size_t size, I2cTransactionOption option) {
    const u32 in = option;
    return serviceDispatchIn(&s->s, 10, in,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcAutoSelect },
        .buffers = { { buf,  size } },
    );
}

Result i2csessionReceiveAuto(I2cSession *s, void *buf, size_t size, I2cTransactionOption option) {
    const u32 in = option;
    return serviceDispatchIn(&s->s, 11, in,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect },
        .buffers = { { buf,  size } },
    );
}

Result i2csessionExecuteCommandList(I2cSession *s, void *dst, size_t dst_size, const void *cmd_list, size_t cmd_list_size) {
    return serviceDispatch(&s->s, 12,
        .buffer_attrs = {
            SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect,
            SfBufferAttr_In | SfBufferAttr_HipcPointer,
        },
        .buffers = {
            { dst,      dst_size },
            { cmd_list, cmd_list_size },
        },
    );
}

void i2csessionClose(I2cSession *s) {
    serviceClose(&s->s);
}
