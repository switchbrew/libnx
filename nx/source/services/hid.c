#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "kernel/shmem.h"
#include "kernel/mutex.h"
#include "kernel/rwlock.h"
#include "services/applet.h"
#include "services/hid.h"
#include "runtime/hosversion.h"
#include "runtime/diag.h"
#include "../runtime/alloc.h"

static Service g_hidSrv;
static Service g_hidIAppletResource;
static Service g_hidIActiveVibrationDeviceList;
static SharedMemory g_hidSharedmem;
static Mutex g_hidVibrationMutex;

static u8* g_sevenSixAxisSensorBuffer;
static TransferMemory g_sevenSixAxisSensorTmem0;
static TransferMemory g_sevenSixAxisSensorTmem1;

static Result _hidCreateAppletResource(Service* srv, Service* srv_out);
static Result _hidGetSharedMemoryHandle(Service* srv, Handle* handle_out);

static Result _hidActivateTouchScreen(void);
static Result _hidActivateMouse(void);
static Result _hidActivateKeyboard(void);
static Result _hidActivateNpad(void);
static Result _hidActivateGesture(void);

static Result _hidGetVibrationDeviceHandles(HidVibrationDeviceHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style);

static Result _hidCreateActiveVibrationDeviceList(Service* srv_out);

static Result _hidActivateVibrationDevice(Service* srv, HidVibrationDeviceHandle handle);

static u8 _hidGetSixAxisSensorHandleNpadStyleIndex(HidSixAxisSensorHandle handle);
static Result _hidGetSixAxisSensorHandles(HidSixAxisSensorHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style);

static Result _hidGetPalmaOperationResult(HidPalmaConnectionHandle handle);

NX_GENERATE_SERVICE_GUARD(hid);

Result _hidInitialize(void) {
    Result rc=0;
    Handle sharedmem_handle;

    rc = smGetService(&g_hidSrv, "hid");
    if (R_FAILED(rc))
        return rc;

    rc = _hidCreateAppletResource(&g_hidSrv, &g_hidIAppletResource);

    if (R_SUCCEEDED(rc))
        rc = _hidGetSharedMemoryHandle(&g_hidIAppletResource, &sharedmem_handle);

    if (R_SUCCEEDED(rc)) {
        shmemLoadRemote(&g_hidSharedmem, sharedmem_handle, 0x40000, Perm_R);
        rc = shmemMap(&g_hidSharedmem);
    }

    return rc;
}

void _hidCleanup(void) {
    if (g_sevenSixAxisSensorBuffer != NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    serviceClose(&g_hidIActiveVibrationDeviceList);
    shmemClose(&g_hidSharedmem);
    serviceClose(&g_hidIAppletResource);
    serviceClose(&g_hidSrv);
}

Service* hidGetServiceSession(void) {
    return &g_hidSrv;
}

void* hidGetSharedmemAddr(void) {
    return shmemGetAddr(&g_hidSharedmem);
}

static HidNpadInternalState* _hidGetNpadInternalState(HidNpadIdType id) {
    HidSharedMemory *sharedmem = (HidSharedMemory*)hidGetSharedmemAddr();
    if (sharedmem == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    if (id <= HidNpadIdType_No8)
        return &sharedmem->npad.entries[id].internal_state;
    else if (id == HidNpadIdType_Handheld)
        return &sharedmem->npad.entries[8].internal_state;
    else if (id == HidNpadIdType_Other)
        return &sharedmem->npad.entries[9].internal_state;
    else
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadInput));
}

u32 hidGetNpadStyleSet(HidNpadIdType id) {
    return atomic_load_explicit(&_hidGetNpadInternalState(id)->style_set, memory_order_acquire);
}

HidNpadJoyAssignmentMode hidGetNpadJoyAssignment(HidNpadIdType id) {
    HidNpadInternalState *npad = _hidGetNpadInternalState(id);

    HidNpadJoyAssignmentMode tmp = atomic_load_explicit(&npad->joy_assignment_mode, memory_order_acquire);
    if (tmp != HidNpadJoyAssignmentMode_Dual && tmp != HidNpadJoyAssignmentMode_Single)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    return tmp;
}

Result hidGetNpadControllerColorSingle(HidNpadIdType id, HidNpadControllerColor *color) {
    Result rc = 0;
    HidNpadInternalState *npad = _hidGetNpadInternalState(id);

    HidColorAttribute attribute = npad->full_key_color.attribute;
    if (attribute==HidColorAttribute_NoController) rc = MAKERESULT(202, 604);
    else if (attribute==HidColorAttribute_ReadError) rc = MAKERESULT(202, 603);
    else if (attribute!=HidColorAttribute_Ok) diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    if (R_SUCCEEDED(rc))
        *color = npad->full_key_color.full_key;

    return rc;
}

Result hidGetNpadControllerColorSplit(HidNpadIdType id, HidNpadControllerColor *color_left, HidNpadControllerColor *color_right) {
    Result rc = 0;
    HidNpadInternalState *npad = _hidGetNpadInternalState(id);

    HidColorAttribute attribute = npad->joy_color.attribute;
    if (attribute==HidColorAttribute_NoController) rc = MAKERESULT(202, 604);
    else if (attribute==HidColorAttribute_ReadError) rc = MAKERESULT(202, 603);
    else if (attribute!=HidColorAttribute_Ok) diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

    if (R_SUCCEEDED(rc)) {
        *color_left  = npad->joy_color.left;
        *color_right = npad->joy_color.right;
    }

    return rc;
}

u32 hidGetNpadDeviceType(HidNpadIdType id) {
    return atomic_load_explicit(&_hidGetNpadInternalState(id)->device_type, memory_order_acquire);
}

void hidGetNpadSystemProperties(HidNpadIdType id, HidNpadSystemProperties *out) {
    *out = atomic_load_explicit(&_hidGetNpadInternalState(id)->system_properties, memory_order_acquire);
}

void hidGetNpadSystemButtonProperties(HidNpadIdType id, HidNpadSystemButtonProperties *out) {
    *out = atomic_load_explicit(&_hidGetNpadInternalState(id)->system_button_properties, memory_order_acquire);
}

static void _hidGetNpadPowerInfo(HidNpadInternalState *npad, HidPowerInfo *info, u64 is_charging, u64 is_powered, u32 i) {
    *info = (HidPowerInfo){
        .battery_level = atomic_load_explicit(&npad->battery_level[i], memory_order_acquire),
        .is_charging = (is_charging & BIT(i)) != 0,
        .is_powered = (is_powered & BIT(i)) != 0,
    };
    if (info->battery_level > 4) info->battery_level = 4; // sdknso would Abort when this occurs.
}

void hidGetNpadPowerInfoSingle(HidNpadIdType id, HidPowerInfo *info) {
    HidNpadInternalState *npad = _hidGetNpadInternalState(id);

    HidNpadSystemProperties properties;
    properties = atomic_load_explicit(&npad->system_properties, memory_order_acquire);

    _hidGetNpadPowerInfo(npad, info, properties.is_charging, properties.is_powered, 0);
}

void hidGetNpadPowerInfoSplit(HidNpadIdType id, HidPowerInfo *info_left, HidPowerInfo *info_right) {
    HidNpadInternalState *npad = _hidGetNpadInternalState(id);

    HidNpadSystemProperties properties;
    properties = atomic_load_explicit(&npad->system_properties, memory_order_acquire);

    _hidGetNpadPowerInfo(npad, info_left,  properties.is_charging, properties.is_powered, 1);
    _hidGetNpadPowerInfo(npad, info_right, properties.is_charging, properties.is_powered, 2);
}

