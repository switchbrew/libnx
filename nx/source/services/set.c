/**
 * @file set.h
 * @brief Settings services IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/set.h"
#include "services/sm.h"
#include "services/applet.h"

static Service g_setSrv;
static Service g_setsysSrv;
static u64 g_refCnt;
static u64 g_refCntSys;

static bool g_setLanguageCodesInitialized;
static u64 g_setLanguageCodes[0x40];
static s32 g_setLanguageCodesTotal;

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode);

Result setInitialize(void)
{
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_setSrv))
        return 0;

    g_setLanguageCodesInitialized = 0;

    return smGetService(&g_setSrv, "set");
}

void setExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_setSrv);
    }
}

Result setsysInitialize(void)
{
    atomicIncrement64(&g_refCntSys);

    if (serviceIsActive(&g_setsysSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    return smGetService(&g_setsysSrv, "set:sys");
}

void setsysExit(void)
{
    if (atomicDecrement64(&g_refCntSys) == 0) {
        serviceClose(&g_setsysSrv);
    }
}

static Result setInitializeLanguageCodesCache(void) {
    if (g_setLanguageCodesInitialized) return 0;
    Result rc = 0;

    rc = setGetAvailableLanguageCodes(&g_setLanguageCodesTotal, g_setLanguageCodes, sizeof(g_setLanguageCodes)/sizeof(u64));
    if (R_FAILED(rc)) return rc;

    if (g_setLanguageCodesTotal < 0) g_setLanguageCodesTotal = 0;

    g_setLanguageCodesInitialized = 1;

    return rc;
}

Result setMakeLanguage(u64 LanguageCode, s32 *Language) {
    Result rc = setInitializeLanguageCodesCache();
    if (R_FAILED(rc)) return rc;

    s32 i;
    rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    for (i=0; i<g_setLanguageCodesTotal; i++) {
        if (g_setLanguageCodes[i] == LanguageCode) {
            *Language = i;
            return 0;
        }
    }

    return rc;
}

Result setMakeLanguageCode(s32 Language, u64 *LanguageCode) {
    Result rc = setInitializeLanguageCodesCache();
    if (R_FAILED(rc)) return rc;

    if (Language < 0)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (Language >= g_setLanguageCodesTotal) {
        if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        return _setMakeLanguageCode(Language, LanguageCode);
    }

    *LanguageCode = g_setLanguageCodes[Language];

    return rc;
}

Result setGetSystemLanguage(u64 *LanguageCode) {
    //This is disabled because the returned LanguageCode can differ from the system language, for example ja instead of {English}.
    /*Result rc = appletGetDesiredLanguage(LanguageCode);
    if (R_SUCCEEDED(rc)) return rc;*/

    return setGetLanguageCode(LanguageCode);
}

Result setGetLanguageCode(u64 *LanguageCode) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;

    Result rc = serviceIpcDispatch(&g_setSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 LanguageCode;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && LanguageCode) *LanguageCode = resp->LanguageCode;
    }

    return rc;
}

Result setGetAvailableLanguageCodes(s32 *total_entries, u64 *LanguageCodes, size_t max_entries) {
    IpcCommand c;
    ipcInitialize(&c);

    Result rc=0;
    bool new_cmd = hosversionAtLeast(4,0,0);

    if (!new_cmd) {//On system-version <4.0.0 the sysmodule will close the session if max_entries is too large.
        s32 tmptotal = 0;

        rc = setGetAvailableLanguageCodeCount(&tmptotal);
        if (R_FAILED(rc)) return rc;

        if (max_entries > (size_t)tmptotal) max_entries = (size_t)tmptotal;
    }

    size_t bufsize = max_entries*sizeof(u64);

    if (!new_cmd) {
        ipcAddRecvStatic(&c, LanguageCodes, bufsize, 0);
    }
    else {
        ipcAddRecvBuffer(&c, LanguageCodes, bufsize, 0);
    }

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = new_cmd ? 5 : 1;

    rc = serviceIpcDispatch(&g_setSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            s32 total_entries;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total_entries) *total_entries = resp->total_entries;
    }

    return rc;
}

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 Language;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->Language = Language;

    Result rc = serviceIpcDispatch(&g_setSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 LanguageCode;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && LanguageCode) *LanguageCode = resp->LanguageCode;
    }

    return rc;
}

Result setGetAvailableLanguageCodeCount(s32 *total) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = hosversionAtLeast(4,0,0) ? 6 : 3;

    Result rc = serviceIpcDispatch(&g_setSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            s32 total;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && total) {
            *total = resp->total;
            if (*total < 0) *total = 0;
        }
    }

    return rc;
}

Result setGetRegionCode(SetRegion *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&g_setSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            s32 RegionCode;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && out) *out = resp->RegionCode;
    }

    return rc;
}

Result setsysGetColorSetId(ColorSetId *out)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 color_set;
        } *resp = r.Raw;

        *out = resp->color_set;
        rc = resp->result;
    }

    return rc;
}

Result setsysSetColorSetId(ColorSetId id)
{
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        s32 id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;
    raw->id = id;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

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

Result setsysGetSettingsItemValue(const char *name, const char *item_key, void *value_out, size_t value_out_size) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];
    
    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_name, SET_MAX_NAME_SIZE, 0);
    ipcAddSendStatic(&c, send_item_key, SET_MAX_NAME_SIZE, 0);
    ipcAddRecvBuffer(&c, value_out, value_out_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 38;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

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

Result setsysGetSettingsItemValueSize(const char *name, const char *item_key, u64 *size_out) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];
    
    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);
    
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_name, SET_MAX_NAME_SIZE, 0);
    ipcAddSendStatic(&c, send_item_key, SET_MAX_NAME_SIZE, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 37;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && size_out) *size_out = resp->size;
    }

    return rc;
}

Result setsysGetSerialNumber(char *serial) {
    IpcCommand c;
    ipcInitialize(&c);

    if (serial) memset(serial, 0, 0x19);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 68;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            char serial[0x18];
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc) && serial)
        	memcpy(serial, resp->serial, 0x18);
    }

    return rc;
}

Result setsysGetFlag(SetSysFlag flag, bool *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = flag;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 flag;
        } *resp = r.Raw;

        *out = resp->flag;
        rc = resp->result;
    }

    return rc;
}

Result setsysSetFlag(SetSysFlag flag, bool enable) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 flag;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = flag + 1;
    raw->flag = enable;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 flag;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _setsysGetFirmwareVersionImpl(SetSysFirmwareVersion *out, u32 cmd_id) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, out, sizeof(*out), 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_setsysSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;
        
        serviceIpcParse(&g_setsysSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;

}

Result setsysGetFirmwareVersion(SetSysFirmwareVersion *out) {
    /* GetFirmwareVersion2 does exactly what GetFirmwareVersion does, except it doesn't zero the revision field. */
    if (hosversionAtLeast(3,0,0)) {
        return _setsysGetFirmwareVersionImpl(out, 4);
    } else {
        return _setsysGetFirmwareVersionImpl(out, 3);
    }
}

Result setsysBindFatalDirtyFlagEvent(Event *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 93;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(out, r.Handles[0], false);
        }
    }

    return rc;
}

Result setsysGetFatalDirtyFlags(u64 *flags_0, u64 *flags_1) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 94;

    Result rc = serviceIpcDispatch(&g_setsysSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 flags_0;
            u64 flags_1;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *flags_0 = resp->flags_0;
            *flags_1 = resp->flags_1;
        }
    }

    return rc;
}
