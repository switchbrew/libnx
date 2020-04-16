/**
 * @file mii.h
 * @brief Mii services (mii:*) IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    MiiServiceType_System  = 0,  ///< Initializes mii:e.
    MiiServiceType_User    = 1,  ///< Initializes mii:u.
} MiiServiceType;

/// Mii age.
typedef enum {
    MiiAge_Young   = 0,  ///< Young
    MiiAge_Normal  = 1,  ///< Normal
    MiiAge_Old     = 2,  ///< Old
    MiiAge_All     = 3,  ///< All of them
} MiiAge;

/// Mii gender.
typedef enum {
    MiiGender_Male    = 0,  ///< Male
    MiiGender_Female  = 1,  ///< Female
    MiiGender_All     = 2,  ///< Both of them
} MiiGender;

/// Mii race.
typedef enum {
    MiiRace_Black  = 0,  ///< Black
    MiiRace_White  = 1,  ///< White
    MiiRace_Asian  = 2,  ///< Asian
    MiiRace_All    = 3,  ///< All of them
} MiiRace;

// Mii source flag.
typedef enum {
    MiiSourceFlag_Database  = BIT(0),                                          ///< Miis created by the user
    MiiSourceFlag_Default   = BIT(1),                                          ///< Default console miis
    MiiSourceFlag_All       = MiiSourceFlag_Database | MiiSourceFlag_Default,  ///< All of them
} MiiSourceFlag;

// Mii special key code
typedef enum {
    MiiSpecialKeyCode_Normal   = 0,           ///< Normal miis
    MiiSpecialKeyCode_Special  = 0xA523B78F,  ///< Special miis
} MiiSpecialKeyCode;

typedef struct {
    Service s;
} MiiDatabase;

// Mii create ID.
typedef struct {
    Uuid uuid;
} MiiCreateId;

// Mii data structure.
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

/**
 * @brief Opens a mii database.
 * @param[in] key_code Mii key code filter.
 * @param[out] out Database.
 */
Result miiOpenDatabase(MiiDatabase *out, MiiSpecialKeyCode key_code);

/**
 * @brief Returns whether the mii database is updated.
 * @param[in] db Database.
 * @param[in] flag Source flag.
 * @param[out] out Out boolean.
 */
Result miiDatabaseIsUpdated(MiiDatabase *db, u8 *out, MiiSourceFlag flag);

/**
 * @brief Returns whether the mii database is full.
 * @param[in] db Database.
 * @param[in] flag Source flag.
 * @param[out] out Out boolean.
 */
Result miiDatabaseIsFull(MiiDatabase *db, u8 *out);

/**
 * @brief Returns number of miis in the database with the specified source flag.
 * @param[in] db Database.
 * @param[in] flag Source flag.
 * @param[out] out Out count.
 */
Result miiDatabaseGetCount(MiiDatabase *db, u32 *out, MiiSourceFlag flag);

/**
 * @brief Reads mii charinfo data from the specified source flag.
 * @param[in] db Database.
 * @param[in] flag Source flag.
 * @param[out] out_infos Output mii charinfo array.
 * @param[in] out_infos_count Amount of mii chainfos to read.
 * @param[out] out_count Number of mii charinfos which were read.
 */
Result miiDatabaseGetCharInfo(MiiDatabase *db, MiiSourceFlag flag, MiiCharInfo *out_infos, size_t out_infos_count, u32 *out_count);

/**
 * @brief Generates a random mii charinfo (doesn't register it in the console database).
 * @param[in] db Database.
 * @param[in] age Mii's age.
 * @param[in] gender Mii's gender.
 * @param[in] race Mii's race.
 * @param[out] out_info Out mii charinfo data.
 */
Result miiDatabaseBuildRandom(MiiDatabase *db, MiiAge age, MiiGender gender, MiiRace race, MiiCharInfo *out_info);

/// Closes a mii database.
void miiDatabaseClose(MiiDatabase *db);