u32 hidGetAppletFooterUiAttributesSet(HidNpadIdType id) {
    return atomic_load_explicit(&_hidGetNpadInternalState(id)->applet_footer_ui_attribute, memory_order_acquire);
}

HidAppletFooterUiType hidGetAppletFooterUiTypes(HidNpadIdType id) {
    return atomic_load_explicit(&_hidGetNpadInternalState(id)->applet_footer_ui_type, memory_order_acquire);
}

HidNpadLagerType hidGetNpadLagerType(HidNpadIdType id) {
    HidNpadLagerType lager_type = atomic_load_explicit(&_hidGetNpadInternalState(id)->lager_type, memory_order_acquire);
    if (!(lager_type>=HidNpadLagerType_J && lager_type<=HidNpadLagerType_U)) lager_type = HidNpadLagerType_Invalid;
    return lager_type;
}

static size_t _hidGetStates(HidCommonLifoHeader *header, void* in_states, size_t max_states, size_t state_offset, size_t sampling_number_offset, void* states, size_t entrysize, size_t count) {
    s32 total_entries = (s32)atomic_load_explicit(&header->count, memory_order_acquire);
    if (total_entries < 0) total_entries = 0;
    if (total_entries > count) total_entries = count;
    s32 tail = (s32)atomic_load_explicit(&header->tail, memory_order_acquire);

    for (s32 i=0; i<total_entries; i++) {
        s32 entrypos = (((tail + (max_states+1)) - total_entries) + i) % max_states;
        void* state_entry = (void*)((uintptr_t)in_states + entrypos*(state_offset+entrysize));
        void* out_state = (void*)((uintptr_t)states + (total_entries-i-1)*entrysize);
        void* out_state_prev = (void*)((uintptr_t)states + (total_entries-i)*entrysize);

        u64 sampling_number0=0, sampling_number1=0;

        sampling_number0 = atomic_load_explicit((u64*)state_entry, memory_order_acquire);
        memcpy(out_state, (void*)((uintptr_t)state_entry + state_offset), entrysize);
        sampling_number1 = atomic_load_explicit((u64*)state_entry, memory_order_acquire);

        if (sampling_number0 != sampling_number1 || (i>0 && *((u64*)((uintptr_t)out_state+sampling_number_offset)) - *((u64*)((uintptr_t)out_state_prev+sampling_number_offset)) != 1)) {
            s32 tmpcount = (s32)atomic_load_explicit(&header->count, memory_order_acquire);
            tmpcount = total_entries < tmpcount ? tmpcount : total_entries;
            total_entries = tmpcount < count ? tmpcount : count;
            tail = (s32)atomic_load_explicit(&header->tail, memory_order_acquire);

            i=-1;
        }
    }

    return total_entries;
}

