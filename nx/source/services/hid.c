#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/shmem.h"
#include "kernel/rwlock.h"
#include "kernel/event.h"
#include "services/applet.h"
#include "services/hid.h"
#include "services/sm.h"

static Service g_hidSrv;
static Service g_hidIAppletResource;
static u64 g_refCnt;
static SharedMemory g_hidSharedmem;

static HidTouchScreenEntry g_touchEntry;
static HidMouseEntry *g_mouseEntry;
static HidMouse g_mouse;
static HidKeyboardEntry g_keyboardEntry;
static HidControllerHeader g_controllerHeaders[10];
static HidControllerInputEntry g_controllerEntries[10];
static HidControllerSixAxisLayout g_sixaxisLayouts[10];

static u64 g_mouseOld, g_mouseHeld, g_mouseDown, g_mouseUp;
static u64 g_keyboardModOld, g_keyboardModHeld, g_keyboardModDown, g_keyboardModUp;
static u32 g_keyboardOld[8], g_keyboardHeld[8], g_keyboardDown[8], g_keyboardUp[8];
static u64 g_controllerOld[10], g_controllerHeld[10], g_controllerDown[10], g_controllerUp[10];
static bool g_sixaxisEnabled[10];

static HidControllerLayoutType g_controllerLayout[10];
static u64 g_touchTimestamp, g_mouseTimestamp, g_keyboardTimestamp, g_controllerTimestamps[10];

static HidControllerID g_controllerP1AutoID;

static RwLock g_hidLock;

static Result _hidCreateAppletResource(Service* srv, Service* srv_out, u64 AppletResourceUserId);
static Result _hidGetSharedMemoryHandle(Service* srv, Handle* handle_out);

static Result _hidActivateNpad(void);
static Result _hidDeactivateNpad(void);

static Result _hidSetDualModeAll(void);

Result hidInitialize(void)
{
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

    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_hidSrv))
        return 0;

    Result rc;
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

    if (R_SUCCEEDED(rc))
    {
        shmemLoadRemote(&g_hidSharedmem, sharedmem_handle, 0x40000, Perm_R);

        rc = shmemMap(&g_hidSharedmem);
    }

    if (R_SUCCEEDED(rc))
        rc = _hidActivateNpad();

    if (R_SUCCEEDED(rc))
        rc = hidSetSupportedNpadStyleSet(TYPE_PROCONTROLLER | TYPE_HANDHELD | TYPE_JOYCON_PAIR | TYPE_JOYCON_LEFT | TYPE_JOYCON_RIGHT);

    if (R_SUCCEEDED(rc))
        rc = hidSetSupportedNpadIdType(idbuf, 9);

    if (R_SUCCEEDED(rc))
        rc = _hidSetDualModeAll();

    if (R_SUCCEEDED(rc))
        rc = hidSetNpadJoyHoldType(HidJoyHoldType_Default);

    if (R_FAILED(rc))
        hidExit();

    hidReset();
    return rc;
}

void hidExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        hidSetNpadJoyHoldType(HidJoyHoldType_Default);

        _hidSetDualModeAll();

        _hidDeactivateNpad();

        serviceClose(&g_hidIAppletResource);
        serviceClose(&g_hidSrv);
        shmemClose(&g_hidSharedmem);
    }
}

void hidReset(void)
{
    rwlockWriteLock(&g_hidLock);

    // Reset internal state
    memset(&g_touchEntry, 0, sizeof(HidTouchScreenEntry));
    memset(&g_mouse, 0, sizeof(HidMouse));
    memset(&g_keyboardEntry, 0, sizeof(HidKeyboardEntry));
    memset(g_controllerHeaders, 0, sizeof(g_controllerHeaders));
    memset(g_controllerEntries, 0, sizeof(g_controllerEntries));
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

Service* hidGetSessionService(void) {
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

static Result _hidCreateAppletResource(Service* srv, Service* srv_out, u64 AppletResourceUserId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->AppletResourceUserId = AppletResourceUserId;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(srv_out, r.Handles[0]);
        }
    }

    return rc;
}

static Result _hidGetSharedMemoryHandle(Service* srv, Handle* handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

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

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    ipcAddSendStatic(&c, tmpbuf, sizeof(u32)*count, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 102;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidCmdWithInputU32(u64 cmd_id, u32 inputval) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 val;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->val = inputval;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidCmdWithInputU64(u64 cmd_id, u64 inputval) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
        u64 val;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->val = inputval;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidCmdWithNoInput(u64 cmd_id) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hidSetSupportedNpadStyleSet(HidControllerType type) {
    return _hidCmdWithInputU32(100, type);
}

static Result _hidActivateNpad(void) {
    return _hidCmdWithNoInput(103);
}

static Result _hidDeactivateNpad(void) {
    return _hidCmdWithNoInput(104);
}

Result hidAcquireNpadStyleSetUpdateEventHandle(HidControllerID id, Event* event, bool autoclear) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 id;
        u64 AppletResourceUserId;
        u64 event_ptr;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 106;
    raw->id = hidControllerIDToOfficial(id);
    raw->AppletResourceUserId = AppletResourceUserId;
    raw->event_ptr = 0;//Official sw sets this to a ptr, which the sysmodule doesn't seem to use.

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event, r.Handles[0], autoclear);
        }
    }

    return rc;
}

