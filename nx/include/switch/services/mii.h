/**
 * @file mii.h
 * @brief Mii services IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    MiiServiceType_System,
    MiiServiceType_User
} MiiServiceType;

typedef enum {
    MiiAge_Young,
    MiiAge_Normal,
    MiiAge_Old,
    MiiAge_All
} MiiAge;

typedef enum {
    MiiGender_Male,
    MiiGender_Female,
    MiiGender_All
} MiiGender;

typedef enum {
    MiiRace_Black,
    MiiRace_White,
    MiiRace_Asian,
    MiiRace_All
} MiiRace;

typedef enum {
    MiiSourceFlag_Database = BIT(0),
    MiiSourceFlag_Default = BIT(1),
    MiiSourceFlag_All = MiiSourceFlag_Database | MiiSourceFlag_Default
} MiiSourceFlag;

typedef enum {
    MiiSpecialKeyCode_Normal,
    MiiSpecialKeyCode_Special = 0xA523B78F
} MiiSpecialKeyCode;

typedef struct {
    Service s;
} MiiDatabase;

typedef struct {
    Uuid uuid;
} MiiCreateId;

typedef struct {
    MiiCreateId create_id;
    u16 mii_name[10+1]; ///< utf-16be, null-terminated
    u8 unk_x26;
    u8 mii_color;
    u8 mii_sex;
    u8 mii_height;
    u8 mii_width;
    u8 unk_x2b[2];
    u8 mii_face_shape;
    u8 mii_face_color;
    u8 mii_wrinkles_style;
    u8 mii_makeup_style;
    u8 mii_hair_style;
    u8 mii_hair_color;
    u8 mii_has_hair_flipped;
    u8 mii_eye_style;
    u8 mii_eye_color;
    u8 mii_eye_size;
    u8 mii_eye_thickness;
    u8 mii_eye_angle;
    u8 mii_eye_pos_x;
    u8 mii_eye_pos_y;
    u8 mii_eyebrow_style;
    u8 mii_eyebrow_color;
    u8 mii_eyebrow_size;
    u8 mii_eyebrow_thickness;
    u8 mii_eyebrow_angle;
    u8 mii_eyebrow_pos_x;
    u8 mii_eyebrow_pos_y;
    u8 mii_nose_style;
    u8 mii_nose_size;
    u8 mii_nose_pos;
    u8 mii_mouth_style;
    u8 mii_mouth_color;
    u8 mii_mouth_size;
    u8 mii_mouth_thickness;
    u8 mii_mouth_pos;
    u8 mii_facial_hair_color;
    u8 mii_beard_style;
    u8 mii_mustache_style;
    u8 mii_mustache_size;
    u8 mii_mustache_pos;
    u8 mii_glasses_style;
    u8 mii_glasses_color;
    u8 mii_glasses_size;
    u8 mii_glasses_pos;
    u8 mii_has_mole;
    u8 mii_mole_size;
    u8 mii_mole_pos_x;
    u8 mii_mole_pos_y;
    u8 unk_x57;
} MiiCharInfo;

/// Initialize mii (mii:e).
Result miiInitialize(MiiServiceType service_type);

/// Exit mii.
void miiExit(void);

/// Gets the Service object for the actual mii service session.
Service* miiGetServiceSession(void);

Result miiOpenDatabase(MiiDatabase *out, MiiSpecialKeyCode key_code);

Result miiDatabaseIsUpdated(MiiDatabase *db, u8 *out, MiiSourceFlag flag);
Result miiDatabaseIsFull(MiiDatabase *db, u8 *out);
Result miiDatabaseGetCount(MiiDatabase *db, u32 *out, MiiSourceFlag flag);
Result miiDatabaseGetCharInfo(MiiDatabase *db, MiiSourceFlag flag, MiiCharInfo *out_infos, size_t out_infos_count, u32 *out_count);
Result miiDatabaseBuildRandom(MiiDatabase *db, MiiAge age, MiiGender gender, MiiRace race, MiiCharInfo *out_info);

void miiDatabaseClose(MiiDatabase *db);