void hidInitializeTouchScreen(void) {
    Result rc = _hidActivateTouchScreen();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

size_t hidGetTouchScreenStates(HidTouchScreenState *states, size_t count) {
    HidSharedMemory *sharedmem = (HidSharedMemory*)hidGetSharedmemAddr();
    if (sharedmem == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    size_t total = _hidGetStates(&sharedmem->touchscreen.lifo.header, sharedmem->touchscreen.lifo.storage, 17, offsetof(HidTouchScreenStateAtomicStorage,state), offsetof(HidTouchScreenState,sampling_number), states, sizeof(HidTouchScreenState), count);
    size_t max_touches = sizeof(states[0].touches)/sizeof(states[0].touches[0]);
    for (size_t i=0; i<total; i++) {
        if (states[i].count > max_touches) states[i].count = max_touches;
    }
    return total;
}

void hidInitializeMouse(void) {
    Result rc = _hidActivateMouse();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

size_t hidGetMouseStates(HidMouseState *states, size_t count) {
    HidSharedMemory *sharedmem = (HidSharedMemory*)hidGetSharedmemAddr();
    if (sharedmem == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    size_t total = _hidGetStates(&sharedmem->mouse.lifo.header, sharedmem->mouse.lifo.storage, 17, offsetof(HidMouseStateAtomicStorage,state), offsetof(HidMouseState,sampling_number), states, sizeof(HidMouseState), count);
    return total;
}

void hidInitializeKeyboard(void) {
    Result rc = _hidActivateKeyboard();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

size_t hidGetKeyboardStates(HidKeyboardState *states, size_t count) {
    HidSharedMemory *sharedmem = (HidSharedMemory*)hidGetSharedmemAddr();
    if (sharedmem == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    size_t total = _hidGetStates(&sharedmem->keyboard.lifo.header, sharedmem->keyboard.lifo.storage, 17, offsetof(HidKeyboardStateAtomicStorage,state), offsetof(HidKeyboardState,sampling_number), states, sizeof(HidKeyboardState), count);
    return total;
}

void hidInitializeNpad(void) {
    Result rc = _hidActivateNpad();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

static size_t _hidGetNpadStates(HidNpadCommonLifo *lifo, HidNpadCommonState *states, size_t count) {
    if (count > 17) count = 17;
    return _hidGetStates(&lifo->header, lifo->storage, 17, offsetof(HidNpadCommonStateAtomicStorage,state), offsetof(HidNpadCommonState,sampling_number), states, sizeof(HidNpadCommonState), count);
}

size_t hidGetNpadStatesFullKey(HidNpadIdType id, HidNpadFullKeyState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->full_key_lifo, states, count);

    // sdknso would handle button-bitmasking with ControlPadRestriction here.

    return total;
}

size_t hidGetNpadStatesHandheld(HidNpadIdType id, HidNpadHandheldState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->handheld_lifo, states, count);

    // sdknso would handle button-bitmasking with ControlPadRestriction here.

    return total;
}

size_t hidGetNpadStatesJoyDual(HidNpadIdType id, HidNpadJoyDualState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->joy_dual_lifo, states, count);

    // sdknso would handle button-bitmasking with ControlPadRestriction here.

    return total;
}

size_t hidGetNpadStatesJoyLeft(HidNpadIdType id, HidNpadJoyLeftState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->joy_left_lifo, states, count);

    // sdknso would handle button-bitmasking with ControlPadRestriction here.

    return total;
}

size_t hidGetNpadStatesJoyRight(HidNpadIdType id, HidNpadJoyRightState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->joy_right_lifo, states, count);

    // sdknso would handle button-bitmasking with ControlPadRestriction here.

    return total;
}

size_t hidGetNpadStatesGc(HidNpadIdType id, HidNpadGcState *states, size_t count) {
    HidNpadCommonState tmp_entries[17];
    HidNpadGcTriggerState tmp_entries_trigger[17];

    HidNpadInternalState *npad = _hidGetNpadInternalState(id);
    size_t total = _hidGetNpadStates(&npad->full_key_lifo, tmp_entries, count);
    size_t total2 = _hidGetStates(&npad->gc_trigger_lifo.header, npad->gc_trigger_lifo.storage, 17, offsetof(HidNpadGcTriggerStateAtomicStorage,state), offsetof(HidNpadGcTriggerState,sampling_number), tmp_entries_trigger, sizeof(HidNpadGcTriggerState), total);
    if (total2 < total) total = total2;

    memset(states, 0, sizeof(HidNpadGcState) * total);

    for (size_t i=0; i<total; i++) {
        states[i].sampling_number = tmp_entries[i].sampling_number;

        // sdknso would handle button-bitmasking with ControlPadRestriction here.

        states[i].buttons = tmp_entries[i].buttons;

        memcpy(&states[i].analog_stick_l, &tmp_entries[i].analog_stick_l, sizeof(tmp_entries[i].analog_stick_l)); // sdknso uses index 0 for the src here.
        memcpy(&states[i].analog_stick_r, &tmp_entries[i].analog_stick_r, sizeof(tmp_entries[i].analog_stick_r)); // sdknso uses index 0 for the src here.
        states[i].attributes = tmp_entries[i].attributes;

        states[i].trigger_l = tmp_entries_trigger[i].trigger_l;
        states[i].trigger_r = tmp_entries_trigger[i].trigger_r;
    }

    return total;
}

size_t hidGetNpadStatesPalma(HidNpadIdType id, HidNpadPalmaState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->palma_lifo, states, count);

    // sdknso doesn't handle ControlPadRestriction with this.

    return total;
}

size_t hidGetNpadStatesLark(HidNpadIdType id, HidNpadLarkState *states, size_t count) {
    HidNpadCommonState tmp_entries[17];

    HidNpadInternalState *npad = _hidGetNpadInternalState(id);
    size_t total = _hidGetNpadStates(&npad->full_key_lifo, tmp_entries, count);

    memset(states, 0, sizeof(HidNpadLarkState) * total);

    HidNpadLarkType lark_type_l_and_main = atomic_load_explicit(&npad->lark_type_l_and_main, memory_order_acquire);
    if (!(lark_type_l_and_main>=HidNpadLarkType_H1 && lark_type_l_and_main<=HidNpadLarkType_NR)) lark_type_l_and_main = HidNpadLarkType_Invalid;

    for (size_t i=0; i<total; i++) {
        states[i].sampling_number = tmp_entries[i].sampling_number;

        // sdknso would handle button-bitmasking with ControlPadRestriction here.

        states[i].buttons = tmp_entries[i].buttons;

        // Leave analog-sticks state at zeros.

        states[i].attributes = tmp_entries[i].attributes;
        states[i].lark_type_l_and_main = lark_type_l_and_main;
    }

    return total;
}

size_t hidGetNpadStatesHandheldLark(HidNpadIdType id, HidNpadHandheldLarkState *states, size_t count) {
    HidNpadCommonState tmp_entries[17];

    HidNpadInternalState *npad = _hidGetNpadInternalState(id);
    size_t total = _hidGetNpadStates(&npad->handheld_lifo, tmp_entries, count);

    memset(states, 0, sizeof(HidNpadHandheldLarkState) * total);

    HidNpadLarkType lark_type_l_and_main = atomic_load_explicit(&npad->lark_type_l_and_main, memory_order_acquire);
    if (!(lark_type_l_and_main>=HidNpadLarkType_H1 && lark_type_l_and_main<=HidNpadLarkType_NR)) lark_type_l_and_main = HidNpadLarkType_Invalid;

    HidNpadLarkType lark_type_r = atomic_load_explicit(&npad->lark_type_r, memory_order_acquire);
    if (!(lark_type_r>=HidNpadLarkType_H1 && lark_type_r<=HidNpadLarkType_NR)) lark_type_r = HidNpadLarkType_Invalid;

    for (size_t i=0; i<total; i++) {
        states[i].sampling_number = tmp_entries[i].sampling_number;

        // sdknso would handle button-bitmasking with ControlPadRestriction here.

        states[i].buttons = tmp_entries[i].buttons;

        memcpy(&states[i].analog_stick_l, &tmp_entries[i].analog_stick_l, sizeof(tmp_entries[i].analog_stick_l)); // sdknso uses index 0 for the src here.
        memcpy(&states[i].analog_stick_r, &tmp_entries[i].analog_stick_r, sizeof(tmp_entries[i].analog_stick_r)); // sdknso uses index 0 for the src here.
        states[i].attributes = tmp_entries[i].attributes;
        states[i].lark_type_l_and_main = lark_type_l_and_main;
        states[i].lark_type_r = lark_type_r;
    }

    return total;
}

size_t hidGetNpadStatesLucia(HidNpadIdType id, HidNpadLuciaState *states, size_t count) {
    HidNpadCommonState tmp_entries[17];

    HidNpadInternalState *npad = _hidGetNpadInternalState(id);
    size_t total = _hidGetNpadStates(&npad->full_key_lifo, tmp_entries, count);

    memset(states, 0, sizeof(HidNpadLuciaState) * total);

    HidNpadLuciaType lucia_type = atomic_load_explicit(&npad->lucia_type, memory_order_acquire);
    if (!(lucia_type>=HidNpadLuciaType_J && lucia_type<=HidNpadLuciaType_U)) lucia_type = HidNpadLuciaType_Invalid;

    for (size_t i=0; i<total; i++) {
        states[i].sampling_number = tmp_entries[i].sampling_number;

        // sdknso would handle button-bitmasking with ControlPadRestriction here.

        states[i].buttons = tmp_entries[i].buttons;

        // Leave analog-sticks state at zeros.

        states[i].attributes = tmp_entries[i].attributes;
        states[i].lucia_type = lucia_type;
    }

    return total;
}

size_t hidGetNpadStatesLager(HidNpadIdType id, HidNpadLagerState *states, size_t count) {
    HidNpadCommonState tmp_entries[17];

    HidNpadInternalState *npad = _hidGetNpadInternalState(id);
    size_t total = _hidGetNpadStates(&npad->full_key_lifo, tmp_entries, count);

    memset(states, 0, sizeof(HidNpadLagerState) * total);

    for (size_t i=0; i<total; i++) {
        states[i].sampling_number = tmp_entries[i].sampling_number;

        // sdknso would handle button-bitmasking with ControlPadRestriction here.

        states[i].buttons = tmp_entries[i].buttons;

        // Leave analog-sticks state at zeros.

        states[i].attributes = tmp_entries[i].attributes;
    }

    return total;
}

size_t hidGetNpadStatesSystemExt(HidNpadIdType id, HidNpadSystemExtState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->system_ext_lifo, states, count);

    // sdknso would handle button-bitmasking with ControlPadRestriction here.

    return total;
}

size_t hidGetNpadStatesSystem(HidNpadIdType id, HidNpadSystemState *states, size_t count) {
    size_t total = _hidGetNpadStates(&_hidGetNpadInternalState(id)->system_ext_lifo, states, count);

    for (size_t i=0; i<total; i++) {
        u64 buttons = states[i].buttons;
        u64 new_buttons = 0;

        if (buttons & HidNpadButton_AnyLeft) new_buttons |= HidNpadButton_Left;
        if (buttons & HidNpadButton_AnyUp) new_buttons |= HidNpadButton_Up;
        if (buttons & HidNpadButton_AnyRight) new_buttons |= HidNpadButton_Right;
        if (buttons & HidNpadButton_AnyDown) new_buttons |= HidNpadButton_Down;
        if (buttons & (HidNpadButton_L|HidNpadButton_ZL)) new_buttons |= HidNpadButton_L; // sdknso would mask out this button on the else condition for both of these, but it was already clear anyway.
        if (buttons & (HidNpadButton_R|HidNpadButton_ZR)) new_buttons |= HidNpadButton_R;
        buttons = new_buttons | (buttons & (HidNpadButton_A|HidNpadButton_B|HidNpadButton_X|HidNpadButton_Y));

        // sdknso would handle button-bitmasking with ControlPadRestriction here.

        states[i].buttons = buttons;

        memset(&states[i].analog_stick_l, 0, sizeof(states[i].analog_stick_l));
        memset(&states[i].analog_stick_r, 0, sizeof(states[i].analog_stick_r));
    }

    return total;
}

