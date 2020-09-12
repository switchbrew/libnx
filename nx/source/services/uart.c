#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "kernel/tmem.h"
#include "services/uart.h"

static Service g_uartSrv;

NX_GENERATE_SERVICE_GUARD(uart);

Result _uartInitialize(void) {
    return smGetService(&g_uartSrv, "uart");
}

void _uartCleanup(void) {
    serviceClose(&g_uartSrv);
}

Service* uartGetServiceSession(void) {
    return &g_uartSrv;
}

static Result _uartCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _uartCmdInU32OutBool(Service* srv, u32 inval, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(srv, cmd_id, inval, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _uartCmdInTwoU32sOutBool(Service* srv, u32 inval0, u32 inval1, bool *out, u32 cmd_id) {
    const struct {
        u32 inval0;
        u32 inval1;
    } in = { inval0, inval1 };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(srv, cmd_id, in, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result uartHasPort(UartPort port, bool *out) {
    return _uartCmdInU32OutBool(&g_uartSrv, port, out, 0);
}

Result uartHasPortForDev(UartPortForDev port, bool *out) {
    return _uartCmdInU32OutBool(&g_uartSrv, port, out, 1);
}

Result uartIsSupportedBaudRate(UartPort port, u32 baud_rate, bool *out) {
    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, baud_rate, out, 2);
}

Result uartIsSupportedBaudRateForDev(UartPortForDev port, u32 baud_rate, bool *out) {
    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, baud_rate, out, 3);
}

Result uartIsSupportedFlowControlMode(UartPort port, UartFlowControlMode flow_control_mode, bool *out) {
    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, flow_control_mode, out, 4);
}

Result uartIsSupportedFlowControlModeForDev(UartPortForDev port, UartFlowControlMode flow_control_mode, bool *out) {
    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, flow_control_mode, out, 5);
}

Result uartCreatePortSession(UartPortSession *s) {
    return serviceDispatch(&g_uartSrv, 6,
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
}

Result uartIsSupportedPortEvent(UartPort port, UartPortEventType port_event_type, bool *out) {
    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, port_event_type, out, 7);
}

Result uartIsSupportedPortEventForDev(UartPortForDev port, UartPortEventType port_event_type, bool *out) {
    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, port_event_type, out, 8);
}

Result uartIsSupportedDeviceVariation(UartPort port, u32 device_variation, bool *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, device_variation, out, 9);
}

Result uartIsSupportedDeviceVariationForDev(UartPortForDev port, u32 device_variation, bool *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _uartCmdInTwoU32sOutBool(&g_uartSrv, port, device_variation, out, 10);
}

// IPortSession

void uartPortSessionClose(UartPortSession* s) {
    serviceClose(&s->s);
}

