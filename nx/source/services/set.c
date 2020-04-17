#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/set.h"
#include "services/applet.h"

static Service g_setSrv;
static Service g_setsysSrv;
static Service g_setcalSrv;

static bool g_setLanguageCodesInitialized;
static u64 g_setLanguageCodes[0x40];
static s32 g_setLanguageCodesTotal;

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode);

NX_GENERATE_SERVICE_GUARD(set);

Result _setInitialize(void) {
    g_setLanguageCodesInitialized = 0;

    return smGetService(&g_setSrv, "set");
}

void _setCleanup(void) {
    serviceClose(&g_setSrv);
}

Service* setGetServiceSession(void) {
    return &g_setSrv;
}

NX_GENERATE_SERVICE_GUARD(setsys);

Result _setsysInitialize(void) {
    return smGetService(&g_setsysSrv, "set:sys");
}

void _setsysCleanup(void) {
    serviceClose(&g_setsysSrv);
}

Service* setsysGetServiceSession(void) {
    return &g_setsysSrv;
}

NX_GENERATE_SERVICE_GUARD(setcal);

Result _setcalInitialize(void) {
    return smGetService(&g_setcalSrv, "set:cal");
}

void _setcalCleanup(void) {
    serviceClose(&g_setcalSrv);
}

Service* setcalGetServiceSession(void) {
    return &g_setcalSrv;
}

static Result _setCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _setCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _setCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _setCmdNoInOut64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _setCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _setCmdNoInOutUuid(Service* srv, Uuid *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdInU8NoOut(Service* srv, u8 inval, u64 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _setCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    return _setCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _setCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _setCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _setCmdInUuidNoOut(Service* srv, const Uuid *inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, *inval);
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

Result setMakeLanguage(u64 LanguageCode, SetLanguage *Language) {
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

Result setMakeLanguageCode(SetLanguage Language, u64 *LanguageCode) {
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
    return _setCmdNoInOut64(&g_setSrv, LanguageCode, 0);
}

Result setGetAvailableLanguageCodes(s32 *total_entries, u64 *LanguageCodes, size_t max_entries) {
    Result rc=0;
    bool new_cmd = hosversionAtLeast(4,0,0);

    if (!new_cmd) {//On system-version <4.0.0 the sysmodule will close the session if max_entries is too large.
        s32 tmptotal = 0;

        rc = setGetAvailableLanguageCodeCount(&tmptotal);
        if (R_FAILED(rc)) return rc;

        if (max_entries > (size_t)tmptotal) max_entries = (size_t)tmptotal;
    }

    return serviceDispatchOut(&g_setSrv, new_cmd ? 5 : 1, *total_entries,
        .buffer_attrs = { (new_cmd ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcPointer) | SfBufferAttr_Out },
        .buffers = { { LanguageCodes, max_entries*sizeof(u64) } },
    );
}

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode) {
    return serviceDispatchInOut(&g_setSrv, 2, Language, *LanguageCode);
}

Result setGetAvailableLanguageCodeCount(s32 *total) {
    Result rc = _setCmdNoInOutU32(&g_setSrv, (u32*)total, hosversionAtLeast(4,0,0) ? 6 : 3);
    if (R_SUCCEEDED(rc) && total && *total < 0) *total = 0;
    return rc;
}

Result setGetRegionCode(SetRegion *out) {
    s32 code=0;
    Result rc = _setCmdNoInOutU32(&g_setSrv, (u32*)&code, 4);
    if (R_SUCCEEDED(rc) && out) *out = code;
    return rc;
}

Result setsysSetLanguageCode(u64 LanguageCode) {
    return _setCmdInU64NoOut(&g_setsysSrv, LanguageCode, 0);
}

Result setsysSetNetworkSettings(const SetSysNetworkSettings *settings, s32 count) {
    return serviceDispatch(&g_setsysSrv, 1,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysNetworkSettings) } },
    );
}

Result setsysGetNetworkSettings(s32 *total_out, SetSysNetworkSettings *settings, s32 count) {
    return serviceDispatchOut(&g_setsysSrv, 2, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysNetworkSettings) } },
    );
}

static Result _setsysGetFirmwareVersionImpl(SetSysFirmwareVersion *out, u32 cmd_id) {
    return serviceDispatch(&g_setsysSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result setsysGetFirmwareVersion(SetSysFirmwareVersion *out) {
    /* GetFirmwareVersion2 does exactly what GetFirmwareVersion does, except it doesn't zero the revision field. */
    if (hosversionAtLeast(3,0,0)) {
        return _setsysGetFirmwareVersionImpl(out, 4);
    } else {
        return _setsysGetFirmwareVersionImpl(out, 3);
    }
}

Result setsysGetFirmwareVersionDigest(SetSysFirmwareVersionDigest *out) {
    return serviceDispatchOut(&g_setsysSrv, 5, *out);
}

Result setsysGetLockScreenFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 7);
}