size_t hidGetSixAxisSensorStates(HidSixAxisSensorHandle handle, HidSixAxisSensorState *states, size_t count) {
    HidNpadSixAxisSensorLifo *lifo = NULL;
    HidNpadInternalState *npad = _hidGetNpadInternalState(handle.player_number);
    switch(_hidGetSixAxisSensorHandleNpadStyleIndex(handle)) {
        default:
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));

        case 0: // NpadFullKey/NpadPalma
        case 5: // NpadGc (not actually returned by GetHandles, NpadFullKey is used instead)
            lifo = &npad->full_key_six_axis_sensor_lifo;
        break;

        case 1: // NpadHandheld/NpadHandheldLark
            lifo = &npad->handheld_six_axis_sensor_lifo;
        break;

        case 2: // NpadJoyDual
            if (handle.device_idx==0) lifo = &npad->joy_dual_left_six_axis_sensor_lifo;
            else if (handle.device_idx==1) lifo = &npad->joy_dual_right_six_axis_sensor_lifo;
            else diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen));
        break;

        case 3: // NpadJoyLeft
            lifo = &npad->joy_left_six_axis_sensor_lifo;
        break;

        case 4: // NpadJoyRight
            lifo = &npad->joy_right_six_axis_sensor_lifo;
        break;

        case 29: // System(Ext) (not actually returned by GetHandles)
        case 30:
        return 0;
    }

    size_t total = _hidGetStates(&lifo->header, lifo->storage, 17, offsetof(HidSixAxisSensorStateAtomicStorage,state), offsetof(HidSixAxisSensorState,sampling_number), states, sizeof(HidSixAxisSensorState), count);
    return total;
}

