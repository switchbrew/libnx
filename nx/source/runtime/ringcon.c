#include <string.h>
#include <malloc.h>
#include "runtime/ringcon.h"
#include "arm/counter.h"

static Result _ringconSetup(RingCon *c);

/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Fri Mar 13 12:10:23 2020
 * by pycrc v0.9.2, https://pycrc.org
 * using the configuration:
 *  - Width         = 8
 *  - Poly          = 0x8d
 *  - XorIn         = 0x00
 *  - ReflectIn     = False
 *  - XorOut        = 0x00
 *  - ReflectOut    = False
 *  - Algorithm     = bit-by-bit-fast
 */
// Generated output from pycrc was slightly adjusted.

static u8 crc_update(u8 crc, const void *data, size_t data_len) {
    const u8 *d = (const u8*)data;
    u32 i;
    bool bit;
    u8 c;

    while (data_len--) {
        c = *d++;
        for (i = 0x80; i > 0; i >>= 1) {
            bit = crc & 0x80;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x8d;
            }
        }
        crc &= 0xff;
    }
    return crc & 0xff;
}

static void _ringconSetErrorFlag(RingCon *c, RingConErrorFlag flag, bool value) {
    if (value)
        c->error_flags |= BIT(flag);
    else
        c->error_flags &= ~BIT(flag);
}