Result setsysSetLockScreenFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 8);
}

Result setsysGetBacklightSettings(SetSysBacklightSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 9, *out);
}

Result setsysSetBacklightSettings(const SetSysBacklightSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 10, *settings);
}

Result setsysSetBluetoothDevicesSettings(const SetSysBluetoothDevicesSettings *settings, s32 count) {
    return serviceDispatch(&g_setsysSrv, 11,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysBluetoothDevicesSettings) } },
    );
}

Result setsysGetBluetoothDevicesSettings(s32 *total_out, SetSysBluetoothDevicesSettings *settings, s32 count) {
    return serviceDispatchOut(&g_setsysSrv, 12, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysBluetoothDevicesSettings) } },
    );
}

Result setsysGetExternalSteadyClockSourceId(Uuid *out) {
    return _setCmdNoInOutUuid(&g_setsysSrv, out, 13);
}

Result setsysSetExternalSteadyClockSourceId(const Uuid *uuid) {
    return _setCmdInUuidNoOut(&g_setsysSrv, uuid, 14);
}

Result setsysGetUserSystemClockContext(TimeSystemClockContext *out) {
    return serviceDispatchOut(&g_setsysSrv, 15, *out);
}

Result setsysSetUserSystemClockContext(const TimeSystemClockContext *context) {
    return serviceDispatchIn(&g_setsysSrv, 16, *context);
}

Result setsysGetAccountSettings(SetSysAccountSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 17, *out);
}

Result setsysSetAccountSettings(SetSysAccountSettings settings) {
    return serviceDispatchIn(&g_setsysSrv, 18, settings);
}

Result setsysGetAudioVolume(SetSysAudioDevice device, SetSysAudioVolume *out) {
    return serviceDispatchInOut(&g_setsysSrv, 19, device, *out);
}

Result setsysSetAudioVolume(SetSysAudioDevice device, const SetSysAudioVolume *volume) {
    const struct {
        SetSysAudioVolume volume;
        u32 device;
    } in = { *volume, device };

    return serviceDispatchIn(&g_setsysSrv, 20, in);
}

Result setsysGetEulaVersions(s32 *total_out, SetSysEulaVersion *versions, s32 count) {
    return serviceDispatchOut(&g_setsysSrv, 21, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { versions, count*sizeof(SetSysEulaVersion) } },
    );
}

Result setsysSetEulaVersions(const SetSysEulaVersion *versions, s32 count) {
    return serviceDispatch(&g_setsysSrv, 22,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { versions, count*sizeof(SetSysEulaVersion) } },
    );
}

Result setsysGetColorSetId(ColorSetId *out) {
    u32 color_set=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &color_set, 23);
    if (R_SUCCEEDED(rc) && out) *out = color_set;
    return rc;
}

Result setsysSetColorSetId(ColorSetId id) {
    return _setCmdInU32NoOut(&g_setsysSrv, id, 24);
}

Result setsysGetConsoleInformationUploadFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 25);
}

Result setsysSetConsoleInformationUploadFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 26);
}

Result setsysGetAutomaticApplicationDownloadFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 27);
}

Result setsysSetAutomaticApplicationDownloadFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 28);
}

Result setsysGetNotificationSettings(SetSysNotificationSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 29, *out);
}

Result setsysSetNotificationSettings(const SetSysNotificationSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 30, *settings);
}

Result setsysGetAccountNotificationSettings(s32 *total_out, SetSysAccountNotificationSettings *settings, s32 count) {
    return serviceDispatchOut(&g_setsysSrv, 31, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysAccountNotificationSettings) } },
    );
}

Result setsysSetAccountNotificationSettings(const SetSysAccountNotificationSettings *settings, s32 count) {
    return serviceDispatch(&g_setsysSrv, 32,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysAccountNotificationSettings) } },
    );
}

Result setsysGetVibrationMasterVolume(float *out) {
    return serviceDispatchOut(&g_setsysSrv, 35, *out);
}

Result setsysSetVibrationMasterVolume(float volume) {
    return serviceDispatchIn(&g_setsysSrv, 36, volume);
}