void hidInitializeGesture(void) {
    Result rc = _hidActivateGesture();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

size_t hidGetGestureStates(HidGestureState *states, size_t count) {
    HidSharedMemory *sharedmem = (HidSharedMemory*)hidGetSharedmemAddr();
    if (sharedmem == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    size_t total = _hidGetStates(&sharedmem->gesture.lifo.header, sharedmem->gesture.lifo.storage, 17, offsetof(HidGestureDummyStateAtomicStorage,state), offsetof(HidGestureState,sampling_number), states, sizeof(HidGestureState), count);
    return total;
}

static Result _hidCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _hidCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _hidCmdGetSession(Service* srv_out, u32 cmd_id) {
    return serviceDispatch(&g_hidSrv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _hidCmdInAruidNoOut(u32 cmd_id) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(&g_hidSrv, cmd_id, AppletResourceUserId,
        .in_send_pid = true,
    );
}

static Result _hidCmdInU8NoOut(u8 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hidSrv, cmd_id, inval);
}

static Result _hidCmdInBoolNoOut(bool inval, u32 cmd_id) {
    return _hidCmdInU8NoOut(inval!=0, cmd_id);
}

static Result _hidCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _hidCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _hidCmdInU16U64NoOut(Service* srv, u16 inval0, u64 inval1, u32 cmd_id) {
    const struct {
        u16 inval0;
        u8 pad[6];
        u64 inval1;
    } in = { inval0, {0}, inval1 };

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _hidCmdInU32U64NoOut(Service* srv, u32 inval0, u64 inval1, u32 cmd_id) {
    const struct {
        u32 inval0;
        u32 pad;
        u64 inval1;
    } in = { inval0, 0, inval1 };

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _hidCmdInU64U64NoOut(Service* srv, u64 inval0, u64 inval1, u32 cmd_id) {
    const struct {
        u64 inval0;
        u64 inval1;
    } in = { inval0, inval1 };

    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _hidCmdInU8AruidNoOut(u8 inval, u32 cmd_id) {
    const struct {
        u8 inval;
        u8 pad[7];
        u64 AppletResourceUserId;
    } in = { inval, {0}, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdInBoolAruidNoOut(bool flag, u32 cmd_id) {
    return _hidCmdInU8AruidNoOut(flag!=0, cmd_id);
}

static Result _hidCmdInU32AruidNoOut(u32 inval, u32 cmd_id) {
    const struct {
        u32 inval;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { inval, 0, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdInU64AruidNoOut(u64 inval, u32 cmd_id) {
    const struct {
        u64 AppletResourceUserId;
        u64 inval;
    } in = { appletGetAppletResourceUserId(), inval };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdInU32AruidU64NoOut(u32 in32, u64 in64, u32 cmd_id) {
    const struct {
        u32 in32;
        u32 pad;
        u64 AppletResourceUserId;
        u64 in64;
    } in = { in32, 0, appletGetAppletResourceUserId(), in64 };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdInU8U32AruidNoOut(u8 in8, u32 in32, u32 cmd_id) {
    const struct {
        u8 in8;
        u8 pad[3];
        u32 in32;
        u64 AppletResourceUserId;
    } in = { in8, {0}, in32, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdInBoolU32AruidNoOut(bool flag, u32 in32, u32 cmd_id) {
    return _hidCmdInU8U32AruidNoOut(flag!=0, in32, cmd_id);
}

static Result _hidCmdInU32U32AruidNoOut(u32 val0, u32 val1, u32 cmd_id) {
    const struct {
        u32 val0, val1;
        u64 AppletResourceUserId;
    } in = { val0, val1, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdInU32AruidOutU8(u32 inval, u8 *out, u32 cmd_id) {
    const struct {
        u32 inval;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { inval, 0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_hidSrv, cmd_id, in, *out,
        .in_send_pid = true,
    );
}

static Result _hidCmdInU32AruidOutBool(u32 inval, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _hidCmdInU32AruidOutU8(inval, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidCmdInU32AruidOutU64(u32 inval, u64 *out, u32 cmd_id) {
    const struct {
        u32 inval;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { inval, 0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_hidSrv, cmd_id, in, *out,
        .in_send_pid = true,
    );
}

static Result _hidCmdInAruidOutU32(u32 *out, u32 cmd_id) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchInOut(&g_hidSrv, cmd_id, AppletResourceUserId, *out,
        .in_send_pid = true,
    );
}

static Result _hidCmdInAruidOutU64(u64 *out, u32 cmd_id) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchInOut(&g_hidSrv, cmd_id, AppletResourceUserId, *out,
        .in_send_pid = true,
    );
}

static Result _hidCmdNoInOutU8(u8 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_hidSrv, cmd_id, *out);
}

static Result _hidCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _hidCmdNoInOutU8(&tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidCmdNoInOutU64(u64 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_hidSrv, cmd_id, *out);
}

static Result _hidCreateAppletResource(Service* srv, Service* srv_out) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(srv, 0, AppletResourceUserId,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _hidGetSharedMemoryHandle(Service* srv, Handle* handle_out) {
    return _hidCmdGetHandle(srv, handle_out, 0);
}

static Result _hidActivateTouchScreen(void) {
    return _hidCmdInAruidNoOut(11);
}

static Result _hidActivateMouse(void) {
    return _hidCmdInAruidNoOut(21);
}

static Result _hidActivateKeyboard(void) {
    return _hidCmdInAruidNoOut(31);
}

Result hidSendKeyboardLockKeyEvent(u32 events) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU32AruidNoOut(events, 32);
}

Result hidGetSixAxisSensorHandles(HidSixAxisSensorHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style) {
    return _hidGetSixAxisSensorHandles(handles, total_handles, id, style);
}

Result hidStartSixAxisSensor(HidSixAxisSensorHandle handle) {
    return _hidCmdInU32AruidNoOut(handle.type_value, 66);
}

Result hidStopSixAxisSensor(HidSixAxisSensorHandle handle) {
    return _hidCmdInU32AruidNoOut(handle.type_value, 67);
}

Result hidIsSixAxisSensorFusionEnabled(HidSixAxisSensorHandle handle, bool *out) {
    return _hidCmdInU32AruidOutBool(handle.type_value, out, 68);
}

Result hidEnableSixAxisSensorFusion(HidSixAxisSensorHandle handle, bool flag) {
    return _hidCmdInBoolU32AruidNoOut(flag, handle.type_value, 69);
}

Result hidSetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle, float unk0, float unk1) {
    if (unk0 < 0.0f || unk0 > 1.0f)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    const struct {
        HidSixAxisSensorHandle handle;
        float unk0;
        float unk1;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, unk0, unk1, 0, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, 70, in,
        .in_send_pid = true,
    );
}

Result hidGetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle, float *unk0, float *unk1) {
    Result rc;

    const struct {
        HidSixAxisSensorHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, 0, appletGetAppletResourceUserId() };

    struct {
        float unk0;
        float unk1;
    } out;

    rc = serviceDispatchInOut(&g_hidSrv, 71, in, out,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && unk0) *unk0 = out.unk0;
    if (R_SUCCEEDED(rc) && unk1) *unk1 = out.unk1;
    return rc;
}

Result hidResetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle) {
    return _hidCmdInU32AruidNoOut(handle.type_value, 72);
}

Result hidSetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle, HidGyroscopeZeroDriftMode mode) {
    const struct {
        HidSixAxisSensorHandle handle;
        u32 mode;
        u64 AppletResourceUserId;
    } in = { handle, mode, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, 79, in,
        .in_send_pid = true,
    );
}

Result hidGetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle, HidGyroscopeZeroDriftMode *mode) {
    Result rc;

    const struct {
        HidSixAxisSensorHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, 0, appletGetAppletResourceUserId() };

    u32 tmp=0;
    rc = serviceDispatchInOut(&g_hidSrv, 80, in, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && mode) *mode = tmp;
    return rc;
}

Result hidResetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle) {
    return _hidCmdInU32AruidNoOut(handle.type_value, 81);
}

Result hidIsSixAxisSensorAtRest(HidSixAxisSensorHandle handle, bool *out) {
    return _hidCmdInU32AruidOutBool(handle.type_value, out, 82);
}

Result hidIsFirmwareUpdateAvailableForSixAxisSensor(HidSixAxisSensorHandle handle, bool *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU32AruidOutBool(handle.type_value, out, 83);
}

static Result _hidActivateGesture(void) {
    u32 val=1;

    return _hidCmdInU32AruidNoOut(val, 91); // ActivateGesture
}

Result hidSetSupportedNpadStyleSet(u32 style_set) {
    return _hidCmdInU32AruidNoOut(style_set, 100);
}

Result hidGetSupportedNpadStyleSet(u32 *style_set) {
    u32 tmp=0;
    Result rc = _hidCmdInAruidOutU32(&tmp, 101);
    if (R_SUCCEEDED(rc) && style_set) *style_set = tmp;
    return rc;
}

Result hidSetSupportedNpadIdType(const HidNpadIdType *ids, size_t count) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(&g_hidSrv, 102, AppletResourceUserId,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { ids, count*sizeof(HidNpadIdType) } },
        .in_send_pid = true,
    );
}

static Result _hidActivateNpad(void) {
    u32 revision=0x0;

    if (hosversionBefore(5,0,0))
        return _hidCmdInAruidNoOut(103); // ActivateNpad

    revision = 0x1; // [5.0.0+]
    if (hosversionAtLeast(6,0,0))
        revision = 0x2; // [6.0.0+]
    if (hosversionAtLeast(8,0,0))
        revision = 0x3; // [8.0.0+]

    return _hidCmdInU32AruidNoOut(revision, 109); // ActivateNpadWithRevision
}

Result hidAcquireNpadStyleSetUpdateEventHandle(HidNpadIdType id, Event* out_event, bool autoclear) {
    Result rc;
    Handle tmp_handle = INVALID_HANDLE;

    const struct {
        u32 id;
        u32 pad;
        u64 AppletResourceUserId;
        u64 event_ptr; // Official sw sets this to a ptr, which the sysmodule doesn't seem to use.
    } in = { id, 0, appletGetAppletResourceUserId(), 0 };

    rc = serviceDispatchIn(&g_hidSrv, 106, in,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

Result hidDisconnectNpad(HidNpadIdType id) {
    return _hidCmdInU32AruidNoOut(id, 107);
}

Result hidGetPlayerLedPattern(HidNpadIdType id, u8 *out) {
    u32 in=id;
    u64 tmp=0;
    Result rc = serviceDispatchIn(&g_hidSrv, 108, in, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result hidSetNpadJoyHoldType(HidNpadJoyHoldType type) {
    return _hidCmdInU64AruidNoOut(type, 120);
}

Result hidGetNpadJoyHoldType(HidNpadJoyHoldType *type) {
    u64 tmp=0;
    Result rc = _hidCmdInAruidOutU64(&tmp, 121);
    if (R_SUCCEEDED(rc) && type) *type = tmp;
    return rc;
}

Result hidSetNpadJoyAssignmentModeSingleByDefault(HidNpadIdType id) {
    return _hidCmdInU32AruidNoOut(id, 122);
}

Result hidSetNpadJoyAssignmentModeSingle(HidNpadIdType id, HidNpadJoyDeviceType type) {
    return _hidCmdInU32AruidU64NoOut(id, type, 123);
}

Result hidSetNpadJoyAssignmentModeDual(HidNpadIdType id) {
    return _hidCmdInU32AruidNoOut(id, 124);
}

Result hidMergeSingleJoyAsDualJoy(HidNpadIdType id0, HidNpadIdType id1) {
    return _hidCmdInU32U32AruidNoOut(id0, id1, 125);
}

Result hidStartLrAssignmentMode(void) {
    return _hidCmdInAruidNoOut(126);
}

Result hidStopLrAssignmentMode(void) {
    return _hidCmdInAruidNoOut(127);
}

Result hidSetNpadHandheldActivationMode(HidNpadHandheldActivationMode mode) {
    return _hidCmdInU64AruidNoOut(mode, 128);
}

Result hidGetNpadHandheldActivationMode(HidNpadHandheldActivationMode *out) {
    u64 tmp=0;
    Result rc = _hidCmdInAruidOutU64(&tmp, 129);
    if (R_SUCCEEDED(rc) && out) *out = tmp; // Official sw would abort if tmp isn't 0-2.
    return rc;
}

Result hidSwapNpadAssignment(HidNpadIdType id0, HidNpadIdType id1) {
    return _hidCmdInU32U32AruidNoOut(id0, id1, 130);
}

Result hidEnableUnintendedHomeButtonInputProtection(HidNpadIdType id, bool flag) {
    return _hidCmdInBoolU32AruidNoOut(flag, id, 132);
}

Result hidSetNpadJoyAssignmentModeSingleWithDestination(HidNpadIdType id, HidNpadJoyDeviceType type, bool *flag, HidNpadIdType *dest) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 id;
        u32 pad;
        u64 AppletResourceUserId;
        s64 type;
    } in = { id, 0, appletGetAppletResourceUserId(), type };

    struct {
        u8 flag;
        u8 pad[3];
        u32 id;
    } out;

    Result rc = serviceDispatchInOut(&g_hidSrv, 133, in, out,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc)) {
        if (flag) *flag = out.flag & 1;
        if (dest) *dest = out.id;
    }
    return rc;
}

Result hidSetNpadAnalogStickUseCenterClamp(bool flag) {
    if (hosversionBefore(6,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInBoolAruidNoOut(flag, 134);
}

Result hidSetNpadCaptureButtonAssignment(HidNpadStyleTag style, u64 buttons) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU32AruidU64NoOut(style, buttons, 135);
}

Result hidClearNpadCaptureButtonAssignment(void) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInAruidNoOut(136);
}

Result hidInitializeVibrationDevices(HidVibrationDeviceHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style) {
    Result rc=0;
    s32 i;

    rc = _hidGetVibrationDeviceHandles(handles, total_handles, id, style);
    if (R_FAILED(rc)) return rc;

    mutexLock(&g_hidVibrationMutex);
    if (!serviceIsActive(&g_hidIActiveVibrationDeviceList)) {
        rc = _hidCreateActiveVibrationDeviceList(&g_hidIActiveVibrationDeviceList);
    }
    mutexUnlock(&g_hidVibrationMutex);
    if (R_FAILED(rc)) return rc;

    for (i=0; i<total_handles; i++) {
        rc = _hidActivateVibrationDevice(&g_hidIActiveVibrationDeviceList, handles[i]);

        if (R_FAILED(rc))
            break;
    }

    return rc;
}

Result hidGetVibrationDeviceInfo(HidVibrationDeviceHandle handle, HidVibrationDeviceInfo *out) {
    return serviceDispatchInOut(&g_hidSrv, 200, handle, *out);
}

Result hidSendVibrationValue(HidVibrationDeviceHandle handle, const HidVibrationValue *value) {
    const struct {
        HidVibrationDeviceHandle handle;
        HidVibrationValue value;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, *value, 0, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, 201, in,
        .in_send_pid = true,
    );
}

Result hidGetActualVibrationValue(HidVibrationDeviceHandle handle, HidVibrationValue *out) {
    const struct {
        HidVibrationDeviceHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, 0, appletGetAppletResourceUserId() };

    return serviceDispatchInOut(&g_hidSrv, 202, in, *out,
        .in_send_pid = true,
    );
}

static Result _hidCreateActiveVibrationDeviceList(Service* srv_out) {
    return _hidCmdGetSession(srv_out, 203);
}

static Result _hidActivateVibrationDevice(Service* srv, HidVibrationDeviceHandle handle) {
    return _hidCmdInU32NoOut(srv, handle.type_value, 0);
}

Result hidPermitVibration(bool flag) {
    return _hidCmdInBoolNoOut(flag, 204);
}

Result hidIsVibrationPermitted(bool *flag) {
    return _hidCmdNoInOutBool(flag, 205);
}

Result hidSendVibrationValues(const HidVibrationDeviceHandle *handles, const HidVibrationValue *values, s32 count) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(&g_hidSrv, 206, AppletResourceUserId,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { handles, count*sizeof(HidVibrationDeviceHandle) },
            { values, count*sizeof(HidVibrationValue) },
        },
    );
}

Result hidSendVibrationGcErmCommand(HidVibrationDeviceHandle handle, HidVibrationGcErmCommand cmd) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidVibrationDeviceHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
        u64 cmd;
    } in = { handle, 0, appletGetAppletResourceUserId(), cmd };

    return serviceDispatchIn(&g_hidSrv, 207, in,
        .in_send_pid = true,
    );
}

Result hidGetActualVibrationGcErmCommand(HidVibrationDeviceHandle handle, HidVibrationGcErmCommand *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidVibrationDeviceHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, 0, appletGetAppletResourceUserId() };

    u64 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidSrv, 208, in, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result hidBeginPermitVibrationSession(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, appletGetAppletResourceUserId(), 209);
}

Result hidEndPermitVibrationSession(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdNoIO(&g_hidSrv, 210);
}

Result hidIsVibrationDeviceMounted(HidVibrationDeviceHandle handle, bool *flag) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;

    const struct {
        HidVibrationDeviceHandle handle;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { handle, 0, appletGetAppletResourceUserId() };

    u8 tmp=0;
    rc = serviceDispatchInOut(&g_hidSrv, 211, in, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
    return rc;
}

static HidVibrationDeviceHandle _hidMakeVibrationDeviceHandle(u8 npad_style_index, u8 player_number, u8 device_idx) {
    return (HidVibrationDeviceHandle){.npad_style_index = npad_style_index, .player_number = player_number, .device_idx = device_idx, .pad = 0};
}

static HidSixAxisSensorHandle _hidMakeSixAxisSensorHandle(u8 npad_style_index, u8 player_number, u8 device_idx) {
    return (HidSixAxisSensorHandle){.npad_style_index = npad_style_index, .player_number = player_number, .device_idx = device_idx, .pad = 0};
}

static u8 _hidGetSixAxisSensorHandleNpadStyleIndex(HidSixAxisSensorHandle handle) {
    return handle.npad_style_index - 3;
}

static Result _hidGetVibrationDeviceHandles(HidVibrationDeviceHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style) {
    Result rc=0;
    s32 max_handles=1;
    u32 style_index=0;
    u8 device_idx=0;

    if (total_handles <= 0 || total_handles > 2)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (style & HidNpadStyleTag_NpadFullKey) {
        style_index = 3;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadHandheld) {
        style_index = 4;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadJoyDual) {
        style_index = 5;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadJoyLeft) {
        style_index = 6;
    }
    else if (style & HidNpadStyleTag_NpadJoyRight) {
        style_index = 7;
        device_idx = 0x1;
    }
    else if (style & HidNpadStyleTag_NpadGc) {
        style_index = 8;
    }
    else if (style & HidNpadStyleTag_NpadLagon) {
        style_index = 0xd;
    }
    else if (style & (HidNpadStyleTag_NpadLark | HidNpadStyleTag_NpadLucia)) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }
    else if (style & HidNpadStyleTag_NpadHandheldLark) {
        style_index = 4;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadSystem) {
        style_index = 0x21;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadSystemExt) {
        style_index = 0x20;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadPalma) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }
    else {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    handles[0] = _hidMakeVibrationDeviceHandle(style_index, id, device_idx);

    if (total_handles > 1) {
        if (max_handles > 1) {
            handles[1] = _hidMakeVibrationDeviceHandle(style_index, id, 0x1);
        }
        else {
            return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would just return 0 here.
        }
    }

    return rc;
}

static Result _hidGetSixAxisSensorHandles(HidSixAxisSensorHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style) {
    Result rc=0;
    s32 max_handles=1;
    u32 style_index=0;
    u8 device_idx=0;

    if (total_handles <= 0 || total_handles > 2)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (style & HidNpadStyleTag_NpadFullKey) {
        style_index = 3;
        device_idx = 0x2;
    }
    else if (style & HidNpadStyleTag_NpadHandheld) {
        style_index = 4;
        device_idx = 0x2;
    }
    else if (style & HidNpadStyleTag_NpadJoyDual) {
        style_index = 5;
        max_handles = 2;
    }
    else if (style & HidNpadStyleTag_NpadJoyLeft) {
        style_index = 6;
    }
    else if (style & HidNpadStyleTag_NpadJoyRight) {
        style_index = 7;
        device_idx = 0x1;
    }
    else if (style & HidNpadStyleTag_NpadGc) {
        style_index = 3;
        device_idx = 0x2;
    }
    else if (style & HidNpadStyleTag_NpadLagon) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }
    else if (style & (HidNpadStyleTag_NpadLark | HidNpadStyleTag_NpadLucia)) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }
    else if (style & HidNpadStyleTag_NpadHandheldLark) {
        style_index = 4;
        device_idx = 0x2;
    }
    else if (style & HidNpadStyleTag_NpadSystem) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }
    else if (style & HidNpadStyleTag_NpadSystemExt) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }
    else if (style & HidNpadStyleTag_NpadPalma) {
        style_index = 3;
        device_idx = 0x2;
    }
    else {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would return 0, and return no handles.
    }

    handles[0] = _hidMakeSixAxisSensorHandle(style_index, id, device_idx);

    if (total_handles > 1) {
        if (max_handles > 1) {
            handles[1] = _hidMakeSixAxisSensorHandle(style_index, id, 0x1);
        }
        else {
            return MAKERESULT(Module_Libnx, LibnxError_BadInput); // sdknso would just return 0 here.
        }
    }

    return rc;
}

static Result _hidActivateConsoleSixAxisSensor(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInAruidNoOut(300);
}

Result hidStartSevenSixAxisSensor(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInAruidNoOut(304);
}

Result hidStopSevenSixAxisSensor(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInAruidNoOut(305);
}

static Result _hidInitializeSevenSixAxisSensor(TransferMemory *tmem0, TransferMemory *tmem1) {
    const struct {
        u64 AppletResourceUserId;
        u64 size0;
        u64 size1;
    } in = { appletGetAppletResourceUserId(), tmem0->size, tmem1->size };

    return serviceDispatchIn(&g_hidSrv, 306, in,
        .in_send_pid = true,
        .in_num_handles = 2,
        .in_handles = { tmem0->handle, tmem1->handle },
    );
}

Result hidInitializeSevenSixAxisSensor(void) {
    Result rc=0;
    size_t bufsize = 0x80000;

    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (g_sevenSixAxisSensorBuffer != NULL)
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    if (hosversionBefore(10,0,0)) { // No longer used by sdknso on [10.0.0+].
        rc = _hidActivateConsoleSixAxisSensor();
        if (R_FAILED(rc)) return rc;
    }

    g_sevenSixAxisSensorBuffer = (u8*)__libnx_aligned_alloc(0x1000, bufsize);
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    memset(g_sevenSixAxisSensorBuffer, 0, bufsize);

    rc = tmemCreateFromMemory(&g_sevenSixAxisSensorTmem0, g_sevenSixAxisSensorBuffer, 0x1000, Perm_R);
    if (R_SUCCEEDED(rc)) rc = tmemCreateFromMemory(&g_sevenSixAxisSensorTmem1, &g_sevenSixAxisSensorBuffer[0x1000], bufsize-0x1000, Perm_None);

    if (R_SUCCEEDED(rc)) rc = _hidInitializeSevenSixAxisSensor(&g_sevenSixAxisSensorTmem0, &g_sevenSixAxisSensorTmem1);

    if (R_FAILED(rc)) {
        tmemClose(&g_sevenSixAxisSensorTmem0);
        tmemClose(&g_sevenSixAxisSensorTmem1);

        __libnx_free(g_sevenSixAxisSensorBuffer);
        g_sevenSixAxisSensorBuffer = NULL;
    }

    return rc;
}

Result hidFinalizeSevenSixAxisSensor(void) {
    Result rc=0;

    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    rc = _hidCmdInAruidNoOut(307);

    tmemClose(&g_sevenSixAxisSensorTmem0);
    tmemClose(&g_sevenSixAxisSensorTmem1);

    __libnx_free(g_sevenSixAxisSensorBuffer);
    g_sevenSixAxisSensorBuffer = NULL;

    return rc;
}

Result hidSetSevenSixAxisSensorFusionStrength(float strength) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        float strength;
        u64 AppletResourceUserId;
    } in = { strength, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, 308, in,
        .in_send_pid = true,
    );
}

