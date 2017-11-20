#include <string.h>
#include <switch.h>

static Handle g_hidServiceSession = INVALID_HANDLE;
static Handle g_hidIAppletResource = INVALID_HANDLE;
static SharedMemory g_hidSharedmem;

static HIDTouchScreenEntry touchEntry;
static HIDMouseEntry mouseEntry;
static HIDKeyboardEntry keyboardEntry;
static HIDControllerInputEntry controllerEntries[10];

static u64 mouseOld, mouseHeld, mouseDown, mouseUp;
static u64 keyboardModOld, keyboardModHeld, keyboardModDown, keyboardModUp;
static u32 keyboardOld[8], keyboardHeld[8], keyboardDown[8], keyboardUp[8];
static u64 controllerOld[10], controllerHeld[10], controllerDown[10], controllerUp[10];

static HIDControllerLayoutType controllerLayout[10];
static u64 touchTimestamp, mouseTimestamp, keyboardTimestamp, controllerTimestamps[10];

static Result _hidCreateAppletResource(Handle sessionhandle, Handle* handle_out, u64 AppletResourceUserId);
static Result _hidGetSharedMemoryHandle(Handle sessionhandle, Handle* handle_out);

Result hidInitialize(void) {
    if (g_hidServiceSession != INVALID_HANDLE) return MAKERESULT(MODULE_LIBNX, LIBNX_ALREADYINITIALIZED);

    Result rc = 0;
    u64 AppletResourceUserId = 0;
    Handle sharedmem_handle=0;

    rc = appletGetAppletResourceUserId(&AppletResourceUserId);
    if (R_FAILED(rc)) return rc;

    rc = smGetService(&g_hidServiceSession, "hid");
    if (R_FAILED(rc)) return rc;

    rc = _hidCreateAppletResource(g_hidServiceSession, &g_hidIAppletResource, AppletResourceUserId);

    if (R_SUCCEEDED(rc)) rc = _hidGetSharedMemoryHandle(g_hidIAppletResource, &sharedmem_handle);

    if (R_SUCCEEDED(rc)) {
        shmemLoadRemote(&g_hidSharedmem, sharedmem_handle, 0x40000, PERM_R);

        rc = shmemMap(&g_hidSharedmem);

        if (R_FAILED(rc)) svcCloseHandle(sharedmem_handle);
    }

    if (R_FAILED(rc)) {
        if (g_hidServiceSession != INVALID_HANDLE) svcCloseHandle(g_hidServiceSession);
        if (g_hidIAppletResource != INVALID_HANDLE) svcCloseHandle(g_hidIAppletResource);

        g_hidServiceSession = INVALID_HANDLE;
        g_hidIAppletResource = INVALID_HANDLE;
    }

    hidReset();

    return rc;
}

void hidExit(void)
{
    if (g_hidServiceSession == INVALID_HANDLE) return;

    if (g_hidServiceSession != INVALID_HANDLE) {
        svcCloseHandle(g_hidServiceSession);
        g_hidServiceSession = INVALID_HANDLE;
    }

    if (g_hidIAppletResource != INVALID_HANDLE) {
        svcCloseHandle(g_hidIAppletResource);
        g_hidIAppletResource = INVALID_HANDLE;
    }

    shmemClose(&g_hidSharedmem);
}

void hidReset(void)
{
    // Reset internal state
    memset(&touchEntry, 0, sizeof(HIDTouchScreenEntry));
    memset(&mouseEntry, 0, sizeof(HIDMouseEntry));
    memset(&keyboardEntry, 0, sizeof(HIDKeyboardEntry));
    memset(&controllerEntries, 0, sizeof(HIDControllerInputEntry));

    mouseOld = mouseHeld = mouseDown = mouseUp = 0;
    keyboardModOld = keyboardModHeld = keyboardModDown = keyboardModUp = 0;
    for (int i = 0; i < 8; i++)
        keyboardOld[i] = keyboardHeld[i] = keyboardDown[i] = keyboardUp[i] = 0;
    for (int i = 0; i < 10; i++)
        controllerOld[i] = controllerHeld[i] = controllerDown[i] = controllerUp[i] = 0;

    for (int i = 0; i < 10; i++)
        controllerLayout[i] = LAYOUT_DEFAULT;

    touchTimestamp = mouseTimestamp = keyboardTimestamp = 0;
    for (int i = 0; i < 10; i++)
        controllerTimestamps[i] = 0;
}

Handle hidGetSessionService(void) {
    return g_hidServiceSession;
}

void* hidGetSharedmemAddr(void) {
    return shmemGetAddr(&g_hidSharedmem);
}

void hidSetControllerLayout(HIDControllerID id, HIDControllerLayoutType layoutType)
{
    controllerLayout[id] = layoutType;
}

