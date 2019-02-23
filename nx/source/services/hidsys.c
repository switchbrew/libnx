#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/shmem.h"
#include "kernel/rwlock.h"
#include "kernel/event.h"
#include "services/applet.h"
#include "services/hidsys.h"
#include "services/sm.h"

static Service g_hidSysSrv;
static u64 g_hidSysRefCnt;

static Event g_hidSysHomeEvent = {0};
static Event g_hidSysCaptureEvent = {0};
static Event g_hidSysSleeptEvent = {0};

static u64 g_hidSysAppletResourceUserId = 0;

Result hidSysInitialize(void) {
	atomicIncrement64(&g_hidSysRefCnt);
	
    if (serviceIsActive(&g_hidSysSrv)) 
		return 0;
	
    Result rc = smGetService(&g_hidSysSrv, "hid:sys");
	if (R_FAILED(rc))
		return rc;
	
	rc = appletGetAppletResourceUserId(&g_hidSysAppletResourceUserId);
    if (R_FAILED(rc))
        g_hidSysAppletResourceUserId = 0;

    return 0; 
}

void hidSysExit(void) {
    if (atomicDecrement64(&g_hidSysRefCnt) == 0) {
		eventClose(&g_hidSysHomeEvent);
		eventClose(&g_hidSysCaptureEvent);
		eventClose(&g_hidSysSleeptEvent);
		
        serviceClose(&g_hidSysSrv);
    }
}

Result hidSysEnableAppletToGetInput(bool enable) {	
    IpcCommand c;
    ipcInitialize(&c);

    struct CmdStruct{
        u64 magic;
        u64 cmdid;
        bool permitInput;
        u64 appletResourceUserId;
    } *cmdStruct;

    cmdStruct = (struct CmdStruct*) serviceIpcPrepareHeader(&g_hidSysSrv, &c, sizeof(*cmdStruct));

    cmdStruct->magic = SFCI_MAGIC;
    cmdStruct->cmdid = 503;
    cmdStruct->permitInput = enable;
	cmdStruct->appletResourceUserId = g_hidSysAppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_hidSysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidSysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidSysCmdWithResIdAndPid(u64 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 AppletResourceUserId;
    } *raw;

	ipcSendPid(&c);
    raw = serviceIpcPrepareHeader(&g_hidSysSrv,&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
	raw->AppletResourceUserId = g_hidSysAppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_hidSysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidSysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _hidSysGetHandle(Handle* handle_out, u64 cmd_id) {	
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
		u64 AppletResourceUserId;
    } *raw;

	ipcSendPid(&c);
    raw = serviceIpcPrepareHeader(&g_hidSysSrv,&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
	raw->AppletResourceUserId = g_hidSysAppletResourceUserId;

    Result rc = serviceIpcDispatch(&g_hidSysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_hidSysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
        }
    }

    return rc;
}

static Result _hidSysGetEvent(Event* event_out, u64 cmd_id, bool autoclear) {
    Handle tmp_handle=0;
    Result rc = 0;

    rc = _hidSysGetHandle(&tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(event_out, tmp_handle, autoclear);
    return rc;
}

Result hidSysAcquireHomeButtonEventHandle(Event* event_out) {
    if (eventActive(&g_hidSysHomeEvent))
	{
		*event_out = g_hidSysHomeEvent;
		return 0;
	}
	Result rc = _hidSysGetEvent(&g_hidSysHomeEvent, 101, false);
	if (R_SUCCEEDED(rc))
		*event_out = g_hidSysHomeEvent;
	return rc;
}

//These functions don't seem to work in the overlaydisp applet context
Result hidSysAcquireCaptureButtonEventHandle(Event* event_out) {
	if (eventActive(&g_hidSysCaptureEvent))
	{
		*event_out = g_hidSysCaptureEvent;
		return 0;
	}
	Result rc = _hidSysGetEvent(&g_hidSysCaptureEvent, 141, false);
	if (R_SUCCEEDED(rc))
		*event_out = g_hidSysCaptureEvent;
	return rc;
}

Result hidSysAcquireSleepButtonEventHandle(Event* event_out) {
	if (eventActive(&g_hidSysSleeptEvent))
	{
		*event_out = g_hidSysSleeptEvent;
		return 0;
	}
	Result rc = _hidSysGetEvent(&g_hidSysSleeptEvent, 121, false);
	if (R_SUCCEEDED(rc))
		*event_out = g_hidSysSleeptEvent;
	return rc;
}

Result hidSysActivateHomeButton(void) {
	return _hidSysCmdWithResIdAndPid(111);
}

Result hidSysActivateSleepButton(void) {
	return _hidSysCmdWithResIdAndPid(131);
}

Result hidSysActivateCaptureButton(void) {
	return _hidSysCmdWithResIdAndPid(151);
}