Result hidGetSevenSixAxisSensorFusionStrength(float *strength) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchInOut(&g_hidSrv, 309, AppletResourceUserId, *strength,
        .in_send_pid = true,
    );
}

Result hidResetSevenSixAxisSensorTimestamp(void) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInAruidNoOut(310);
}

Result hidGetSevenSixAxisSensorStates(HidSevenSixAxisSensorState *states, size_t count, size_t *total_out) {
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (states == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    HidSevenSixAxisSensorStates *states_buf = (HidSevenSixAxisSensorStates*)g_sevenSixAxisSensorBuffer;

    size_t total = _hidGetStates(&states_buf->header, states_buf->storage, 0x21, offsetof(HidSevenSixAxisSensorStateEntry,state), offsetof(HidSevenSixAxisSensorState,sampling_number), states, sizeof(HidSevenSixAxisSensorState), count);
    if (total_out) *total_out = total;

    return 0;
}

Result hidIsSevenSixAxisSensorAtRest(bool *out) {
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    HidSharedMemory *shared_mem = (HidSharedMemory*)hidGetSharedmemAddr();
    *out = shared_mem->console_six_axis_sensor.is_seven_six_axis_sensor_at_rest & 1;

    return 0;
}

Result hidGetSensorFusionError(float *out) {
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    HidSharedMemory *shared_mem = (HidSharedMemory*)hidGetSharedmemAddr();
    *out = shared_mem->console_six_axis_sensor.verticalization_error;

    return 0;
}

Result hidGetGyroBias(UtilFloat3 *out) {
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    HidSharedMemory *shared_mem = (HidSharedMemory*)hidGetSharedmemAddr();
    *out = shared_mem->console_six_axis_sensor.gyro_bias;

    return 0;
}

Result hidIsUsbFullKeyControllerEnabled(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdNoInOutBool(out, 400);
}

Result hidEnableUsbFullKeyController(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInBoolNoOut(flag, 401);
}

Result hidIsUsbFullKeyControllerConnected(HidNpadIdType id, bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidSrv, 402, id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result hidGetNpadInterfaceType(HidNpadIdType id, u8 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp = id;
    return serviceDispatchInOut(&g_hidSrv, 405, tmp, *out);
}

Result hidGetNpadOfHighestBatteryLevel(const HidNpadIdType *ids, size_t count, HidNpadIdType *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    u32 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidSrv, 407, AppletResourceUserId, tmp,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { ids, count*sizeof(HidNpadIdType) } },
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result hidGetPalmaConnectionHandle(HidNpadIdType id, HidPalmaConnectionHandle *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 tmp=0;
    Result rc = _hidCmdInU32AruidOutU64(id, &tmp, 500);
    if (R_SUCCEEDED(rc) && out) out->handle = tmp;
    return rc;
}

Result hidInitializePalma(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 501);
}

Result hidAcquirePalmaOperationCompleteEvent(HidPalmaConnectionHandle handle, Event* out_event, bool autoclear) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;
    Handle tmp_handle = INVALID_HANDLE;

    rc = serviceDispatchIn(&g_hidSrv, 502, handle,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _hidGetPalmaOperationInfo(HidPalmaConnectionHandle handle, void* buffer, size_t size, u64 *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_hidSrv, 503, handle, *out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result hidGetPalmaOperationInfo(HidPalmaConnectionHandle handle, HidPalmaOperationInfo *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    Result rc2=0;
    u64 tmp=0;
    bool old_ver = hosversionBefore(5,1,0);

    rc = _hidGetPalmaOperationInfo(handle, out->data, sizeof(out->data), &tmp);
    if (R_SUCCEEDED(rc) || old_ver) {
        if (old_ver) rc2 = rc;
        else rc2 = _hidGetPalmaOperationResult(handle);
        out->res = rc2;
        if (tmp > (old_ver ? HidPalmaOperationType_SuspendFeature : HidPalmaOperationType_ResetPlayLog)) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen); // sdknso would Abort here.
        else out->type = tmp;
    }

    return rc;
}

Result hidPlayPalmaActivity(HidPalmaConnectionHandle handle, u16 val) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64U64NoOut(&g_hidSrv, handle.handle, val, 504);
}

Result hidSetPalmaFrModeType(HidPalmaConnectionHandle handle, HidPalmaFrModeType type) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64U64NoOut(&g_hidSrv, handle.handle, type, 505);
}

