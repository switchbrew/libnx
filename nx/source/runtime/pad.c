#include <string.h>
#include "runtime/diag.h"
#include "runtime/pad.h"

NX_CONSTEXPR bool _isStickMoving(const HidAnalogStickState* stick) {
    return stick->x != 0 || stick->y != 0;
}

void padConfigureInput(u32 max_players, u32 style_set) {
    Result rc;
    HidNpadIdType id_set[9];

    if (max_players == 0 || max_players > 8)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadInput));

    for (u32 i = 0; i < max_players; i ++)
        id_set[i] = HidNpadIdType_No1 + i;
    if (style_set & (HidNpadStyleTag_NpadHandheld|HidNpadStyleTag_NpadHandheldLark))
        id_set[max_players++] = HidNpadIdType_Handheld;

    hidInitializeNpad();
    rc = hidSetSupportedNpadIdType(id_set, max_players);
    if (R_FAILED(rc)) diagAbortWithResult(rc);
    rc = hidSetSupportedNpadStyleSet(style_set);
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

void padInitializeWithMask(PadState* pad, u64 mask) {
    memset(pad, 0, sizeof(*pad));
    pad->id_mask = mask & 0xff;
    pad->read_handheld = (mask & (1UL << HidNpadIdType_Handheld)) != 0;
}

static void _padUpdateWithCommonState(PadState* pad, const HidNpadCommonState* state) {
    pad->attributes  |= state->attributes;
    pad->buttons_cur |= state->buttons;
    if (!_isStickMoving(&pad->sticks[0]))
        pad->sticks[0] = state->analog_stick_l;
    if (!_isStickMoving(&pad->sticks[1]))
        pad->sticks[1] = state->analog_stick_r;
}

static void _padUpdateWithGcState(PadState* pad, const HidNpadGcState* state) {
    pad->attributes  |= state->attributes;
    pad->buttons_cur |= state->buttons;
    if (!_isStickMoving(&pad->sticks[0]))
        pad->sticks[0] = state->analog_stick_l;
    if (!_isStickMoving(&pad->sticks[1]))
        pad->sticks[1] = state->analog_stick_r;
    if (pad->gc_triggers[0] < state->trigger_l)
        pad->gc_triggers[0] = state->trigger_l;
    if (pad->gc_triggers[1] < state->trigger_r)
        pad->gc_triggers[1] = state->trigger_r;
}

void padUpdate(PadState* pad) {
    pad->active_id_mask = 0;
    pad->active_handheld = false;
    pad->style_set = 0;
    pad->attributes = 0;
    pad->buttons_old = pad->buttons_cur;
    pad->buttons_cur = 0;
    pad->sticks[0] = pad->sticks[1] = (HidAnalogStickState){ 0, 0 };
    pad->gc_triggers[0] = pad->gc_triggers[1] = 0;

    if (pad->read_handheld) {
        HidNpadCommonState state;
        if (hidGetNpadStatesHandheld(HidNpadIdType_Handheld, &state, 1) && (state.attributes & HidNpadAttribute_IsConnected)) {
            pad->active_handheld = true;
            pad->style_set |= HidNpadStyleTag_NpadHandheld;
            _padUpdateWithCommonState(pad, &state);
        }
    }

    u32 id_mask = pad->id_mask;
    while (id_mask) {
        u32 i = __builtin_ffs(id_mask)-1;
        HidNpadIdType id = HidNpadIdType_No1 + i;
        id_mask &= ~BIT(i);

        u32 style_set = hidGetNpadStyleSet(id);
        if (style_set == 0)
            continue;

        if (style_set & HidNpadStyleTag_NpadGc) {
            HidNpadGcState state;
            if (hidGetNpadStatesGc(id, &state, 1) && (state.attributes & HidNpadAttribute_IsConnected)) {
                pad->active_id_mask |= BIT(i);
                pad->style_set |= style_set;
                _padUpdateWithGcState(pad, &state);
            }
        }
        else {
            HidNpadCommonState state;
            size_t count = 0;

            if (style_set & HidNpadStyleTag_NpadSystemExt)
                count = hidGetNpadStatesSystemExt(id, &state, 1);
            else if (style_set & HidNpadStyleTag_NpadFullKey)
                count = hidGetNpadStatesFullKey(id, &state, 1);
            else if (style_set & HidNpadStyleTag_NpadJoyDual)
                count = hidGetNpadStatesJoyDual(id, &state, 1);
            else if (style_set & HidNpadStyleTag_NpadJoyLeft)
                count = hidGetNpadStatesJoyLeft(id, &state, 1);
            else if (style_set & HidNpadStyleTag_NpadJoyRight)
                count = hidGetNpadStatesJoyRight(id, &state, 1);

            if (count != 0 && (state.attributes & HidNpadAttribute_IsConnected)) {
                pad->active_id_mask |= BIT(i);
                pad->style_set |= style_set;
                _padUpdateWithCommonState(pad, &state);
            }
        }
    }
}

void padRepeaterUpdate(PadRepeater* r, u64 button_mask) {
    if (button_mask != r->button_mask) {
        r->button_mask = button_mask;
        r->counter = -r->delay;
    }
    else if (button_mask != 0) {
        r->counter++;
        if (r->counter >= r->repeat)
            r->counter = 0;
    }
}
