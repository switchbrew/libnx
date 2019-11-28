#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "kernel/shmem.h"
#include "kernel/rwlock.h"
#include "services/applet.h"
#include "services/hid.h"
#include "runtime/hosversion.h"

static Service g_hidSrv;
static Service g_hidIAppletResource;
static SharedMemory g_hidSharedmem;

static HidTouchScreenEntry g_touchEntry;
static HidMouseEntry *g_mouseEntry;
static HidMouse g_mouse;
static HidKeyboardEntry g_keyboardEntry;
static HidControllerHeader g_controllerHeaders[10];
static HidControllerInputEntry g_controllerEntries[10];
static HidControllerSixAxisLayout g_sixaxisLayouts[10];
static HidControllerMisc g_controllerMisc[10];

static u64 g_mouseOld, g_mouseHeld, g_mouseDown, g_mouseUp;
static u64 g_keyboardModOld, g_keyboardModHeld, g_keyboardModDown, g_keyboardModUp;
static u32 g_keyboardOld[8], g_keyboardHeld[8], g_keyboardDown[8], g_keyboardUp[8];
static u64 g_controllerOld[10], g_controllerHeld[10], g_controllerDown[10], g_controllerUp[10];
static bool g_sixaxisEnabled[10];

static HidControllerLayoutType g_controllerLayout[10];
static u64 g_touchTimestamp, g_mouseTimestamp, g_keyboardTimestamp, g_controllerTimestamps[10];

static HidControllerID g_controllerP1AutoID;

static u8* g_sevenSixAxisSensorBuffer;
static TransferMemory g_sevenSixAxisSensorTmem0;
static TransferMemory g_sevenSixAxisSensorTmem1;

static RwLock g_hidLock;

static Result _hidCreateAppletResource(Service* srv, Service* srv_out, u64 AppletResourceUserId);
static Result _hidGetSharedMemoryHandle(Service* srv, Handle* handle_out);

static Result _hidActivateNpad(void);
static Result _hidDeactivateNpad(void);

static Result _hidSetDualModeAll(void);

NX_GENERATE_SERVICE_GUARD(hid);

Result _hidInitialize(void) {
    HidControllerID idbuf[9] = {
        CONTROLLER_PLAYER_1,
        CONTROLLER_PLAYER_2,
        CONTROLLER_PLAYER_3,
        CONTROLLER_PLAYER_4,
        CONTROLLER_PLAYER_5,
        CONTROLLER_PLAYER_6,
        CONTROLLER_PLAYER_7,
        CONTROLLER_PLAYER_8,
        CONTROLLER_HANDHELD};

    Result rc=0;
    Handle sharedmem_handle;

    // If this failed (for example because we're a sysmodule) AppletResourceUserId stays zero
    u64 AppletResourceUserId = 0;
    appletGetAppletResourceUserId(&AppletResourceUserId);

    rc = smGetService(&g_hidSrv, "hid");
    if (R_FAILED(rc))
        return rc;

    rc = _hidCreateAppletResource(&g_hidSrv, &g_hidIAppletResource, AppletResourceUserId);

    if (R_SUCCEEDED(rc))
        rc = _hidGetSharedMemoryHandle(&g_hidIAppletResource, &sharedmem_handle);

    if (R_SUCCEEDED(rc)) {
        shmemLoadRemote(&g_hidSharedmem, sharedmem_handle, 0x40000, Perm_R);

        rc = shmemMap(&g_hidSharedmem);
    }

    if (R_SUCCEEDED(rc))
        rc = _hidActivateNpad();

    if (R_SUCCEEDED(rc))
        rc = hidSetSupportedNpadStyleSet(TYPE_PROCONTROLLER | TYPE_HANDHELD | TYPE_JOYCON_PAIR | TYPE_JOYCON_LEFT | TYPE_JOYCON_RIGHT | TYPE_SYSTEM_EXT | TYPE_SYSTEM);

    if (R_SUCCEEDED(rc))
        rc = hidSetSupportedNpadIdType(idbuf, 9);

    if (R_SUCCEEDED(rc))
        rc = _hidSetDualModeAll();

    if (R_SUCCEEDED(rc))
        rc = hidSetNpadJoyHoldType(HidJoyHoldType_Default);

    hidReset();
    return rc;
}

void _hidCleanup(void) {
    hidFinalizeSevenSixAxisSensor();

    hidSetNpadJoyHoldType(HidJoyHoldType_Default);

    _hidSetDualModeAll();

    _hidDeactivateNpad();

    serviceClose(&g_hidIAppletResource);
    serviceClose(&g_hidSrv);
    shmemClose(&g_hidSharedmem);
}