Result hidReadPalmaStep(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 506);
}

Result hidEnablePalmaStep(HidPalmaConnectionHandle handle, bool flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 flag;
        u8 pad[7];
        HidPalmaConnectionHandle handle;
    } in = { flag!=0, {0}, handle };

    return serviceDispatchIn(&g_hidSrv, 507, in);
}

Result hidResetPalmaStep(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 508);
}

Result hidReadPalmaApplicationSection(HidPalmaConnectionHandle handle, s32 inval0, u64 size) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (size > sizeof(HidPalmaApplicationSectionAccessBuffer))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    const struct {
        HidPalmaConnectionHandle handle;
        u64 inval0;
        u64 size;
    } in = { handle, inval0, size };

    return serviceDispatchIn(&g_hidSrv, 509, in);
}

Result hidWritePalmaApplicationSection(HidPalmaConnectionHandle handle, s32 inval0, u64 size, const HidPalmaApplicationSectionAccessBuffer *buf) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidPalmaConnectionHandle handle;
        u64 inval0;
        u64 size;
    } in = { handle, inval0, size };

    return serviceDispatchIn(&g_hidSrv, 510, in,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buf, sizeof(*buf) } },
    );
}

Result hidReadPalmaUniqueCode(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 511);
}

Result hidSetPalmaUniqueCodeInvalid(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 512);
}