Result setsysGetSettingsItemValueSize(const char *name, const char *item_key, u64 *size_out) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];

    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);

    return serviceDispatchOut(&g_setsysSrv, 37, *size_out,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { send_name, SET_MAX_NAME_SIZE },
            { send_item_key, SET_MAX_NAME_SIZE },
        },
    );
}

Result setsysGetSettingsItemValue(const char *name, const char *item_key, void *value_out, size_t value_out_size, u64 *size_out) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];

    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);

    return serviceDispatchOut(&g_setsysSrv, 38, *size_out,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { send_name, SET_MAX_NAME_SIZE },
            { send_item_key, SET_MAX_NAME_SIZE },
            { value_out, value_out_size },
        },
    );
}

Result setsysGetTvSettings(SetSysTvSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 39, *out);
}

Result setsysSetTvSettings(const SetSysTvSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 40, *settings);
}

Result setsysGetEdid(SetSysEdid *out) {
    return serviceDispatch(&g_setsysSrv, 41,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result setsysSetEdid(const SetSysEdid *edid) {
    return serviceDispatch(&g_setsysSrv, 42,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { edid, sizeof(*edid) } },
    );
}

Result setsysGetAudioOutputMode(SetSysAudioOutputModeTarget target, SetSysAudioOutputMode *out) {
    u32 tmp=0;
    Result rc = serviceDispatchInOut(&g_setsysSrv, 43, target, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetAudioOutputMode(SetSysAudioOutputModeTarget target, SetSysAudioOutputMode mode) {
    const struct {
        u32 target;
        u32 mode;
    } in = { target, mode };

    return serviceDispatchIn(&g_setsysSrv, 44, in);
}

Result setsysIsForceMuteOnHeadphoneRemoved(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 45);
}

Result setsysSetForceMuteOnHeadphoneRemoved(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 46);
}

Result setsysGetQuestFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 47);
}

Result setsysSetQuestFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 48);
}

Result setsysGetDataDeletionSettings(SetSysDataDeletionSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 49, *out);
}

Result setsysSetDataDeletionSettings(const SetSysDataDeletionSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 50, *settings);
}

Result setsysGetInitialSystemAppletProgramId(u64 *out) {
    return _setCmdNoInOut64(&g_setsysSrv, out, 51);
}

Result setsysGetOverlayDispProgramId(u64 *out) {
    return _setCmdNoInOut64(&g_setsysSrv, out, 52);
}

Result setsysGetDeviceTimeZoneLocationName(TimeLocationName *out) {
    return serviceDispatchOut(&g_setsysSrv, 53, *out);
}

Result setsysSetDeviceTimeZoneLocationName(const TimeLocationName *name) {
    return serviceDispatchIn(&g_setsysSrv, 54, *name);
}

Result setsysGetWirelessCertificationFileSize(u64 *out_size) {
    return _setCmdNoInOut64(&g_setsysSrv, out_size, 55);
}

Result setsysGetWirelessCertificationFile(void* buffer, size_t size, u64 *out_size) {
    return serviceDispatchOut(&g_setsysSrv, 56, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result setsysSetRegionCode(SetRegion region) {
    return _setCmdInU32NoOut(&g_setsysSrv, region, 57);
}

Result setsysGetNetworkSystemClockContext(TimeSystemClockContext *out) {
    return serviceDispatchOut(&g_setsysSrv, 58, *out);
}

Result setsysSetNetworkSystemClockContext(const TimeSystemClockContext *context) {
    return serviceDispatchIn(&g_setsysSrv, 59, *context);
}

Result setsysIsUserSystemClockAutomaticCorrectionEnabled(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 60);
}

Result setsysSetUserSystemClockAutomaticCorrectionEnabled(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 61);
}

Result setsysGetDebugModeFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 62);
}

Result setsysGetPrimaryAlbumStorage(SetSysPrimaryAlbumStorage *out) {
    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 63);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetPrimaryAlbumStorage(SetSysPrimaryAlbumStorage storage) {
    return _setCmdInU32NoOut(&g_setsysSrv, storage, 64);
}

Result setsysGetUsb30EnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 65);
}

Result setsysSetUsb30EnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 66);
}

Result setsysGetBatteryLot(SetBatteryLot *out) {
    return serviceDispatchOut(&g_setsysSrv, 67, *out);
}

Result setsysGetSerialNumber(SetSysSerialNumber *out) {
    return serviceDispatchOut(&g_setsysSrv, 68, *out);
}

Result setsysGetNfcEnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 69);
}

Result setsysSetNfcEnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 70);
}

Result setsysGetSleepSettings(SetSysSleepSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 71, *out);
}

Result setsysSetSleepSettings(const SetSysSleepSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 72, *settings);
}

Result setsysGetWirelessLanEnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 73);
}

Result setsysSetWirelessLanEnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 74);
}

Result setsysGetInitialLaunchSettings(SetSysInitialLaunchSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 75, *out);
}

Result setsysSetInitialLaunchSettings(const SetSysInitialLaunchSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 76, *settings);
}

Result setsysGetDeviceNickname(char* nickname) {
    return serviceDispatch(&g_setsysSrv, 77,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { nickname, SET_MAX_NICKNAME_SIZE } },
    );
}

Result setsysSetDeviceNickname(const char* nickname) {
    char send_nickname[SET_MAX_NICKNAME_SIZE] = {0};
    strncpy(send_nickname, nickname, SET_MAX_NICKNAME_SIZE-1);

    return serviceDispatch(&g_setsysSrv, 78,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { send_nickname, SET_MAX_NICKNAME_SIZE } },
    );
}

Result setsysGetProductModel(s32 *out) {
    return _setCmdNoInOutU32(&g_setsysSrv, (u32*)out, 79);
}

Result setsysGetLdnChannel(s32 *out) {
    return _setCmdNoInOutU32(&g_setsysSrv, (u32*)out, 80);
}

Result setsysSetLdnChannel(s32 channel) {
    return _setCmdInU32NoOut(&g_setsysSrv, (u32)channel, 81);
}

Result setsysAcquireTelemetryDirtyFlagEventHandle(Event *out_event) {
    return _setCmdGetEvent(&g_setsysSrv, out_event, false, 82);
}

Result setsysGetTelemetryDirtyFlags(u64 *flags_0, u64 *flags_1) {
    struct {
        u64 flags_0;
        u64 flags_1;
    } out;

    Result rc = serviceDispatchOut(&g_setsysSrv, 83, out);
    if (R_SUCCEEDED(rc) && flags_0) *flags_0 = out.flags_0;
    if (R_SUCCEEDED(rc) && flags_1) *flags_1 = out.flags_1;
    return rc;
}

Result setsysGetPtmBatteryLot(SetBatteryLot *out) {
    return serviceDispatchOut(&g_setsysSrv, 84, *out);
}

Result setsysSetPtmBatteryLot(const SetBatteryLot *lot) {
    return serviceDispatchIn(&g_setsysSrv, 85, *lot);
}

Result setsysGetPtmFuelGaugeParameter(SetSysPtmFuelGaugeParameter *out) {
    return serviceDispatchOut(&g_setsysSrv, 86, *out);
}

Result setsysSetPtmFuelGaugeParameter(const SetSysPtmFuelGaugeParameter *parameter) {
    return serviceDispatchIn(&g_setsysSrv, 87, *parameter);
}

Result setsysGetBluetoothEnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 88);
}

Result setsysSetBluetoothEnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 89);
}

Result setsysGetMiiAuthorId(Uuid *out) {
    return _setCmdNoInOutUuid(&g_setsysSrv, out, 90);
}

Result setsysSetShutdownRtcValue(u64 value) {
    return _setCmdInU64NoOut(&g_setsysSrv, value, 91);
}

Result setsysGetShutdownRtcValue(u64 *out) {
    return _setCmdNoInOut64(&g_setsysSrv, out, 92);
}

Result setsysAcquireFatalDirtyFlagEventHandle(Event *out_event) {
    return _setCmdGetEvent(&g_setsysSrv, out_event, false, 93);
}

Result setsysGetFatalDirtyFlags(u64 *flags_0, u64 *flags_1) {
    struct {
        u64 flags_0;
        u64 flags_1;
    } out;

    Result rc = serviceDispatchOut(&g_setsysSrv, 94, out);
    if (R_SUCCEEDED(rc) && flags_0) *flags_0 = out.flags_0;
    if (R_SUCCEEDED(rc) && flags_1) *flags_1 = out.flags_1;
    return rc;
}

Result setsysGetAutoUpdateEnableFlag(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 95);
}

Result setsysSetAutoUpdateEnableFlag(bool flag) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 96);
}

Result setsysGetNxControllerSettings(s32 *total_out, SetSysNxControllerSettings *settings, s32 count) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 97, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysNxControllerSettings) } },
    );
}

Result setsysSetNxControllerSettings(const SetSysNxControllerSettings *settings, s32 count) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 98,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysNxControllerSettings) } },
    );
}

Result setsysGetBatteryPercentageFlag(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 99);
}

Result setsysSetBatteryPercentageFlag(bool flag) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 100);
}

Result setsysGetExternalRtcResetFlag(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 101);
}