void hidReset(void) {
    rwlockWriteLock(&g_hidLock);

    // Reset internal state
    memset(&g_touchEntry, 0, sizeof(HidTouchScreenEntry));
    memset(&g_mouse, 0, sizeof(HidMouse));
    memset(&g_keyboardEntry, 0, sizeof(HidKeyboardEntry));
    memset(g_controllerHeaders, 0, sizeof(g_controllerHeaders));
    memset(g_controllerEntries, 0, sizeof(g_controllerEntries));
    memset(g_controllerMisc, 0, sizeof(g_controllerMisc));
    memset(g_sixaxisLayouts, 0, sizeof(g_sixaxisLayouts));
    memset(g_sixaxisEnabled, 0, sizeof(g_sixaxisEnabled));

    g_mouseEntry = &g_mouse.entries[0];
    g_mouseOld = g_mouseHeld = g_mouseDown = g_mouseUp = 0;
    g_keyboardModOld = g_keyboardModHeld = g_keyboardModDown = g_keyboardModUp = 0;
    for (int i = 0; i < 8; i++)
        g_keyboardOld[i] = g_keyboardHeld[i] = g_keyboardDown[i] = g_keyboardUp[i] = 0;
    for (int i = 0; i < 10; i++)
        g_controllerOld[i] = g_controllerHeld[i] = g_controllerDown[i] = g_controllerUp[i] = 0;

    for (int i = 0; i < 10; i++)
        g_controllerLayout[i] = LAYOUT_DEFAULT;

    g_touchTimestamp = g_mouseTimestamp = g_keyboardTimestamp = 0;
    for (int i = 0; i < 10; i++)
        g_controllerTimestamps[i] = 0;

    g_controllerP1AutoID = CONTROLLER_HANDHELD;

    rwlockWriteUnlock(&g_hidLock);
}

Service* hidGetServiceSession(void) {
    return &g_hidSrv;
}

void* hidGetSharedmemAddr(void) {
    return shmemGetAddr(&g_hidSharedmem);
}

void hidSetControllerLayout(HidControllerID id, HidControllerLayoutType layoutType) {
    if (id < 0 || id > 9) return;

    rwlockWriteLock(&g_hidLock);
    g_controllerLayout[id] = layoutType;
    rwlockWriteUnlock(&g_hidLock);
}