void hidScanInput(void)
{
    if (g_hidServiceSession == INVALID_HANDLE) return;
    HIDSharedMemory *sharedMem = (HIDSharedMemory*)hidGetSharedmemAddr();

    mouseOld = mouseHeld;
    keyboardModOld = keyboardModHeld;
    memcpy(keyboardOld, keyboardHeld, sizeof(keyboardOld));
    memcpy(controllerOld, controllerHeld, sizeof(controllerOld));

    mouseHeld = 0;
    keyboardModHeld = 0;
    memset(keyboardHeld, 0, sizeof(keyboardHeld));
    memset(controllerHeld, 0, sizeof(controllerHeld));
    memset(&touchEntry, 0, sizeof(HIDTouchScreenEntry));
    memset(&mouseEntry, 0, sizeof(HIDMouseEntry));
    memset(&keyboardEntry, 0, sizeof(HIDKeyboardEntry));
    for (int i = 0; i < 10; i++)
        memset(&controllerEntries[i], 0, sizeof(HIDControllerInputEntry));

    u64 latestTouchEntry = sharedMem->touchscreen.header.latestEntry;
    HIDTouchScreenEntry *newTouchEntry = &sharedMem->touchscreen.entries[latestTouchEntry];
    if (newTouchEntry->header.timestamp - touchTimestamp) {
        memcpy(&touchEntry, newTouchEntry, sizeof(HIDTouchScreenEntry));
        touchTimestamp = newTouchEntry->header.timestamp;
    }

    u64 latestMouseEntry = sharedMem->mouse.header.latestEntry;
    HIDMouseEntry *newMouseEntry = &sharedMem->mouse.entries[latestMouseEntry];
    if (newMouseEntry->timestamp - mouseTimestamp) {
        memcpy(&mouseEntry, newMouseEntry, sizeof(HIDMouseEntry));
        mouseTimestamp = newMouseEntry->timestamp;

        mouseHeld = mouseEntry.buttons;
    }
    mouseDown = (~mouseOld) & mouseHeld;
    mouseUp = mouseOld & (~mouseHeld);

    u64 latestKeyboardEntry = sharedMem->keyboard.header.latestEntry;
    HIDKeyboardEntry *newKeyboardEntry = &sharedMem->keyboard.entries[latestKeyboardEntry];
    if (newKeyboardEntry->timestamp - keyboardTimestamp) {
        memcpy(&keyboardEntry, newKeyboardEntry, sizeof(HIDKeyboardEntry));
        keyboardTimestamp = newKeyboardEntry->timestamp;

        keyboardModHeld = keyboardEntry.modifier;
        for (int i = 0; i < 8; i++) {
            keyboardHeld[i] = keyboardEntry.keys[i];
        }
    }
    keyboardModDown = (~keyboardModOld) & keyboardModHeld;
    keyboardModUp = keyboardModOld & (~keyboardModHeld);
    for (int i = 0; i < 8; i++) {
        keyboardDown[i] = (~keyboardOld[i]) & keyboardHeld[i];
        keyboardUp[i] = keyboardOld[i] & (~keyboardHeld[i]);
    }

    for (int i = 0; i < 10; i++) {
        HIDControllerLayout *currentLayout = &sharedMem->controllers[i].layouts[controllerLayout[i]];
        u64 latestControllerEntry = currentLayout->header.latestEntry;
        HIDControllerInputEntry *newInputEntry = &currentLayout->entries[latestControllerEntry];
        if (newInputEntry->timestamp - controllerTimestamps[i]) {
            memcpy(&controllerEntries[i], newInputEntry, sizeof(HIDControllerInputEntry));
            controllerTimestamps[i] = newInputEntry->timestamp;

            controllerHeld[i] = controllerEntries[i].buttons;
        }

        controllerDown[i] = (~controllerOld[i]) & controllerHeld[i];
        controllerUp[i] = controllerOld[i] & (~controllerHeld[i]);
    }
}

u64 hidKeysHeld(HIDControllerID id)
{
    if (id < 0 || id > 9) return 0;

    return controllerHeld[id];
}

u64 hidKeysDown(HIDControllerID id)
{
    if (id < 0 || id > 9) return 0;

    return controllerDown[id];
}

u64 hidKeysUp(HIDControllerID id)
{
    if (id < 0 || id > 9) return 0;

    return controllerUp[id];
}

u64 hidMouseButtonsHeld(void)
{
    return mouseHeld;
}

u64 hidMouseButtonsDown(void)
{
    return mouseDown;
}

u64 hidMouseButtonsUp(void)
{
    return mouseUp;
}

bool hidKeyboardModifierHeld(HIDKeyboardModifier modifier)
{
    return keyboardModHeld & modifier;
}

bool hidKeyboardModifierDown(HIDKeyboardModifier modifier)
{
    return keyboardModDown & modifier;
}

bool hidKeyboardModifierUp(HIDKeyboardModifier modifier)
{
    return keyboardModUp & modifier;
}

bool hidKeyboardHeld(HIDKeyboardScancode key)
{
    return keyboardHeld[key / 32] & (1 << (key % 32));
}

bool hidKeyboardDown(HIDKeyboardScancode key)
{
    return keyboardDown[key / 32] & (1 << (key % 32));
}

bool hidKeyboardUp(HIDKeyboardScancode key)
{
    return keyboardUp[key / 32] & (1 << (key % 32));
}

static Result _hidCreateAppletResource(Handle sessionhandle, Handle* handle_out, u64 AppletResourceUserId) {
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

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

static Result _hidGetSharedMemoryHandle(Handle sessionhandle, Handle* handle_out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = ipcDispatch(sessionhandle);

    if (R_SUCCEEDED(rc)) {
        IpcCommandResponse r;
        ipcParseResponse(&r);

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

