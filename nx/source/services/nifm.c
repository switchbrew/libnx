/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108
 * @copyright libnx Authors
 */
#include "services/nifm.h"

static Service g_nifmSrv;

Result nifmInitialize(){
	if (serviceIsActive(&g_nifmSrv))
        return 0;	
	return smGetService(&g_nifmSrv, "nifm:u");
}

void nifmExit(void){
	serviceClose(&g_nifmSrv);
}

Result CreateGeneralService(IGeneralService *out){
	  if(kernelAbove200())
		return _CreateGeneralService(out, 0); // What does this parameter do?
	else
		return _CreateGeneralServiceOld(out);
}

Result _CreateGeneralService(IGeneralService* out, u64 in){
	IpcCommand c;
      ipcInitialize(&c);
	  ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 param;
    } PACKED *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->param = in;

    Result rc = serviceIpcDispatch(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }
	
	return rc;
}

Result _CreateGeneralServiceOld(IGeneralService* out){
	  IpcCommand c;
      ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } PACKED *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }
	
	return rc;
}

Result GetCurrentIpAddress(IGeneralService* srv, u32* out){
	IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&srv->s);

    if (R_SUCCEEDED(rc)) 
    {
        IpcParsedCommand r;
        ipcParse(&r);

        struct 
        {
            u64 magic;
            u64 result;
            u32 out;
        } *resp = r.Raw;

        rc = resp->result;
        *out = resp->out;
    }

    return rc;

}