/// pre-6.0.0
static Result _uartPortSessionOpenPort_v1(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length, u32 cmd_id) {
    const struct {
        u32 port;
        u32 baud_rate;
        u32 flow_control_mode;
        u32 pad;
        u64 send_buffer_length;
        u64 receive_buffer_length;
    } in = { port, baud_rate, flow_control_mode, 0, send_buffer_length, receive_buffer_length };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&s->s, cmd_id, in, tmp,
        .in_num_handles = 2,
        .in_handles = { send_handle, receive_handle },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

/// 6.x
static Result _uartPortSessionOpenPort_v6(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length, u32 cmd_id) {
    const struct {
        u8 is_invert_tx;
        u8 is_invert_rx;
        u8 is_invert_rts;
        u8 is_invert_cts;
        u32 port;
        u32 baud_rate;
        u32 flow_control_mode;
        u64 send_buffer_length;
        u64 receive_buffer_length;
    } in = { is_invert_tx!=0, is_invert_rx!=0, is_invert_rts!=0, is_invert_cts!=0, port, baud_rate, flow_control_mode, send_buffer_length, receive_buffer_length };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&s->s, cmd_id, in, tmp,
        .in_num_handles = 2,
        .in_handles = { send_handle, receive_handle },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

/// [7.0.0+]
static Result _uartPortSessionOpenPort_v7(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length, u32 cmd_id) {
    const struct {
        u8 is_invert_tx;
        u8 is_invert_rx;
        u8 is_invert_rts;
        u8 is_invert_cts;
        u32 port;
        u32 baud_rate;
        u32 flow_control_mode;
        u32 device_variation;
        u32 pad;
        u64 send_buffer_length;
        u64 receive_buffer_length;
    } in = { is_invert_tx!=0, is_invert_rx!=0, is_invert_rts!=0, is_invert_cts!=0, port, baud_rate, flow_control_mode, device_variation, 0, send_buffer_length, receive_buffer_length };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&s->s, cmd_id, in, tmp,
        .in_num_handles = 2,
        .in_handles = { send_handle, receive_handle },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _uartPortSessionOpenPort(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length, u32 cmd_id) {
    if (hosversionBefore(6,0,0))
        return _uartPortSessionOpenPort_v1(s, out, port, baud_rate, flow_control_mode, send_handle, receive_handle, send_buffer_length, receive_buffer_length, cmd_id);
    else if (hosversionBefore(7,0,0)) // 6.x
        return _uartPortSessionOpenPort_v6(s, out, port, baud_rate, flow_control_mode, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_handle, receive_handle, send_buffer_length, receive_buffer_length, cmd_id);
    else // [7.0.0+]
        return _uartPortSessionOpenPort_v7(s, out, port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_handle, receive_handle, send_buffer_length, receive_buffer_length, cmd_id);
}

Result uartPortSessionOpenPort(UartPortSession* s, bool *out, UartPort port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, void* send_buffer, u64 send_buffer_length, void* receive_buffer, u64 receive_buffer_length) {
    Result rc=0;
    TransferMemory send_tmem={0};
    TransferMemory receive_tmem={0};

    rc = tmemCreateFromMemory(&send_tmem, send_buffer, send_buffer_length, Perm_R | Perm_W);
    if (R_SUCCEEDED(rc)) rc = tmemCreateFromMemory(&receive_tmem, receive_buffer, send_buffer_length, Perm_R | Perm_W);

    if (R_SUCCEEDED(rc)) rc = _uartPortSessionOpenPort(s, out, port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_tmem.handle, receive_tmem.handle, send_buffer_length, receive_buffer_length, 0);

    tmemClose(&send_tmem);
    tmemClose(&receive_tmem);

    return rc;
}

Result uartPortSessionOpenPortForDev(UartPortSession* s, bool *out, UartPortForDev port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, void* send_buffer, u64 send_buffer_length, void* receive_buffer, u64 receive_buffer_length) {
    Result rc=0;
    TransferMemory send_tmem={0};
    TransferMemory receive_tmem={0};

    rc = tmemCreateFromMemory(&send_tmem, send_buffer, send_buffer_length, Perm_R | Perm_W);
    if (R_SUCCEEDED(rc)) rc = tmemCreateFromMemory(&receive_tmem, receive_buffer, send_buffer_length, Perm_R | Perm_W);

    if (R_SUCCEEDED(rc)) rc = _uartPortSessionOpenPort(s, out, port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_tmem.handle, receive_tmem.handle, send_buffer_length, receive_buffer_length, 1);

    tmemClose(&send_tmem);
    tmemClose(&receive_tmem);

    return rc;
}

Result uartPortSessionGetWritableLength(UartPortSession* s, u64 *out) {
    return _uartCmdNoInOutU64(&s->s, out, 2);
}

Result uartPortSessionSend(UartPortSession* s, const void* in_data, size_t size, u64 *out_size) {
    return serviceDispatchOut(&s->s, 3, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_In },
        .buffers = { { in_data, size } },
    );
}

Result uartPortSessionGetReadableLength(UartPortSession* s, u64 *out) {
    return _uartCmdNoInOutU64(&s->s, out, 4);
}

Result uartPortSessionReceive(UartPortSession* s, void* out_data, size_t size, u64 *out_size) {
    return serviceDispatchOut(&s->s, 5, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out },
        .buffers = { { out_data, size } },
    );
}

Result uartPortSessionBindPortEvent(UartPortSession* s, UartPortEventType port_event_type, s64 threshold, bool *out, Event *out_event) {
    const struct {
        u32 port_event_type;
        u32 pad;
        s64 threshold;
    } in = { port_event_type, 0, threshold };

    u8 tmp=0;
    Handle tmp_handle=0;
    Result rc = serviceDispatchInOut(&s->s, 6, in, tmp,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    if (R_SUCCEEDED(rc) && out_event) eventLoadRemote(out_event, tmp_handle, false);
    return rc;
}

Result uartPortSessionUnbindPortEvent(UartPortSession* s, UartPortEventType port_event_type, bool *out) {
    return _uartCmdInU32OutBool(&s->s, port_event_type, out, 7);
}

