/**
 * @file set.h
 * @brief Settings services IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#include "types.h"
#include "result.h"
#include "ipc.h"
#include "kernel/detect.h"
#include "services/set.h"
#include "services/sm.h"
#include "services/applet.h"

static Service g_setSrv;
static Service g_setsysSrv;

static bool g_setLanguageCodesInitialized;
static u64 g_setLanguageCodes[0x40];
static s32 g_setLanguageCodesTotal;

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode);

Result setInitialize(void)
{
    if (serviceIsActive(&g_setSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    g_setLanguageCodesInitialized = 0;

    return smGetService(&g_setSrv, "set");
}

void setExit(void)
{
    serviceClose(&g_setSrv);
}

Result setsysInitialize(void)
{
    if (serviceIsActive(&g_setsysSrv))
        return MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized);

    return smGetService(&g_setsysSrv, "set:sys");
}

void setsysExit(void)
{
    serviceClose(&g_setsysSrv);
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
        if (!kernelAbove400()) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
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
    bool new_cmd = kernelAbove400();

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
    raw->cmd_id = kernelAbove400() ? 6 : 3;

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

Result setGetRegionCode(s32 *RegionCode) {
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

        if (R_SUCCEEDED(rc) && RegionCode) *RegionCode = resp->RegionCode;
    }

    return rc;
}

Result setsysGetColorSetId(ColorSetId* out)
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
