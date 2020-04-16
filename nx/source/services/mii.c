#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/mii.h"

static MiiServiceType g_miiServiceType;
static Service g_miiSrv;

NX_GENERATE_SERVICE_GUARD_PARAMS(mii, (MiiServiceType service_type), (service_type));

Result _miiInitialize(MiiServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    g_miiServiceType = service_type;
    switch (g_miiServiceType) {
        case MiiServiceType_System:
            rc = smGetService(&g_miiSrv, "mii:e");
            break;
        case MiiServiceType_User:
            rc = smGetService(&g_miiSrv, "mii:u");
            break;
    }

    return rc;
}

void _miiCleanup(void) {
    serviceClose(&g_miiSrv);
}

Service* miiGetServiceSession(void) {
    return &g_miiSrv;
}

Result miiOpenDatabase(MiiDatabase *out, MiiSpecialKeyCode key_code) {
    u32 in = (u32)key_code;
    return serviceDispatchIn(&g_miiSrv, 0, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result miiDatabaseIsUpdated(MiiDatabase *db, bool *out_updated, MiiSourceFlag flag) {
    u32 in = (u32)flag;
    u8 tmp = 0;
    Result rc = serviceDispatchInOut(&db->s, 0, in, tmp);
    if (R_SUCCEEDED(rc) && out_updated) *out_updated = tmp & 1;
    return rc;
}

Result miiDatabaseIsFull(MiiDatabase *db, bool *out_full) {
    u8 tmp = 0;
    Result rc = serviceDispatchOut(&db->s, 1, tmp);
    if (R_SUCCEEDED(rc) && out_full) *out_full = tmp & 1;
    return rc;
}
Result miiDatabaseGetCount(MiiDatabase *db, s32 *out_count, MiiSourceFlag flag) {
    u32 in = (u32)flag;
    return serviceDispatchInOut(&db->s, 2, in, *out_count);
}

Result miiDatabaseGet1(MiiDatabase *db, MiiSourceFlag flag, MiiCharInfo *out_infos, s32 count, s32 *total_out) {
    u32 in = (u32)flag;
    return serviceDispatchInOut(&db->s, 4, in, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_infos, count * sizeof(MiiCharInfo) } },
    );
}

Result miiDatabaseBuildRandom(MiiDatabase *db, MiiAge age, MiiGender gender, MiiFaceColor face_color, MiiCharInfo *out_info) {
    const struct {
        u32 age;
        u32 gender;
        u32 face_color;
    } in = { age, gender, face_color };
    return serviceDispatchInOut(&db->s, 6, in, *out_info);
}

void miiDatabaseClose(MiiDatabase *db) {
    serviceClose(&db->s);
}