Result setsysSetExternalRtcResetFlag(bool flag) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 102);
}

Result setsysGetUsbFullKeyEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 103);
}

Result setsysSetUsbFullKeyEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 104);
}

Result setsysSetExternalSteadyClockInternalOffset(u64 offset) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU64NoOut(&g_setsysSrv, offset, 105);
}

Result setsysGetExternalSteadyClockInternalOffset(u64 *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOut64(&g_setsysSrv, out, 106);
}

Result setsysGetBacklightSettingsEx(SetSysBacklightSettingsEx *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 107, *out);
}

Result setsysSetBacklightSettingsEx(const SetSysBacklightSettingsEx *settings) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 108, *settings);
}

Result setsysGetHeadphoneVolumeWarningCount(u32 *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setsysSrv, out, 109);
}

Result setsysSetHeadphoneVolumeWarningCount(u32 count) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, count, 110);
}

Result setsysGetBluetoothAfhEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 111);
}

Result setsysSetBluetoothAfhEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 112);
}

Result setsysGetBluetoothBoostEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 113);
}

Result setsysSetBluetoothBoostEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 114);
}

Result setsysGetInRepairProcessEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 115);
}

Result setsysSetInRepairProcessEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 116);
}

Result setsysGetHeadphoneVolumeUpdateFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 117);
}

Result setsysSetHeadphoneVolumeUpdateFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 118);
}

Result setsysNeedsToUpdateHeadphoneVolume(u8 *a0, u8 *a1, u8 *a2, bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        u8 a0;
        u8 a1;
        u8 a2;
    } out;

    Result rc = serviceDispatchInOut(&g_setsysSrv, 119, flag, out);
    if (R_SUCCEEDED(rc) && a0) *a0 = out.a0;
    if (R_SUCCEEDED(rc) && a1) *a1 = out.a1;
    if (R_SUCCEEDED(rc) && a2) *a2 = out.a2;
    return rc;
}

Result setsysGetPushNotificationActivityModeOnSleep(u32 *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setsysSrv, out, 120);
}

Result setsysSetPushNotificationActivityModeOnSleep(u32 mode) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, mode, 121);
}

Result setsysGetServiceDiscoveryControlSettings(SetSysServiceDiscoveryControlSettings *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 122);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetServiceDiscoveryControlSettings(SetSysServiceDiscoveryControlSettings settings) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, settings, 123);
}

Result setsysGetErrorReportSharePermission(SetSysErrorReportSharePermission *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 124);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetErrorReportSharePermission(SetSysErrorReportSharePermission permission) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, permission, 125);
}

Result setsysGetAppletLaunchFlags(u32 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setsysSrv, out, 126);
}

Result setsysSetAppletLaunchFlags(u32 flags) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, flags, 127);
}

Result setsysGetConsoleSixAxisSensorAccelerationBias(SetSysConsoleSixAxisSensorAccelerationBias *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 128, *out);
}

Result setsysSetConsoleSixAxisSensorAccelerationBias(const SetSysConsoleSixAxisSensorAccelerationBias *bias) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 129, *bias);
}

Result setsysGetConsoleSixAxisSensorAngularVelocityBias(SetSysConsoleSixAxisSensorAngularVelocityBias *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 130, *out);
}

Result setsysSetConsoleSixAxisSensorAngularVelocityBias(const SetSysConsoleSixAxisSensorAngularVelocityBias *bias) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 131, *bias);
}

Result setsysGetConsoleSixAxisSensorAccelerationGain(SetSysConsoleSixAxisSensorAccelerationGain *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 132, *out);
}

Result setsysSetConsoleSixAxisSensorAccelerationGain(const SetSysConsoleSixAxisSensorAccelerationGain *gain) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 133, *gain);
}

Result setsysGetConsoleSixAxisSensorAngularVelocityGain(SetSysConsoleSixAxisSensorAngularVelocityGain *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 134, *out);
}

Result setsysSetConsoleSixAxisSensorAngularVelocityGain(const SetSysConsoleSixAxisSensorAngularVelocityGain *gain) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 135, *gain);
}

Result setsysGetKeyboardLayout(SetKeyboardLayout *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 136);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetKeyboardLayout(SetKeyboardLayout layout) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, layout, 137);
}

Result setsysGetWebInspectorFlag(bool *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 138);
}

Result setsysGetAllowedSslHosts(s32 *total_out, SetSysAllowedSslHosts *out, s32 count) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 139, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, count*sizeof(SetSysAllowedSslHosts) } },
    );
}