Result ringconCreate(RingCon *c, HidControllerID id) {
    Result rc=0;
    bool handleflag=0;
    HidbusBusType bus_type;
    u32 type = hidGetControllerType(id);
    u32 cmd = 0x00020101;

    memset(c, 0, sizeof(*c));

    c->workbuf_size = 0x1000;
    c->workbuf = memalign(0x1000, c->workbuf_size);
    if (c->workbuf == NULL)
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    else
        memset(c->workbuf, 0, c->workbuf_size);

    if (R_SUCCEEDED(rc)) {
        if (type & TYPE_JOYCON_LEFT)
            bus_type = HidbusBusType_JoyLeftRail;
        else if (type & (TYPE_JOYCON_RIGHT | TYPE_JOYCON_PAIR))
            bus_type = HidbusBusType_JoyRightRail;
        else
            rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    if (R_SUCCEEDED(rc)) rc = hidbusGetBusHandle(&c->handle, &handleflag, id, bus_type);
    if (R_SUCCEEDED(rc) && !handleflag) rc = MAKERESULT(Module_Libnx, LibnxError_NotFound);

    if (R_SUCCEEDED(rc)) rc = hidbusInitialize(c->handle);
    if (R_SUCCEEDED(rc)) c->bus_initialized = true;

    if (R_SUCCEEDED(rc)) rc = hidbusEnableExternalDevice(c->handle, true, 0x20);
    if (R_SUCCEEDED(rc)) rc = hidbusEnableJoyPollingReceiveMode(c->handle, &cmd, sizeof(cmd), c->workbuf, c->workbuf_size, HidbusJoyPollingMode_JoyEnableSixAxisPollingData);

    if (R_SUCCEEDED(rc)) rc = _ringconSetup(c);

    if (R_FAILED(rc)) ringconClose(c);

    return rc;
}

void ringconClose(RingCon *c) {
    if (c->bus_initialized) {
        // Official sw uses hidbusDisableJoyPollingReceiveMode here, but that's redundant since hidbusEnableExternalDevice with flag=false uses that automatically.
        hidbusEnableExternalDevice(c->handle, false, 0x20);
        hidbusFinalize(c->handle);
    }

    free(c->workbuf);
    c->workbuf = 0;
}

static Result _ringconSetup(RingCon *c) {
    Result rc=0, rc2=0;
    u64 prev_tick=0;
    u64 tick_freq = armGetSystemTickFreq(); // Used for 1-second timeout.

    while (R_FAILED(rc2 = ringconCmdx00020105(c, &c->flag))) {
        if (R_VALUE(rc2) != MAKERESULT(218, 7)) return rc;
        prev_tick = armGetSystemTick();
        while (R_FAILED(rc = ringconReadFwVersion(c, &c->fw_ver))) {
            if (armGetSystemTick() - prev_tick >= tick_freq) return rc;
        }
        if (c->fw_ver.fw_main_ver > 0x1f) continue;
        c->flag = 1;
        break;
    }
    if (R_SUCCEEDED(rc2)) {
        if (c->flag == 1)  {
            prev_tick = armGetSystemTick();
            do {
                rc2 = ringconReadFwVersion(c, &c->fw_ver);
            } while (R_FAILED(rc2) && armGetSystemTick() - prev_tick < tick_freq);

            prev_tick = armGetSystemTick();
            while (R_FAILED(rc = ringconReadUnkCal(c, &c->unk_cal))) {
                if (armGetSystemTick() - prev_tick >= tick_freq) {
                    return R_SUCCEEDED(rc2) ? rc : rc2;
                }
            }
            if (R_FAILED(rc2)) return rc2;
        }
    }

    s32 total_push_count = 0;
    RingConDataValid data_valid = RingConDataValid_Ok;
    rc = ringconReadTotalPushCount(c, &total_push_count, &data_valid);
    if (R_SUCCEEDED(rc) && data_valid == RingConDataValid_Ok) c->total_push_count = total_push_count;

    // Official sw has a success_flag, where the app just write rc=0 to state returns here, when it's not set. But it's not actually possible to reach this point with that flag being clear, so we won't impl that.

    if (c->flag != 1) {
        _ringconSetErrorFlag(c, RingConErrorFlag_BadFlag, true);
        return MAKERESULT(Module_Libnx, LibnxError_IoError);
    }

    rc = ringconReadId(c, &c->id_l, &c->id_h);
    if (R_FAILED(rc)) return rc;

    for (u32 i=0; i<2; i++) {
        rc = ringconReadManuCal(c, &c->manu_cal);
        if (R_SUCCEEDED(rc))
            break;
        else if (i==1)
            return rc;
    }

    for (u32 i=0; i<2; i++) {
        rc = ringconReadUserCal(c, &c->user_cal);
        if (R_SUCCEEDED(rc))
            break;
        else if (i==1)
            return rc;
    }

    s16 manu_os_max = c->manu_cal.os_max;
    s16 manu_hk_max = c->manu_cal.hk_max;
    s16 manu_zero_min = c->manu_cal.zero_min;
    if (manu_os_max <= manu_hk_max || manu_zero_min <= manu_hk_max || c->manu_cal.zero_max <= manu_zero_min || manu_os_max <= manu_zero_min) {
        _ringconSetErrorFlag(c, RingConErrorFlag_BadManuCal, true);
        return MAKERESULT(218, 7);
    }

    s16 user_os_max = c->user_cal.os_max;
    if (c->user_cal.data_valid == RingConDataValid_CRC || (user_os_max != RINGCON_CAL_MAGIC && user_os_max <= c->user_cal.hk_max)) {
        _ringconSetErrorFlag(c, RingConErrorFlag_BadUserCal, true);
        return MAKERESULT(Module_Libnx, LibnxError_IoError);
    }

    return 0;
}

Result ringconUpdateUserCal(RingCon *c, RingConUserCal cal) {
    Result rc=0;
    RingConUserCal tmp_cal={0};

    _ringconSetErrorFlag(c, RingConErrorFlag_BadUserCalUpdate, false);

    for (u32 i=0; i<2; i++) {
        rc = ringconWriteUserCal(c, cal);
        if (R_SUCCEEDED(rc)) {
            rc = ringconReadUserCal(c, &tmp_cal);
            if (R_SUCCEEDED(rc) && tmp_cal.data_valid == RingConDataValid_Ok && tmp_cal.os_max == cal.os_max && tmp_cal.hk_max == cal.hk_max && tmp_cal.zero == cal.zero) return 0;
        }
    }
    if (R_FAILED(rc)) return rc;

    _ringconSetErrorFlag(c, RingConErrorFlag_BadUserCalUpdate, true);
    return MAKERESULT(Module_Libnx, LibnxError_IoError);
}

Result ringconReadFwVersion(RingCon *c, RingConFwVersion *out) {
    Result rc=0;
    u32 cmd = 0x00020000;
    u64 out_size=0;

    struct {
        u8 status;
        u8 pad[0x3];
        u8 fw_sub_ver;
        u8 fw_main_ver;
        u8 pad2[0x2];
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
    if (R_SUCCEEDED(rc)) {
        out->fw_main_ver = reply.fw_main_ver;
        out->fw_sub_ver = reply.fw_sub_ver;
    }

    return rc;
}

Result ringconReadId(RingCon *c, u64 *id_l, u64 *id_h) {
    Result rc=0;
    u32 cmd = 0x00020100;
    u64 out_size=0;

    struct {
        u8 status;
        u8 pad[0x3];

        struct {
            struct {
              u32 data_x0;
              u16 data_x4;
            } PACKED id_l;

            struct {
              u32 data_x0;
              u16 data_x4;
            } PACKED id_h;
        } id;
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
    if (R_SUCCEEDED(rc)) {
        *id_l = reply.id.id_l.data_x0 | ((u64)reply.id.id_l.data_x4<<32);
        *id_h = reply.id.id_h.data_x0 | ((u64)reply.id.id_h.data_x4<<32);
    }

    return rc;
}

Result ringconGetPollingData(RingCon *c, RingConPollingData *out, s32 count, s32 *total_out) {
    Result rc=0;
    HidbusJoyPollingReceivedData recv_data[0x9];

    rc = hidbusGetJoyPollingReceivedData(c->handle, recv_data, 0x9);
    if (R_SUCCEEDED(rc)) {
        u64 tmp = recv_data[0].timestamp - c->polling_last_timestamp;
        if (tmp > 0x9) tmp = 0x9;
        if (count > tmp) count = tmp;
        c->polling_last_timestamp = recv_data[0].timestamp;

        for (s32 i=0; i<count; i++) {
            struct {
                u8 status;
                u8 pad[0x3];
                s16 data;
                u8 pad2[0x2];
            } *reply = (void*)recv_data[i].data;

            if (recv_data[i].size != sizeof(*reply) || reply->status != 0) return MAKERESULT(218, 7);

            out[i].data = reply->data;
            out[i].timestamp = recv_data[i].timestamp;
        }
    }

    if (R_SUCCEEDED(rc) && count) *total_out = count;

    return rc;
}

Result ringconCmdx00020105(RingCon *c, u32 *out) {
    Result rc=0;
    u32 cmd = 0x00020105;
    u64 out_size=0;

    struct {
        u8 status;
        u8 pad[0x3];
        u8 data;
        u8 pad2[0x3];
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
    if (R_SUCCEEDED(rc)) *out = reply.data;

    return rc;
}

Result ringconReadManuCal(RingCon *c, RingConManuCal *out) {
    Result rc=0;
    u32 cmd=0;
    u64 out_size=0;
    RingConFwVersion ver={0};

    rc = ringconReadFwVersion(c, &ver);
    if (R_FAILED(rc)) return rc;

    if (ver.fw_main_ver >= 0x20) {
        struct {
            u8 status;
            u8 pad[0x3];
            s16 os_max;
            u16 pad1;
            s16 hk_max;
            u16 pad2;
            s16 zero_min;
            u16 pad3;
            s16 zero_max;
            u16 pad4;
        } reply;

        cmd = 0x00020A04;
        rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
        if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
        if (R_SUCCEEDED(rc)) {
            out->os_max = reply.os_max;
            out->hk_max = reply.hk_max;
            out->zero_min = reply.zero_min;
            out->zero_max = reply.zero_max;
        }
    }
    else {
        struct {
            u8 status;
            u8 pad[0x3];
            s16 data;
            u8 pad2[0x2];
        } reply;

        if (R_SUCCEEDED(rc)) {
            cmd = 0x00020104;
            rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
            if (R_SUCCEEDED(rc)) out->os_max = reply.data;
        }

        if (R_SUCCEEDED(rc)) {
            cmd = 0x00020204;
            rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
            if (R_SUCCEEDED(rc)) out->hk_max = reply.data;
        }

        if (R_SUCCEEDED(rc)) {
            cmd = 0x00020404;
            rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
            if (R_SUCCEEDED(rc)) out->zero_max = reply.data;
        }

        if (R_SUCCEEDED(rc)) {
            cmd = 0x00020304;
            rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
            if (R_SUCCEEDED(rc)) out->zero_min = reply.data;
        }
    }

    return rc;
}

Result ringconReadUnkCal(RingCon *c, s16 *out) {
    Result rc=0;
    u32 cmd = 0x00020504;
    u64 out_size=0;
    RingConManuCal cal={0};

    struct {
        u8 status;
        u8 pad[0x3];
        s16 data;
        u8 pad2[0x2];
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
    if (R_FAILED(rc)) return rc;

    rc = ringconReadManuCal(c, &cal);
    if (R_SUCCEEDED(rc)) {
        s16 tmp = cal.hk_max - cal.os_max;
        if (tmp < 0) tmp++;
        *out = reply.data + (((u16)tmp)>>1);
    }

    return 0; // Official sw ignores error from the above ringconReadManuCal call, besides the above block.
}

static Result _ringconReadUserCalOld(RingCon *c, u32 cmd, s16 *out, RingConDataValid *data_valid) {
    Result rc=0;
    u64 out_size=0;

    struct {
        u8 status;
        u8 pad[0x3];
        s16 data;
        u8 crc;
        u8 pad2;
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
    if (R_SUCCEEDED(rc)) {
        if (crc_update(0, &reply.data, sizeof(reply.data)) != reply.crc) *data_valid = RingConDataValid_CRC;
        else {
            *data_valid = RingConDataValid_Ok;
            *out = reply.data;
        }
    }

    return rc;
}

Result ringconReadUserCal(RingCon *c, RingConUserCal *out) {
    Result rc=0;
    u32 cmd = 0x00021A04;
    u64 out_size=0;
    RingConFwVersion ver={0};

    out->data_valid = RingConDataValid_Ok;

    rc = ringconReadFwVersion(c, &ver);
    if (R_FAILED(rc)) return rc;

    if (ver.fw_main_ver >= 0x20) {
        struct {
            u8 status;
            u8 pad[0x3];
            s16 os_max;
            u8 os_max_crc;
            u8 pad1;
            s16 hk_max;
            u8 hk_max_crc;
            u8 pad2;
            s16 zero;
            u8 zero_crc;
            u8 pad4;
            u8 unused[0x4];
        } reply;

        rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
        if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
        if (R_SUCCEEDED(rc)) {
            if ((crc_update(0, &reply.os_max, sizeof(reply.os_max)) != reply.os_max_crc) || (crc_update(0, &reply.hk_max, sizeof(reply.hk_max)) != reply.hk_max_crc) || (crc_update(0, &reply.zero, sizeof(reply.zero)) != reply.zero_crc))
                out->data_valid = RingConDataValid_CRC;
            else {
                out->data_valid = RingConDataValid_Ok;
                out->os_max = reply.os_max;
                out->hk_max = reply.hk_max;
                out->zero = reply.zero;
            }
        }
    }
    else {
        // Official sw doesn't check the data_valid output from these.
        rc = _ringconReadUserCalOld(c, 0x00021104 , &out->os_max, &out->data_valid);
        if (R_SUCCEEDED(rc)) rc = _ringconReadUserCalOld(c, 0x00021204, &out->hk_max, &out->data_valid);
        if (R_SUCCEEDED(rc)) rc = _ringconReadUserCalOld(c, 0x00021304, &out->zero, &out->data_valid);
    }

    if (R_SUCCEEDED(rc)) {
        if (out->os_max == RINGCON_CAL_MAGIC || out->hk_max == RINGCON_CAL_MAGIC || out->zero == RINGCON_CAL_MAGIC)
            out->data_valid = RingConDataValid_Cal;
    }

    return rc;
}

static Result _ringconGet3ByteOut(RingCon *c, u32 cmd, s32 *out, RingConDataValid *data_valid) {
    Result rc=0;
    u64 out_size=0;
    u8 data[0x4]={0};

    struct {
        u8 status;
        u8 pad[0x3];
        u8 data[0x3];
        u8 crc;
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != sizeof(reply) || reply.status != 0)) rc = MAKERESULT(218, 7);
    if (R_SUCCEEDED(rc)) {
        memcpy(data, reply.data, sizeof(reply.data));
        if (crc_update(0, data, sizeof(data)) != reply.crc) *data_valid = RingConDataValid_CRC; // Official sw has this field value inverted with this func, but whatever.
        else {
            *data_valid = RingConDataValid_Ok;
            *out = data[0x0] | (data[0x1]<<8) | (data[0x2]<<16);
        }
    }

    return rc;
}

Result ringconReadRepCount(RingCon *c, s32 *out, RingConDataValid *data_valid) {
    return _ringconGet3ByteOut(c, 0x00023104, out, data_valid);
}

Result ringconReadTotalPushCount(RingCon *c, s32 *out, RingConDataValid *data_valid) {
    return _ringconGet3ByteOut(c, 0x00023204, out, data_valid);
}

Result ringconResetRepCount(RingCon *c) {
    Result rc=0;
    u64 cmd = 0x04013104;
    u64 out_size=0;

    struct {
        u8 status;
        u8 pad[0x3];
        u8 unused[0x4];
    } reply;

    rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
    if (R_SUCCEEDED(rc) && (out_size != 0x4 || reply.status != 0)) rc = MAKERESULT(218, 7); // Official sw uses value 0x4 for the out_size check, instead of the usual sizeof(reply) value.

    return rc;
}

Result ringconWriteUserCal(RingCon *c, RingConUserCal cal) {
    Result rc=0;
    u64 out_size=0;
    RingConFwVersion ver={0};

    struct {
        u32 cmd;

        struct {
            s16 data;
            u8 crc;
            u8 pad;
        } os_max;

        struct {
            s16 data;
            u8 crc;
            u8 pad;
        } hk_max;

        struct {
            s16 data;
            u8 crc;
            u8 pad;
        } zero;

        u8 unused[0x4];
    } cmd = {.cmd = 0x10011A04};

    struct {
        u8 status;
        u8 pad[0x3];
    } reply;

    cmd.os_max.data = cal.os_max;
    cmd.os_max.crc = crc_update(0, &cmd.os_max.data, sizeof(cmd.os_max.data));

    cmd.hk_max.data = cal.hk_max;
    cmd.hk_max.crc = crc_update(0, &cmd.hk_max.data, sizeof(cmd.hk_max.data));

    cmd.zero.data = cal.zero;
    cmd.zero.crc = crc_update(0, &cmd.zero.data, sizeof(cmd.zero.data));

    rc = ringconReadFwVersion(c, &ver);
    if (R_FAILED(rc)) return rc;

    // Official sw doesn't check the output_size for these.
    if (ver.fw_main_ver >= 0x20) {
        rc = hidbusSendAndReceive(c->handle, &cmd, sizeof(cmd), &reply, sizeof(reply), &out_size);
        if (R_SUCCEEDED(rc) && reply.status != 0) rc = MAKERESULT(218, 7);
    }
    else {
        struct {
            u32 cmd;
            u8 data[0x4];
        } old_cmd;

        if (R_SUCCEEDED(rc)) {
            old_cmd.cmd = 0x04011104;
            memcpy(old_cmd.data, &cmd.os_max, sizeof(cmd.os_max));
            rc = hidbusSendAndReceive(c->handle, &old_cmd, sizeof(old_cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && reply.status != 0) rc = MAKERESULT(218, 7);
        }

        if (R_SUCCEEDED(rc)) {
            old_cmd.cmd = 0x04011204;
            memcpy(old_cmd.data, &cmd.hk_max, sizeof(cmd.hk_max));
            rc = hidbusSendAndReceive(c->handle, &old_cmd, sizeof(old_cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && reply.status != 0) rc = MAKERESULT(218, 7);
        }

        if (R_SUCCEEDED(rc)) {
            old_cmd.cmd = 0x04011304;
            memcpy(old_cmd.data, &cmd.zero, sizeof(cmd.zero));
            rc = hidbusSendAndReceive(c->handle, &old_cmd, sizeof(old_cmd), &reply, sizeof(reply), &out_size);
            if (R_SUCCEEDED(rc) && reply.status != 0) rc = MAKERESULT(218, 7);
        }
    }

    return rc;
}