Result hidWritePalmaActivityEntry(HidPalmaConnectionHandle handle, u16 unk, const HidPalmaActivityEntry *entry) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidPalmaConnectionHandle handle;
        u64 unk;
        u64 rgb_led_pattern_index;
        u64 wave_set;
        u64 wave_index;
    } in = { handle, unk, entry->rgb_led_pattern_index, entry->wave_set, entry->wave_index };

    return serviceDispatchIn(&g_hidSrv, 513, in);
}

Result hidWritePalmaRgbLedPatternEntry(HidPalmaConnectionHandle handle, u16 unk, const void* buffer, size_t size) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidPalmaConnectionHandle handle;
        u64 unk;
    } in = { handle, unk };

    return serviceDispatchIn(&g_hidSrv, 514, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buffer, size } },
    );
}

static Result _hidWritePalmaWaveEntry(HidPalmaConnectionHandle handle, HidPalmaWaveSet wave_set, u16 unk, TransferMemory *tmem, size_t size) {
    const struct {
        HidPalmaConnectionHandle handle;
        u64 wave_set;
        u64 unk;
        u64 tmem_size;
        u64 size;
    } in = { handle, wave_set, unk, tmem->size, size };

    return serviceDispatchIn(&g_hidSrv, 515, in,
        .in_num_handles = 1,
        .in_handles = { tmem->handle },
    );
}

Result hidWritePalmaWaveEntry(HidPalmaConnectionHandle handle, HidPalmaWaveSet wave_set, u16 unk, const void* buffer, size_t tmem_size, size_t size) {
    Result rc=0;
    TransferMemory tmem={0};

    rc = tmemCreateFromMemory(&tmem, (void*)buffer, tmem_size, Perm_R);
    if (R_SUCCEEDED(rc)) rc = _hidWritePalmaWaveEntry(handle, wave_set, unk, &tmem, size);
    tmemClose(&tmem);

    return rc;
}

Result hidSetPalmaDataBaseIdentificationVersion(HidPalmaConnectionHandle handle, s32 version) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU32U64NoOut(&g_hidSrv, version, handle.handle, 516);
}

Result hidGetPalmaDataBaseIdentificationVersion(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 517);
}

Result hidSuspendPalmaFeature(HidPalmaConnectionHandle handle, u32 features) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU32U64NoOut(&g_hidSrv, features, handle.handle, 518);
}

static Result _hidGetPalmaOperationResult(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 519);
}

Result hidReadPalmaPlayLog(HidPalmaConnectionHandle handle, u16 unk) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU16U64NoOut(&g_hidSrv, unk, handle.handle, 520);
}

Result hidResetPalmaPlayLog(HidPalmaConnectionHandle handle, u16 unk) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU16U64NoOut(&g_hidSrv, unk, handle.handle, 521);
}

Result hidSetIsPalmaAllConnectable(bool flag) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInBoolAruidNoOut(flag, 522);
}

Result hidSetIsPalmaPairedConnectable(bool flag) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInBoolAruidNoOut(flag, 523);
}

Result hidPairPalma(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 524);
}

static Result _hidSetPalmaBoostMode(bool flag) {
    if (hosversionBefore(5,1,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInBoolNoOut(flag, 525);
}

Result hidCancelWritePalmaWaveEntry(HidPalmaConnectionHandle handle) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdInU64NoOut(&g_hidSrv, handle.handle, 526);
}

Result hidEnablePalmaBoostMode(bool flag) {
    if (hosversionBefore(8,0,0))
        return _hidSetPalmaBoostMode(flag);

    return _hidCmdInBoolAruidNoOut(flag, 527);
}

Result hidGetPalmaBluetoothAddress(HidPalmaConnectionHandle handle, BtdrvAddress *out) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_hidSrv, 528, handle, *out);
}

Result hidSetDisallowedPalmaConnection(const BtdrvAddress *addrs, s32 count) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u64 AppletResourceUserId = appletGetAppletResourceUserId();

    return serviceDispatchIn(&g_hidSrv, 529, AppletResourceUserId,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { addrs, count*sizeof(BtdrvAddress) } },
        .in_send_pid = true,
    );
}

Result hidSetNpadCommunicationMode(HidNpadCommunicationMode mode) {
    return _hidCmdInU64AruidNoOut(mode, 1000);
}

Result hidGetNpadCommunicationMode(HidNpadCommunicationMode *out) {
    u64 tmp=0;
    Result rc = _hidCmdNoInOutU64(&tmp, 1001);
    if (R_SUCCEEDED(rc) && out) *out = tmp; // sdknso would Abort when tmp is not 0-3.
    return rc;
}

Result hidSetTouchScreenConfiguration(const HidTouchScreenConfigurationForNx *config) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidTouchScreenConfigurationForNx config;
        u64 AppletResourceUserId;
    } in = { *config, appletGetAppletResourceUserId() };

    return serviceDispatchIn(&g_hidSrv, 1002, in,
        .in_send_pid = true,
    );
}

Result hidIsFirmwareUpdateNeededForNotification(bool *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    s32 val=1;

    const struct {
        s32 val;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { val, 0, appletGetAppletResourceUserId() };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidSrv, 1003, in, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