Result setsysGetHostFsMountPoint(SetSysHostFsMountPoint *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 140,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetSysHostFsMountPoint) } },
    );
}

Result setsysGetRequiresRunRepairTimeReviser(bool *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 141);
}

Result setsysSetRequiresRunRepairTimeReviser(bool flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 142);
}

Result setsysSetBlePairingSettings(const SetSysBlePairingSettings *settings, s32 count) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 143,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysBlePairingSettings) } },
    );
}

Result setsysGetBlePairingSettings(s32 *total_out, SetSysBlePairingSettings *settings, s32 count) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 144, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysBlePairingSettings) } },
    );
}

Result setsysGetConsoleSixAxisSensorAngularVelocityTimeBias(SetSysConsoleSixAxisSensorAngularVelocityTimeBias *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 145, *out);
}

Result setsysSetConsoleSixAxisSensorAngularVelocityTimeBias(const SetSysConsoleSixAxisSensorAngularVelocityTimeBias *bias) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 146, *bias);
}

Result setsysGetConsoleSixAxisSensorAngularAcceleration(SetSysConsoleSixAxisSensorAngularAcceleration *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 147, *out);
}

Result setsysSetConsoleSixAxisSensorAngularAcceleration(const SetSysConsoleSixAxisSensorAngularAcceleration *acceleration) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 148, *acceleration);
}

Result setsysGetRebootlessSystemUpdateVersion(SetSysRebootlessSystemUpdateVersion *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 149, *out);
}

Result setsysGetDeviceTimeZoneLocationUpdatedTime(TimeSteadyClockTimePoint *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 150, *out);
}

Result setsysSetDeviceTimeZoneLocationUpdatedTime(const TimeSteadyClockTimePoint *time_point) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 151, *time_point);
}

Result setsysGetUserSystemClockAutomaticCorrectionUpdatedTime(TimeSteadyClockTimePoint *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 152, *out);
}

Result setsysSetUserSystemClockAutomaticCorrectionUpdatedTime(const TimeSteadyClockTimePoint *time_point) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 153, *time_point);
}

Result setsysGetAccountOnlineStorageSettings(s32 *total_out, SetSysAccountOnlineStorageSettings *settings, s32 count) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 154, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysAccountOnlineStorageSettings) } },
    );
}

Result setsysSetAccountOnlineStorageSettings(const SetSysAccountOnlineStorageSettings *settings, s32 count) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 155,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysAccountOnlineStorageSettings) } },
    );
}

Result setsysGetPctlReadyFlag(bool *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 156);
}

Result setsysSetPctlReadyFlag(bool flag) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 157);
}

Result setsysGetAnalogStickUserCalibrationL(SetSysAnalogStickUserCalibration *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 158, *out);
}

Result setsysSetAnalogStickUserCalibrationL(const SetSysAnalogStickUserCalibration *calibration) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 159, *calibration);
}

Result setsysGetAnalogStickUserCalibrationR(SetSysAnalogStickUserCalibration *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 160, *out);
}

Result setsysSetAnalogStickUserCalibrationR(const SetSysAnalogStickUserCalibration *calibration) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 161, *calibration);
}

Result setsysGetPtmBatteryVersion(u8 *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setsysSrv, out, 162);
}

Result setsysSetPtmBatteryVersion(u8 version) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU8NoOut(&g_setsysSrv, version, 163);
}

Result setsysGetUsb30HostEnableFlag(bool *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 164);
}

Result setsysSetUsb30HostEnableFlag(bool flag) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 165);
}

Result setsysGetUsb30DeviceEnableFlag(bool *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 166);
}

Result setsysSetUsb30DeviceEnableFlag(bool flag) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 167);
}

Result setsysGetThemeId(s32 type, SetSysThemeId *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_setsysSrv, 168, type, *out);
}

Result setsysSetThemeId(s32 type, const SetSysThemeId *theme_id) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        s32 type;
        SetSysThemeId theme_id;
    } in = { type, *theme_id };

    return serviceDispatchIn(&g_setsysSrv, 169, in);
}

Result setsysGetChineseTraditionalInputMethod(SetChineseTraditionalInputMethod *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 170);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetChineseTraditionalInputMethod(SetChineseTraditionalInputMethod method) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, method, 171);
}

Result setsysGetPtmCycleCountReliability(SetSysPtmCycleCountReliability *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 172);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetPtmCycleCountReliability(SetSysPtmCycleCountReliability reliability) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, reliability, 173);
}

Result setsysGetHomeMenuScheme(SetSysHomeMenuScheme *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 174, *out);
}

Result setsysGetThemeSettings(SetSysThemeSettings *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 175, *out);
}

