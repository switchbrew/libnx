/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108
 * @copyright libnx Authors
 */
#include "services/nifm.h"

static Service g_nifmSrv;
static IGeneralService g_nifmIGS;

Result nifmInitialize(){
	if (serviceIsActive(&g_nifmSrv))
        return 0;	
	Result rc;
	
	rc = smGetService(&g_nifmSrv, "nifm:u");
	
	if(R_SUCCEEDED(rc)){
		if(kernelAbove200())
			rc = _CreateGeneralService(&g_nifmIGS, 0); // What does this parameter do?
		else
			rc = _CreateGeneralServiceOld(&g_nifmIGS);
	}
	
	if(R_FAILED(rc))
		nifmExit();
		
	return rc;	
}

void nifmExit(void){
	if(serviceIsActive(&g_nifmIGS.s))
		serviceClose(&g_nifmIGS.s);
	if(serviceIsActive(&g_nifmSrv))
		serviceClose(&g_nifmSrv);
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

        if (R_SUCCEEDED(rc)) 
            serviceCreate(&out->s, r.Handles[0]);      
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

Result GetCurrentIpAddress(u32* out){
	IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;

    Result rc = serviceIpcDispatch(&g_nifmIGS.s);

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