HidControllerLayoutType hidGetControllerLayout(HidControllerID id) {
    if (id < 0 || id > 9) return LAYOUT_DEFAULT;

    rwlockReadLock(&g_hidLock);
    HidControllerLayoutType tmp = g_controllerLayout[id];
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

void hidScanInput(void) {
    rwlockWriteLock(&g_hidLock);

    HidSharedMemory *sharedMem = (HidSharedMemory*)hidGetSharedmemAddr();

    g_mouseOld = g_mouseHeld;
    g_keyboardModOld = g_keyboardModHeld;
    memcpy(g_keyboardOld, g_keyboardHeld, sizeof(g_keyboardOld));
    memcpy(g_controllerOld, g_controllerHeld, sizeof(g_controllerOld));

    g_mouseHeld = 0;
    g_keyboardModHeld = 0;
    memset(g_keyboardHeld, 0, sizeof(g_keyboardHeld));
    memset(g_controllerHeld, 0, sizeof(g_controllerHeld));
    memset(&g_touchEntry, 0, sizeof(HidTouchScreenEntry));
    memset(&g_mouse, 0, sizeof(HidMouse));
    memset(&g_keyboardEntry, 0, sizeof(HidKeyboardEntry));
    memset(g_controllerEntries, 0, sizeof(g_controllerEntries));
    memset(g_controllerMisc, 0, sizeof(g_controllerMisc));
    memset(g_sixaxisLayouts, 0, sizeof(g_sixaxisLayouts));

    u64 latestTouchEntry = sharedMem->touchscreen.header.latestEntry;
    HidTouchScreenEntry *newTouchEntry = &sharedMem->touchscreen.entries[latestTouchEntry];
    if ((s64)(newTouchEntry->header.timestamp - g_touchTimestamp) >= 0) {
        memcpy(&g_touchEntry, newTouchEntry, sizeof(HidTouchScreenEntry));
        g_touchTimestamp = newTouchEntry->header.timestamp;

        if (hidTouchCount())
            g_controllerHeld[CONTROLLER_HANDHELD] |= KEY_TOUCH;
    }

    u64 latestMouseEntry = sharedMem->mouse.header.latestEntry;
    HidMouseEntry *newMouseEntry = &sharedMem->mouse.entries[latestMouseEntry];
    memcpy(&g_mouse, &sharedMem->mouse, sizeof(HidMouse));
    if ((s64)(newMouseEntry->timestamp - g_mouseTimestamp) >= 0) {
        g_mouseEntry = &g_mouse.entries[latestMouseEntry];
        g_mouseTimestamp = newMouseEntry->timestamp;

        g_mouseHeld = g_mouseEntry->buttons;
    }
    g_mouseDown = (~g_mouseOld) & g_mouseHeld;
    g_mouseUp = g_mouseOld & (~g_mouseHeld);

    u64 latestKeyboardEntry = sharedMem->keyboard.header.latestEntry;
    HidKeyboardEntry *newKeyboardEntry = &sharedMem->keyboard.entries[latestKeyboardEntry];
    if ((s64)(newKeyboardEntry->timestamp - g_keyboardTimestamp) >= 0) {
        memcpy(&g_keyboardEntry, newKeyboardEntry, sizeof(HidKeyboardEntry));
        g_keyboardTimestamp = newKeyboardEntry->timestamp;

        g_keyboardModHeld = g_keyboardEntry.modifier;
        for (int i = 0; i < 8; i++) {
            g_keyboardHeld[i] = g_keyboardEntry.keys[i];
        }
    }
    g_keyboardModDown = (~g_keyboardModOld) & g_keyboardModHeld;
    g_keyboardModUp = g_keyboardModOld & (~g_keyboardModHeld);
    for (int i = 0; i < 8; i++) {
        g_keyboardDown[i] = (~g_keyboardOld[i]) & g_keyboardHeld[i];
        g_keyboardUp[i] = g_keyboardOld[i] & (~g_keyboardHeld[i]);
    }

    for (int i = 0; i < 10; i++) {
        HidControllerLayout *currentLayout = &sharedMem->controllers[i].layouts[g_controllerLayout[i]];
        memcpy(&g_controllerHeaders[i], &sharedMem->controllers[i].header, sizeof(HidControllerHeader));
        u64 latestControllerEntry = currentLayout->header.latestEntry;
        HidControllerInputEntry *newInputEntry = &currentLayout->entries[latestControllerEntry];
        if ((s64)(newInputEntry->timestamp - g_controllerTimestamps[i]) >= 0) {
            memcpy(&g_controllerEntries[i], newInputEntry, sizeof(HidControllerInputEntry));
            g_controllerTimestamps[i] = newInputEntry->timestamp;

            g_controllerHeld[i] |= g_controllerEntries[i].buttons;
        }

        g_controllerDown[i] = (~g_controllerOld[i]) & g_controllerHeld[i];
        g_controllerUp[i] = g_controllerOld[i] & (~g_controllerHeld[i]);

        memcpy(&g_controllerMisc[i], &sharedMem->controllers[i].misc, sizeof(HidControllerMisc));

        if (g_sixaxisEnabled[i]) {
            u32 type = g_controllerHeaders[i].type;
            HidControllerSixAxisLayout *sixaxis = NULL;
            if (type & TYPE_PROCONTROLLER) {
                sixaxis = &sharedMem->controllers[i].sixaxis[0];
            }
            else if (type & TYPE_HANDHELD) {
                sixaxis = &sharedMem->controllers[i].sixaxis[1];
            }
            else if (type & TYPE_JOYCON_PAIR) {
                if (type & TYPE_JOYCON_LEFT) {
                    sixaxis = &sharedMem->controllers[i].sixaxis[2];
                }
                else {
                    sixaxis = &sharedMem->controllers[i].sixaxis[3];
                }
            }
            else if (type & TYPE_JOYCON_LEFT) {
                sixaxis = &sharedMem->controllers[i].sixaxis[4];
            }
            else if (type & TYPE_JOYCON_RIGHT) {
                sixaxis = &sharedMem->controllers[i].sixaxis[5];
            }
            if (sixaxis) {
                memcpy(&g_sixaxisLayouts[i], sixaxis, sizeof(*sixaxis));
            }
        }
    }

    g_controllerP1AutoID = CONTROLLER_HANDHELD;
    if (g_controllerEntries[CONTROLLER_PLAYER_1].connectionState & CONTROLLER_STATE_CONNECTED)
       g_controllerP1AutoID = CONTROLLER_PLAYER_1;

    rwlockWriteUnlock(&g_hidLock);
}

HidControllerType hidGetControllerType(HidControllerID id) {
    if (id==CONTROLLER_P1_AUTO) return hidGetControllerType(g_controllerP1AutoID);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    u32 tmp = g_controllerHeaders[id].type;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

void hidGetControllerColors(HidControllerID id, HidControllerColors *colors) {
    if (id==CONTROLLER_P1_AUTO) {
        hidGetControllerColors(g_controllerP1AutoID, colors);
        return;
    }
    if (id < 0 || id > 9) return;
    if (colors == NULL) return;

    HidControllerHeader *hdr = &g_controllerHeaders[id];

    memset(colors, 0, sizeof(HidControllerColors));

    rwlockReadLock(&g_hidLock);

    colors->singleSet = (hdr->singleColorsDescriptor & BIT(1)) == 0;
    colors->splitSet = (hdr->splitColorsDescriptor & BIT(1)) == 0;

    if (colors->singleSet) {
        colors->singleColorBody = hdr->singleColorBody;
        colors->singleColorButtons = hdr->singleColorButtons;
    }

    if (colors->splitSet) {
        colors->leftColorBody = hdr->leftColorBody;
        colors->leftColorButtons = hdr->leftColorButtons;
        colors->rightColorBody = hdr->rightColorBody;
        colors->rightColorButtons = hdr->rightColorButtons;
    }

    rwlockReadUnlock(&g_hidLock);
}

bool hidIsControllerConnected(HidControllerID id) {
    if (id==CONTROLLER_P1_AUTO)
        return hidIsControllerConnected(g_controllerP1AutoID);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    bool flag = (g_controllerEntries[id].connectionState & CONTROLLER_STATE_CONNECTED) != 0;
    rwlockReadUnlock(&g_hidLock);
    return flag;
}

u32 hidGetControllerDeviceType(HidControllerID id) {
    if (id==CONTROLLER_P1_AUTO)
        return hidGetControllerDeviceType(g_controllerP1AutoID);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    u32 type = g_controllerMisc[id].deviceType;
    rwlockReadUnlock(&g_hidLock);
    return type;
}

void hidGetControllerFlags(HidControllerID id, HidFlags *flags) {
    if (id==CONTROLLER_P1_AUTO) {
        hidGetControllerFlags(g_controllerP1AutoID, flags);
        return;
    }
    if (id < 0 || id > 9) return;

    rwlockReadLock(&g_hidLock);
    *flags = g_controllerMisc[id].flags;
    rwlockReadUnlock(&g_hidLock);
}

void hidGetControllerPowerInfo(HidControllerID id, HidPowerInfo *info, size_t total_info) {
    size_t i;
    size_t indexbase;
    HidFlags flags;

    if (id==CONTROLLER_P1_AUTO) {
        hidGetControllerPowerInfo(g_controllerP1AutoID, info, total_info);
        return;
    }
    if (id < 0 || id > 9) return;

    if (total_info == 0) return;
    if (total_info > 2) total_info = 2;
    indexbase = total_info-1;

    hidGetControllerFlags(id, &flags);

    for (i=0; i<total_info; i++) {
        info[i].isCharging = (flags.powerInfo & BIT(indexbase+i)) != 0;
        info[i].powerConnected = (flags.powerInfo & BIT(indexbase+i+3)) != 0;

        rwlockReadLock(&g_hidLock);
        info[i].batteryCharge = g_controllerMisc[id].batteryCharge[indexbase+i];
        rwlockReadUnlock(&g_hidLock);
        if (info[i].batteryCharge > 4) info->batteryCharge = 4;
    }
}

u64 hidKeysHeld(HidControllerID id) {
    if (id==CONTROLLER_P1_AUTO) return hidKeysHeld(g_controllerP1AutoID);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    u64 tmp = g_controllerHeld[id];
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

u64 hidKeysDown(HidControllerID id) {
    if (id==CONTROLLER_P1_AUTO) return hidKeysDown(g_controllerP1AutoID);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    u64 tmp = g_controllerDown[id];
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

u64 hidKeysUp(HidControllerID id) {
    if (id==CONTROLLER_P1_AUTO) return hidKeysUp(g_controllerP1AutoID);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    u64 tmp = g_controllerUp[id];
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

u64 hidMouseButtonsHeld(void) {
    rwlockReadLock(&g_hidLock);
    u64 tmp = g_mouseHeld;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

u64 hidMouseButtonsDown(void) {
    rwlockReadLock(&g_hidLock);
    u64 tmp = g_mouseDown;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

u64 hidMouseButtonsUp(void) {
    rwlockReadLock(&g_hidLock);
    u64 tmp = g_mouseUp;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

void hidMouseRead(MousePosition *pos) {
    rwlockReadLock(&g_hidLock);
    *pos = g_mouseEntry->position;
    rwlockReadUnlock(&g_hidLock);
}

u32 hidMouseMultiRead(MousePosition *entries, u32 num_entries) {
    int entry;
    int i;

    if (!entries || !num_entries) return 0;

    memset(entries, 0, sizeof(MousePosition) * num_entries);

    rwlockReadLock(&g_hidLock);

    if (num_entries > g_mouse.header.maxEntryIndex + 1)
        num_entries = g_mouse.header.maxEntryIndex + 1;

    entry = g_mouse.header.latestEntry + 1 - num_entries;
    if (entry < 0)
        entry += g_mouse.header.maxEntryIndex + 1;

    u64 timestamp = 0;
    for (i = 0; i < num_entries; i++) {
        if (timestamp && g_mouse.entries[entry].timestamp - timestamp != 1)
            break;
        memcpy(&entries[i], &g_mouse.entries[entry].position, sizeof(MousePosition));
        timestamp = g_mouse.entries[entry].timestamp;

        entry++;
        if (entry > g_mouse.header.maxEntryIndex)
            entry = 0;
    }

    rwlockReadUnlock(&g_hidLock);

    return i;
}

bool hidKeyboardModifierHeld(HidKeyboardModifier modifier) {
    rwlockReadLock(&g_hidLock);
    bool tmp = g_keyboardModHeld & modifier;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

bool hidKeyboardModifierDown(HidKeyboardModifier modifier) {
    rwlockReadLock(&g_hidLock);
    bool tmp = g_keyboardModDown & modifier;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

bool hidKeyboardModifierUp(HidKeyboardModifier modifier) {
    rwlockReadLock(&g_hidLock);
    bool tmp = g_keyboardModUp & modifier;
    rwlockReadUnlock(&g_hidLock);

    return tmp;
}

bool hidKeyboardHeld(HidKeyboardScancode key) {
    rwlockReadLock(&g_hidLock);
    u32 tmp = g_keyboardHeld[key / 32] & (1 << (key % 32));
    rwlockReadUnlock(&g_hidLock);

    return !!tmp;
}

bool hidKeyboardDown(HidKeyboardScancode key) {
    rwlockReadLock(&g_hidLock);
    u32 tmp = g_keyboardDown[key / 32] & (1 << (key % 32));
    rwlockReadUnlock(&g_hidLock);

    return !!tmp;
}

bool hidKeyboardUp(HidKeyboardScancode key) {
    rwlockReadLock(&g_hidLock);
    u32 tmp = g_keyboardUp[key / 32] & (1 << (key % 32));
    rwlockReadUnlock(&g_hidLock);

    return !!tmp;
}

u32 hidTouchCount(void) {
    return g_touchEntry.header.numTouches;
}

void hidTouchRead(touchPosition *pos, u32 point_id) {
    if (pos) {
        if (point_id >= g_touchEntry.header.numTouches) {
            memset(pos, 0, sizeof(touchPosition));
            return;
        }

        pos->id = g_touchEntry.touches[point_id].touchIndex;
        pos->px = g_touchEntry.touches[point_id].x;
        pos->py = g_touchEntry.touches[point_id].y;
        pos->dx = g_touchEntry.touches[point_id].diameterX;
        pos->dy = g_touchEntry.touches[point_id].diameterY;
        pos->angle = g_touchEntry.touches[point_id].angle;
    }
}

void hidJoystickRead(JoystickPosition *pos, HidControllerID id, HidControllerJoystick stick) {
    if (id == CONTROLLER_P1_AUTO) return hidJoystickRead(pos, g_controllerP1AutoID, stick);

    if (pos) {
        if (id < 0 || id > 9 || stick >= JOYSTICK_NUM_STICKS) {
            memset(pos, 0, sizeof(touchPosition));
            return;
        }

        rwlockReadLock(&g_hidLock);
        pos->dx = g_controllerEntries[id].joysticks[stick].dx;
        pos->dy = g_controllerEntries[id].joysticks[stick].dy;
        rwlockReadUnlock(&g_hidLock);
    }
}

u32 hidSixAxisSensorValuesRead(SixAxisSensorValues *values, HidControllerID id, u32 num_entries) {
    int entry;
    int i;

    if (!values || !num_entries) return 0;

    if (id == CONTROLLER_P1_AUTO) return hidSixAxisSensorValuesRead(values, g_controllerP1AutoID, num_entries);

    memset(values, 0, sizeof(SixAxisSensorValues) * num_entries);
    if (id < 0 || id > 9) return 0;

    rwlockReadLock(&g_hidLock);
    if (!g_sixaxisEnabled[id]) {
        rwlockReadUnlock(&g_hidLock);
        return 0;
    }

    if (num_entries > g_sixaxisLayouts[id].header.maxEntryIndex + 1)
        num_entries = g_sixaxisLayouts[id].header.maxEntryIndex + 1;

    entry = g_sixaxisLayouts[id].header.latestEntry + 1 - num_entries;
    if (entry < 0)
        entry += g_sixaxisLayouts[id].header.maxEntryIndex + 1;

    u64 timestamp = 0;
    for (i = 0; i < num_entries; i++) {
        if (timestamp && g_sixaxisLayouts[id].entries[entry].timestamp - timestamp != 1)
            break;
        memcpy(&values[i], &g_sixaxisLayouts[id].entries[entry].values, sizeof(SixAxisSensorValues));
        timestamp = g_sixaxisLayouts[id].entries[entry].timestamp;

        entry++;
        if (entry > g_sixaxisLayouts[id].header.maxEntryIndex)
            entry = 0;
    }
    rwlockReadUnlock(&g_hidLock);

    return i;
}

bool hidGetHandheldMode(void) {
    return g_controllerP1AutoID == CONTROLLER_HANDHELD;
}

static Result _hidSetDualModeAll(void) {
    Result rc;
    int i;

    for (i=0; i<8; i++) {
        rc = hidSetNpadJoyAssignmentModeDual(i);
        if (R_FAILED(rc)) break;
    }

    return rc;
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

static Result _hidCmdWithNoInput(u32 cmd_id) {
    Result rc=0;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

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

static Result _hidCmdWithInputU32(u32 inval, u32 cmd_id) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u32 inval;
        u64 AppletResourceUserId;
    } in = { inval, AppletResourceUserId };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdWithInputU64(u64 inval, u32 cmd_id) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u64 AppletResourceUserId;
        u64 inval;
    } in = { AppletResourceUserId, inval };

    return serviceDispatchIn(&g_hidSrv, cmd_id, in,
        .in_send_pid = true,
    );
}

static Result _hidCmdOutU32(u32 *out, u32 cmd_id) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    return serviceDispatchInOut(&g_hidSrv, cmd_id, AppletResourceUserId, *out,
        .in_send_pid = true,
    );
}

static Result _hidCmdOutU64(u64 *out, u32 cmd_id) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

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

static Result _hidCreateAppletResource(Service* srv, Service* srv_out, u64 AppletResourceUserId) {
    return serviceDispatchIn(srv, 0, AppletResourceUserId,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _hidGetSharedMemoryHandle(Service* srv, Handle* handle_out) {
    return _hidCmdGetHandle(srv, handle_out, 0);
}

Result hidSetSupportedNpadStyleSet(HidControllerType type) {
    return _hidCmdWithInputU32(type, 100);
}

Result hidGetSupportedNpadStyleSet(HidControllerType *type) {
    u32 tmp=0;
    Result rc = _hidCmdOutU32(&tmp, 101);
    if (R_SUCCEEDED(rc) && type) *type = tmp;
    return rc;
}

Result hidSetSupportedNpadIdType(HidControllerID *buf, size_t count) {
    Result rc;
    u64 AppletResourceUserId;
    size_t i;
    u32 tmpval=0;
    u32 tmpbuf[10];

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    if (count > 10)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    memset(tmpbuf, 0, sizeof(tmpbuf));
    for (i=0; i<count; i++) {
        tmpval = buf[i];
        if (tmpval == CONTROLLER_HANDHELD) {
            tmpval = 0x20;
        }
        else if (tmpval >= CONTROLLER_UNKNOWN) {
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        }

        tmpbuf[i] = tmpval;
    }

    return serviceDispatchIn(&g_hidSrv, 102, AppletResourceUserId,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { tmpbuf, count*sizeof(u32) } },
        .in_send_pid = true,
    );
}

static Result _hidActivateNpad(void) {
    u32 revision=0x0;

    if (hosversionBefore(5,0,0))
        return _hidCmdWithNoInput(103); // ActivateNpad

    revision = 0x1; // [5.0.0+]
    if (hosversionAtLeast(6,0,0))
        revision = 0x2; // [6.0.0+]
    if (hosversionAtLeast(8,0,0))
        revision = 0x3; // [8.0.0+]

    return _hidCmdWithInputU32(revision, 109); // ActivateNpadWithRevision
}

static Result _hidDeactivateNpad(void) {
    return _hidCmdWithNoInput(104);
}

Result hidAcquireNpadStyleSetUpdateEventHandle(HidControllerID id, Event* out_event, bool autoclear) {
    Result rc;
    Handle tmp_handle = INVALID_HANDLE;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u32 id;
        u32 pad;
        u64 AppletResourceUserId;
        u64 event_ptr; // Official sw sets this to a ptr, which the sysmodule doesn't seem to use.
    } in = { hidControllerIDToOfficial(id), 0, AppletResourceUserId, 0 };

    rc = serviceDispatchIn(&g_hidSrv, 106, in,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

Result hidSetNpadJoyHoldType(HidJoyHoldType type) {
    return _hidCmdWithInputU64(type, 120);
}

Result hidGetNpadJoyHoldType(HidJoyHoldType *type) {
    u64 tmp=0;
    Result rc = _hidCmdOutU64(&tmp, 121);
    if (R_SUCCEEDED(rc) && type) *type = tmp;
    return rc;
}

Result hidSetNpadJoyAssignmentModeSingleByDefault(HidControllerID id) {
    return _hidCmdWithInputU32(hidControllerIDToOfficial(id), 122);
}

Result hidSetNpadJoyAssignmentModeDual(HidControllerID id) {
    return _hidCmdWithInputU32(hidControllerIDToOfficial(id), 124);
}

Result hidMergeSingleJoyAsDualJoy(HidControllerID id0, HidControllerID id1) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u32 id0, id1;
        u64 AppletResourceUserId;
    } in = { hidControllerIDToOfficial(id0), hidControllerIDToOfficial(id1), AppletResourceUserId };

    return serviceDispatchIn(&g_hidSrv, 125, in,
        .in_send_pid = true,
    );
}

static Result _hidCreateActiveVibrationDeviceList(Service* srv_out) {
    return _hidCmdGetSession(srv_out, 203);
}

static Result _hidActivateVibrationDevice(Service* srv, u32 VibrationDeviceHandle) {
    return _hidCmdInU32NoOut(srv, VibrationDeviceHandle, 0);
}

Result hidGetVibrationDeviceInfo(const u32 *VibrationDeviceHandle, HidVibrationDeviceInfo *VibrationDeviceInfo) {
    return serviceDispatchInOut(&g_hidSrv, 200, *VibrationDeviceHandle, *VibrationDeviceInfo);
}

Result hidSendVibrationValue(const u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u32 VibrationDeviceHandle;
        HidVibrationValue VibrationValue;
        u32 pad;
        u64 AppletResourceUserId;
    } in = { *VibrationDeviceHandle, *VibrationValue, 0, AppletResourceUserId };

    return serviceDispatchIn(&g_hidSrv, 201, in,
        .in_send_pid = true,
    );
}

Result hidGetActualVibrationValue(const u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u32 VibrationDeviceHandle;
        u64 AppletResourceUserId;
    } in = { *VibrationDeviceHandle, AppletResourceUserId };

    return serviceDispatchInOut(&g_hidSrv, 202, in, *VibrationValue,
        .in_send_pid = true,
    );
}

Result hidPermitVibration(bool flag) {
    return _hidCmdInBoolNoOut(flag, 204);
}

Result hidIsVibrationPermitted(bool *flag) {
    return _hidCmdNoInOutBool(flag, 205);
}

Result hidSendVibrationValues(const u32 *VibrationDeviceHandles, HidVibrationValue *VibrationValues, s32 count) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    return serviceDispatchIn(&g_hidSrv, 206, AppletResourceUserId,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { VibrationDeviceHandles, count*sizeof(u32) },
            { VibrationValues, count*sizeof(HidVibrationValue) },
        },
    );
}

Result hidIsVibrationDeviceMounted(const u32 *VibrationDeviceHandle, bool *flag) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u32 VibrationDeviceHandle;
        u64 AppletResourceUserId;
    } in = { *VibrationDeviceHandle, AppletResourceUserId };

    u8 tmp=0;
    rc = serviceDispatchInOut(&g_hidSrv, 211, in, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && flag) *flag = tmp & 1;
    return rc;
}

static Result _hidGetDeviceHandles(u32 devicetype, u32 *DeviceHandles, s32 total_handles, HidControllerID id, HidControllerType type) {
    Result rc=0;
    u32 tmp_type = type & 0xff;
    u32 tmp_id = id;

    if (total_handles <= 0 || total_handles > 2 || devicetype > 1)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (tmp_id == CONTROLLER_HANDHELD)
        tmp_id = 0x20;

    if (tmp_type & TYPE_PROCONTROLLER) {
        tmp_type = 3;
    }
    else if (tmp_type & TYPE_HANDHELD) {
        tmp_type = 4;
    }
    else if (tmp_type & TYPE_JOYCON_PAIR) {
        tmp_type = 5;
    }
    else if (tmp_type & TYPE_JOYCON_LEFT) {
        tmp_type = 6;
    }
    else if (tmp_type & TYPE_JOYCON_RIGHT) {
        tmp_type = 7;
        tmp_type |= 0x010000;
    }
    else if (tmp_type & TYPE_SYSTEM_EXT) {
        tmp_type = 0x20;
    }
    else if (tmp_type & TYPE_SYSTEM) {
        tmp_type = 0x21;
    }
    else {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    DeviceHandles[0] = tmp_type | (tmp_id & 0xff)<<8;

    if (devicetype==1 && (tmp_type==3 || tmp_type==4))
        DeviceHandles[0] |= 0x020000;

    if (total_handles > 1) {
        tmp_type &= 0xff;
        if (devicetype==0 && (tmp_type!=6 && tmp_type!=7)) {
            DeviceHandles[1] = DeviceHandles[0] | 0x010000;
        }
        else if (devicetype==1 && tmp_type==5) {
            DeviceHandles[1] = DeviceHandles[0] | 0x010000;
        }
        else {
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        }
    }

    return rc;
}

Result hidInitializeVibrationDevices(u32 *VibrationDeviceHandles, s32 total_handles, HidControllerID id, HidControllerType type) {
    Result rc=0;
    Service srv;
    s32 i;

    rc = _hidGetDeviceHandles(0, VibrationDeviceHandles, total_handles, id, type);
    if (R_FAILED(rc)) return rc;

    for (i=0; i<total_handles; i++) {
        rc = _hidCreateActiveVibrationDeviceList(&srv);
        if (R_FAILED(rc))
            break;

        rc = _hidActivateVibrationDevice(&srv, VibrationDeviceHandles[i]);
        serviceClose(&srv);

        if (R_FAILED(rc))
            break;
    }

    return rc;
}

Result hidGetSixAxisSensorHandles(u32 *SixAxisSensorHandles, s32 total_handles, HidControllerID id, HidControllerType type) {
    return _hidGetDeviceHandles(1, SixAxisSensorHandles, total_handles, id, type);
}

Result hidStartSixAxisSensor(u32 SixAxisSensorHandle) {
    u32 rc = _hidCmdWithInputU32(SixAxisSensorHandle, 66);
    if (R_SUCCEEDED(rc)) {
        int controller = (SixAxisSensorHandle >> 8) & 0xff;
        if (controller == 0x20)
            controller = CONTROLLER_HANDHELD;
        if (controller < 10) {
            rwlockWriteLock(&g_hidLock);
            g_sixaxisEnabled[controller] = true;
            rwlockWriteUnlock(&g_hidLock);
        }
    }
    return rc;
}

Result hidStopSixAxisSensor(u32 SixAxisSensorHandle) {
    u32 rc = _hidCmdWithInputU32(SixAxisSensorHandle, 67);
    if (R_SUCCEEDED(rc)) {
        int controller = (SixAxisSensorHandle >> 8) & 0xff;
        if (controller == 0x20)
            controller = CONTROLLER_HANDHELD;
        if (controller < 10) {
            rwlockWriteLock(&g_hidLock);
            g_sixaxisEnabled[controller] = false;
            rwlockWriteUnlock(&g_hidLock);
        }
    }
    return rc;
}

static Result _hidActivateConsoleSixAxisSensor(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdWithNoInput(303);
}

Result hidStartSevenSixAxisSensor(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdWithNoInput(304);
}

Result hidStopSevenSixAxisSensor(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdWithNoInput(305);
}

static Result _hidInitializeSevenSixAxisSensor(TransferMemory *tmem0, TransferMemory *tmem1) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        u64 AppletResourceUserId;
        u64 size0;
        u64 size1;
    } in = { AppletResourceUserId, tmem0->size, tmem1->size };

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

    rc = _hidActivateConsoleSixAxisSensor();
    if (R_FAILED(rc)) return rc;

    g_sevenSixAxisSensorBuffer = (u8*)memalign(0x1000, bufsize);
    if (g_sevenSixAxisSensorBuffer == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    memset(g_sevenSixAxisSensorBuffer, 0, bufsize);

    rc = tmemCreateFromMemory(&g_sevenSixAxisSensorTmem0, g_sevenSixAxisSensorBuffer, 0x1000, Perm_R);
    if (R_SUCCEEDED(rc)) rc = tmemCreateFromMemory(&g_sevenSixAxisSensorTmem1, &g_sevenSixAxisSensorBuffer[0x1000], bufsize-0x1000, Perm_None);

    if (R_SUCCEEDED(rc)) rc = _hidInitializeSevenSixAxisSensor(&g_sevenSixAxisSensorTmem0, &g_sevenSixAxisSensorTmem1);

    if (R_FAILED(rc)) {
        tmemClose(&g_sevenSixAxisSensorTmem0);
        tmemClose(&g_sevenSixAxisSensorTmem1);

        free(g_sevenSixAxisSensorBuffer);
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

    rc = _hidCmdWithNoInput(307);

    tmemClose(&g_sevenSixAxisSensorTmem0);
    tmemClose(&g_sevenSixAxisSensorTmem1);

    free(g_sevenSixAxisSensorBuffer);
    g_sevenSixAxisSensorBuffer = NULL;

    return rc;
}

Result hidSetSevenSixAxisSensorFusionStrength(float strength) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    const struct {
        float strength;
        u64 AppletResourceUserId;
    } in = { strength, AppletResourceUserId };

    return serviceDispatchIn(&g_hidSrv, 308, in,
        .in_send_pid = true,
    );
}

Result hidGetSevenSixAxisSensorFusionStrength(float *strength) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    return serviceDispatchInOut(&g_hidSrv, 309, AppletResourceUserId, *strength,
        .in_send_pid = true,
    );
}

Result hidResetSevenSixAxisSensorTimestamp(void) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidCmdWithNoInput(310);
}

Result hidGetNpadInterfaceType(HidControllerID id, u8 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp = hidControllerIDToOfficial(id);
    return serviceDispatchInOut(&g_hidSrv, 405, tmp, *out);
}