Result setsysSetThemeSettings(const SetSysThemeSettings *settings) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 176, *settings);
}

Result setsysGetThemeKey(FsArchiveMacKey *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 177, *out);
}

Result setsysSetThemeKey(const FsArchiveMacKey *key) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_setsysSrv, 178, *key);
}

Result setsysGetZoomFlag(bool *out) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 179);
}

Result setsysSetZoomFlag(bool flag) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 180);
}

Result setsysGetT(bool *out) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 181);
}

Result setsysSetT(bool flag) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 182);
}

Result setsysGetPlatformRegion(SetSysPlatformRegion *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 183);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetPlatformRegion(SetSysPlatformRegion region) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, region, 184);
}

Result setsysGetHomeMenuSchemeModel(u32 *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setsysSrv, out, 185);
}

Result setsysGetMemoryUsageRateFlag(bool *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 186);
}

Result setsysGetTouchScreenMode(SetSysTouchScreenMode *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 187);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetTouchScreenMode(SetSysTouchScreenMode mode) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, mode, 188);
}

Result setsysGetButtonConfigSettingsFull(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 189, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysSetButtonConfigSettingsFull(const SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 190,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysGetButtonConfigSettingsEmbedded(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 191, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysSetButtonConfigSettingsEmbedded(const SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 192,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysGetButtonConfigSettingsLeft(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 193, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysSetButtonConfigSettingsLeft(const SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 194,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysGetButtonConfigSettingsRight(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 195, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysSetButtonConfigSettingsRight(const SetSysButtonConfigSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 196,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigSettings) } },
    );
}

Result setsysGetButtonConfigRegisteredSettingsEmbedded(SetSysButtonConfigRegisteredSettings *settings) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 197,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, sizeof(SetSysButtonConfigRegisteredSettings) } },
    );
}

Result setsysSetButtonConfigRegisteredSettingsEmbedded(const SetSysButtonConfigRegisteredSettings *settings) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 198,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, sizeof(SetSysButtonConfigRegisteredSettings) } },
    );
}

Result setsysGetButtonConfigRegisteredSettings(s32 *total_out, SetSysButtonConfigRegisteredSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 199, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigRegisteredSettings) } },
    );
}

Result setsysSetButtonConfigRegisteredSettings(const SetSysButtonConfigRegisteredSettings *settings, s32 count) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setsysSrv, 200,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysButtonConfigRegisteredSettings) } },
    );
}

Result setcalGetBdAddress(SetCalBdAddress *out) {
    return serviceDispatchOut(&g_setcalSrv, 0, *out);
}

Result setcalGetConfigurationId1(SetCalConfigurationId1 *out) {
    return serviceDispatchOut(&g_setcalSrv, 1, *out);
}

Result setcalGetAccelerometerOffset(SetCalAccelerometerOffset *out) {
    return serviceDispatchOut(&g_setcalSrv, 2, *out);
}

Result setcalGetAccelerometerScale(SetCalAccelerometerScale *out) {
    return serviceDispatchOut(&g_setcalSrv, 3, *out);
}

Result setcalGetGyroscopeOffset(SetCalAccelerometerOffset *out) {
    return serviceDispatchOut(&g_setcalSrv, 4, *out);
}

Result setcalGetGyroscopeScale(SetCalGyroscopeScale *out) {
    return serviceDispatchOut(&g_setcalSrv, 5, *out);
}

Result setcalGetWirelessLanMacAddress(SetCalMacAddress *out) {
    return serviceDispatchOut(&g_setcalSrv, 6, *out);
}

Result setcalGetWirelessLanCountryCodeCount(s32 *out_count) {
    return _setCmdNoInOutU32(&g_setcalSrv, (u32*)out_count, 7);
}

Result setcalGetWirelessLanCountryCodes(s32 *total_out, SetCalCountryCode *codes, s32 count) {
    return serviceDispatchOut(&g_setcalSrv, 8, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { codes, count*sizeof(SetCalCountryCode) } },
    );
}

Result setcalGetSerialNumber(SetCalSerialNumber *out) {
    return serviceDispatchOut(&g_setcalSrv, 9, *out);
}

Result setcalSetInitialSystemAppletProgramId(u64 program_id) {
    return _setCmdInU64NoOut(&g_setcalSrv, program_id, 10);
}

Result setcalSetOverlayDispProgramId(u64 program_id) {
    return _setCmdInU64NoOut(&g_setcalSrv, program_id, 11);
}

Result setcalGetBatteryLot(SetBatteryLot *out) {
    return serviceDispatchOut(&g_setcalSrv, 12, *out);
}