Result hidSetNpadJoyHoldType(HidJoyHoldType type) {
    return _hidCmdWithInputU64(120, type);
}

Result hidSetNpadJoyAssignmentModeSingleByDefault(HidControllerID id) {
    return _hidCmdWithInputU32(122, id);
}

Result hidSetNpadJoyAssignmentModeDual(HidControllerID id) {
    return _hidCmdWithInputU32(124, id);
}

Result hidMergeSingleJoyAsDualJoy(HidControllerID id0, HidControllerID id1) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 id0, id1;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 125;
    raw->id0 = id0;
    raw->id1 = id1;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidCreateActiveVibrationDeviceList(Service* srv_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 203;

    Result rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(srv_out, r.Handles[0]);
        }
    }

    return rc;
}

static Result _hidActivateVibrationDevice(Service* srv, u32 VibrationDeviceHandle) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 VibrationDeviceHandle;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->VibrationDeviceHandle = VibrationDeviceHandle;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hidGetVibrationDeviceInfo(u32 *VibrationDeviceHandle, HidVibrationDeviceInfo *VibrationDeviceInfo) {
    Result rc;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 VibrationDeviceHandle;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 200;
    raw->VibrationDeviceHandle = *VibrationDeviceHandle;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            HidVibrationDeviceInfo VibrationDeviceInfo;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && VibrationDeviceInfo) memcpy(VibrationDeviceInfo, &resp->VibrationDeviceInfo, sizeof(HidVibrationDeviceInfo));
    }

    return rc;
}

Result hidSendVibrationValue(u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 VibrationDeviceHandle;
        HidVibrationValue VibrationValue;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 201;
    raw->VibrationDeviceHandle = *VibrationDeviceHandle;
    raw->AppletResourceUserId = AppletResourceUserId;
    memcpy(&raw->VibrationValue, VibrationValue, sizeof(HidVibrationValue));

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hidGetActualVibrationValue(u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 VibrationDeviceHandle;
        u64 AppletResourceUserId;
    } *raw;

    ipcSendPid(&c);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;
    raw->VibrationDeviceHandle = *VibrationDeviceHandle;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            HidVibrationValue VibrationValue;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && VibrationValue) memcpy(VibrationValue, &resp->VibrationValue, sizeof(HidVibrationValue));
    }

    return rc;
}

Result hidPermitVibration(bool flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 204;
    raw->flag = !!flag;

    Result rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result hidIsVibrationPermitted(bool *flag) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 205;

    Result rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 flag;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && flag)
            *flag = resp->flag & 1;
    }

    return rc;
}

Result hidSendVibrationValues(u32 *VibrationDeviceHandles, HidVibrationValue *VibrationValues, size_t count) {
    Result rc;
    u64 AppletResourceUserId;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc))
        AppletResourceUserId = 0;

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

    ipcAddSendStatic(&c, VibrationDeviceHandles, sizeof(u32)*count, 0);
    ipcAddSendStatic(&c, VibrationValues, sizeof(HidVibrationValue)*count, 0);

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 206;
    raw->AppletResourceUserId = AppletResourceUserId;

    rc = serviceIpcDispatch(&g_hidSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidGetDeviceHandles(u32 devicetype, u32 *DeviceHandles, size_t total_handles, HidControllerID id, HidControllerType type) {
    Result rc=0;
    u32 tmp_type = type & 0xff;
    u32 tmp_id = id;

    if (total_handles == 0 || total_handles > 2 || devicetype > 1)
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
    //Official sw checks for these bits but libnx hid.h doesn't have these currently.
    else if (tmp_type & BIT(29)) {
        tmp_type = 0x20;
    }
    else if (tmp_type & BIT(30)) {
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

Result hidInitializeVibrationDevices(u32 *VibrationDeviceHandles, size_t total_handles, HidControllerID id, HidControllerType type) {
    Result rc=0;
    Service srv;
    size_t i;

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

Result hidGetSixAxisSensorHandles(u32 *SixAxisSensorHandles, size_t total_handles, HidControllerID id, HidControllerType type) {
    return _hidGetDeviceHandles(1, SixAxisSensorHandles, total_handles, id, type);
}

Result hidStartSixAxisSensor(u32 SixAxisSensorHandle) {
    u32 rc = _hidCmdWithInputU32(66, SixAxisSensorHandle);
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
    u32 rc = _hidCmdWithInputU32(67, SixAxisSensorHandle);
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