Result setcalGetEciDeviceCertificate(SetCalEccB233DeviceCertificate *out) {
    return serviceDispatch(&g_setcalSrv, 14,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalEccB233DeviceCertificate) } },
    );
}

Result setcalGetEticketDeviceCertificate(SetCalRsa2048DeviceCertificate *out) {
    return serviceDispatch(&g_setcalSrv, 15,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalRsa2048DeviceCertificate) } },
    );
}

Result setcalGetSslKey(SetCalSslKey *out) {
    return serviceDispatch(&g_setcalSrv, 16,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalSslKey) } },
    );
}

Result setcalGetSslCertificate(SetCalSslCertificate *out) {
    return serviceDispatch(&g_setcalSrv, 17,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalSslCertificate) } },
    );
}

Result setcalGetGameCardKey(SetCalGameCardKey *out) {
    return serviceDispatch(&g_setcalSrv, 18,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalGameCardKey) } },
    );
}

Result setcalGetGameCardCertificate(SetCalGameCardCertificate *out) {
    return serviceDispatch(&g_setcalSrv, 19,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalGameCardCertificate) } },
    );
}

Result setcalGetEciDeviceKey(SetCalEccB233DeviceKey *out) {
    return serviceDispatchOut(&g_setcalSrv, 20, *out);
}

Result setcalGetEticketDeviceKey(SetCalRsa2048DeviceKey *out) {
    return serviceDispatch(&g_setcalSrv, 21,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalRsa2048DeviceKey) } },
    );
}

Result setcalGetSpeakerParameter(SetCalSpeakerParameter *out) {
    return serviceDispatchOut(&g_setcalSrv, 22, *out);
}

Result setcalGetLcdVendorId(u32 *out_vendor_id) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setcalSrv, out_vendor_id, 23);
}

Result setcalGetEciDeviceCertificate2(SetCalRsa2048DeviceCertificate *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setcalSrv, 24,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalRsa2048DeviceCertificate) } },
    );
}

Result setcalGetEciDeviceKey2(SetCalRsa2048DeviceKey *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_setcalSrv, 25,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, sizeof(SetCalRsa2048DeviceKey) } },
    );
}

Result setcalGetAmiiboKey(SetCalAmiiboKey *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 26, *out);
}

Result setcalGetAmiiboEcqvCertificate(SetCalAmiiboEcqvCertificate *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 27, *out);
}

Result setcalGetAmiiboEcdsaCertificate(SetCalAmiiboEcdsaCertificate *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 28, *out);
}

Result setcalGetAmiiboEcqvBlsKey(SetCalAmiiboEcqvBlsKey *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 29, *out);
}

Result setcalGetAmiiboEcqvBlsCertificate(SetCalAmiiboEcqvBlsCertificate *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 30, *out);
}

Result setcalGetAmiiboEcqvBlsRootCertificate(SetCalAmiiboEcqvBlsRootCertificate *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 31, *out);
}

Result setcalGetUsbTypeCPowerSourceCircuitVersion(u8 *out_version) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setcalSrv, out_version, 32);
}

Result setcalGetAnalogStickModuleTypeL(u8 *out_type) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setcalSrv, out_type, 33);
}

Result setcalGetAnalogStickModelParameterL(SetCalAnalogStickModelParameter *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 34, *out);
}

Result setcalGetAnalogStickFactoryCalibrationL(SetCalAnalogStickFactoryCalibration *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 35, *out);
}

Result setcalGetAnalogStickModuleTypeR(u8 *out_type) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setcalSrv, out_type, 36);
}

Result setcalGetAnalogStickModelParameterR(SetCalAnalogStickModelParameter *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 37, *out);
}

Result setcalGetAnalogStickFactoryCalibrationR(SetCalAnalogStickFactoryCalibration *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 38, *out);
}

Result setcalGetConsoleSixAxisSensorModuleType(u8 *out_type) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setcalSrv, out_type, 39);
}

Result setcalGetConsoleSixAxisSensorHorizontalOffset(SetCalConsoleSixAxisSensorHorizontalOffset *out) {
    if (hosversionBefore(8,1,1))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setcalSrv, 40, *out);
}

Result setcalGetBatteryVersion(u8 *out_version) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setcalSrv, out_version, 41);
}

Result setcalGetDeviceId(u64 *out_device_id) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOut64(&g_setcalSrv, out_device_id, 42);
}

Result setcalGetConsoleSixAxisSensorMountType(u8 *out_type) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU8(&g_setcalSrv, out_type, 43);
}
