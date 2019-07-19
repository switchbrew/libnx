/**
 * @file pdm.h
 * @brief PDM (pdm:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../kernel/event.h"

/// PlayEventType
typedef enum {
    PdmPlayEventType_Applet              = 0,  ///< Applet
    PdmPlayEventType_Account             = 1,  ///< Account
    PdmPlayEventType_PowerStateChange    = 2,  ///< PowerStateChange
    PdmPlayEventType_OperationModeChange = 3,  ///< OperationModeChange
    PdmPlayEventType_Initialize          = 4,  ///< Initialize. Used for the very first PlayEvent entry in the log.
} PdmPlayEventType;

/// AppletEventType
typedef enum {
    PdmAppletEventType_Launch      = 0,  ///< "launch"
    PdmAppletEventType_Exit        = 1,  ///< "exit"
    PdmAppletEventType_InFocus     = 2,  ///< "in_focus"
    PdmAppletEventType_OutOfFocus  = 3,  ///< "out_of_focus"
    PdmAppletEventType_OutOfFocus4 = 4,  ///< "out_of_focus"
    PdmAppletEventType_Exit5       = 5,  ///< "exit"
    PdmAppletEventType_Exit6       = 6,  ///< "exit"
} PdmAppletEventType;

/// PlayLogPolicy
typedef enum {
    PdmPlayLogPolicy_All     = 0,        ///< All pdm:qry commands which require ::PdmPlayEventType_Applet and AppletId = Application will only return the entry when PlayLogPolicy matches this value.
    PdmPlayLogPolicy_LogOnly = 1,        ///< The above commands will filter out the entry with this.
    PdmPlayLogPolicy_None    = 2,        ///< The pdm:ntfy commands which handle ::PdmPlayEventType_Applet logging will immediately return 0 when the input param matches this value.
} PdmPlayLogPolicy;

/// ApplicationEvent.
/// Timestamp format, converted from PosixTime: total minutes since epoch UTC 1999/12/31 00:00:00.
/// See \ref pdmPlayTimestampToPosix.
typedef struct {
    u64 titleID;                      ///< Application titleID.
    u32 entryindex;                   ///< Entry index.
    u32 timestampUser;                ///< See PdmPlayEvent::timestampUser, with the above timestamp format.
    u32 timestampNetwork;             ///< See PdmPlayEvent::timestampNetwork, with the above timestamp format.
    u8 eventType;                     ///< \ref PdmAppletEventType
    u8 pad[3];                        ///< Padding.
} PdmApplicationEvent;

/// PlayStatistics
typedef struct {
    u64 titleID;                      ///< Application titleID.

    u32 first_entryindex;             ///< Entry index for the first time the title was played.
    u32 first_timestampUser;          ///< See PdmApplicationEvent::timestampUser. This is for the first time the title was played.
    u32 first_timestampNetwork;       ///< See PdmApplicationEvent::timestampNetwork. This is for the first time the title was played.

    u32 last_entryindex;              ///< Entry index for the last time the title was played.
    u32 last_timestampUser;           ///< See PdmApplicationEvent::timestampUser. This is for the last time the title was played.
    u32 last_timestampNetwork;        ///< See PdmApplicationEvent::timestampNetwork. This is for the last time the title was played.

    u32 playtimeMinutes;              ///< Total play-time in minutes.
    u32 totalLaunches;                ///< Total times the application title was launched.
} PdmPlayStatistics;

/// LastPlayTime.
/// This contains data from the last time the title was played.
typedef struct {
    u64 titleID;                      ///< Application titleID.
    u32 timestampUser;                ///< See PdmApplicationEvent::timestampUser.
    u32 timestampNetwork;             ///< See PdmApplicationEvent::timestampNetwork.
    u32 lastPlayedMinutes;            ///< Total minutes since the title was last played.
    u8 flag;                          ///< Flag indicating whether the above field is set.
    u8 pad[3];                        ///< Padding.
} PdmLastPlayTime;

/// PlayEvent.
/// This is the raw entry struct directly read from FS, without any entry filtering.
typedef struct {
    union {
        struct {
            u32 titleID[2];           ///< titleID.

            union {
                struct {
                    u32 version;      ///< Title version.
                } application;        ///< For AppletId = 0x01.

                struct {
                    u8 flag;          ///< Set to 0x1 by pdm:ntfy cmd8, indicating that the below field is set to an input param.
                    u8 unk_x9;        ///< Input value from pdm:ntfy cmd8.
                    u8 pad[2];        ///< Padding.
                } applet;             ///< For AppletId != 0x01.

                u32 data;
            } unk_x8;

            u8 appletId;              ///< \ref AppletId
            u8 storageId;             ///< See FsStorageId.
            u8 logPolicy;             ///< \ref PdmPlayLogPolicy
            u8 eventType;             ///< \ref PdmAppletEventType
            u8 unused[0xc];           ///< Unused.
        } applet;

        struct {
            u32 userID[4];            ///< userID.
            u32 titleID[2];           ///< Application titleID, see below.
            u8 type;                  ///< 0-1 to be listed by \ref pdmqryQueryAccountEvent, or 2 to include the above titleID.
        } account;

        struct {
            u8 value;                 ///< Input value from the pdm:ntfy command.
            u8 unused[0x1b];          ///< Unused.
        } powerStateChange;

        struct {
            u8 value;                 ///< Input value from the pdm:ntfy command.
            u8 unused[0x1b];          ///< Unused.
        } operationModeChange;

        u8 data[0x1c];
    } eventData;                      ///< titleID/userID stored within here have the u32 low/high swapped in each u64.

    u8 playEventType;                 ///< \ref PdmPlayEventType. Controls which struct in the above eventData is used. ::PdmPlayEventType_Initialize doesn't use eventData.
    u8 pad[3];                        ///< Padding.

    u64 timestampUser;                ///< PosixTime timestamp from StandardUserSystemClock.
    u64 timestampNetwork;             ///< PosixTime timestamp from StandardNetworkSystemClock.
    u64 timestampSteady;              ///< Timestamp in seconds derived from StandardSteadyClock.
} PdmPlayEvent;

/// AccountEvent
typedef struct {
    union { u128 userID; } PACKED;    ///< userID.
    u32 entryindex;                   ///< Entry index.
    u8 pad[4];                        ///< Padding.
    u64 timestampUser;                ///< See PdmPlayEvent::timestampUser.
    u64 timestampNetwork;             ///< See PdmPlayEvent::timestampNetwork.
    u64 timestampSteady;              ///< See PdmPlayEvent::timestampSteady.
    u8 type;                          ///< See PdmPlayEvent::eventData::account::type.
    u8 pad_x31[7];                    ///< Padding.
} PdmAccountEvent;

/// AccountPlayEvent.
/// This is the raw entry struct directly read from FS, without any entry filtering. This is separate from \ref PdmPlayEvent.
typedef struct {
    u8 unk_x0[4];                     ///< Unknown.
    u32 titleID[2];                   ///< titleID, with the u32 low/high words swapped.
    u8 unk_xc[0xc];                   ///< Unknown.
    u64 timestamp0;                   ///< POSIX timestamp.
    u64 timestamp1;                   ///< POSIX timestamp.
} PdmAccountPlayEvent;

/// ApplicationPlayStatistics
typedef struct {
    u64 titleID;                      ///< Application titleID.
    u64 totalPlayTime;                ///< Total play-time in nanoseconds.
    u64 totalLaunches;                ///< Total times the application title was launched.
} PdmApplicationPlayStatistics;

Result pdmqryInitialize(void);
void pdmqryExit(void);
Service* pdmqryGetServiceSession(void);

/**
 * @brief Gets a list of \ref PdmApplicationEvent.
 * @param[in] entryindex Start entry index.
 * @param[out] events Output \ref PdmApplicationEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryApplicationEvent(u32 entryindex, PdmApplicationEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets \ref PdmPlayStatistics for the specified titleID.
 * @param[in] titleID Application titleID.
 * @param[out] stats \ref PdmPlayStatistics
 */
Result pdmqryQueryPlayStatisticsByApplicationId(u64 titleID, PdmPlayStatistics *stats);

/**
 * @brief Gets \ref PdmPlayStatistics for the specified titleID and account userID.
 * @param[in] titleID Application titleID.
 * @param[in] userID Account userID.
 * @param[out] stats \ref PdmPlayStatistics
 */
Result pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(u64 titleID, u128 userID, PdmPlayStatistics *stats);

/**
 * @brief Gets \ref PdmLastPlayTime for the specified titles.
 * @param[out] playtimes Output \ref PdmLastPlayTime array.
 * @param[in] titleIDs Input titleIDs array.
 * @param[in] count Total entries in the input/output arrays.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryLastPlayTime(PdmLastPlayTime *playtimes, const u64 *titleIDs, s32 count, s32 *total_out);

/**
 * @brief Gets a list of \ref PdmPlayEvent.
 * @param[in] entryindex Start entry index.
 * @param[out] events Output \ref PdmPlayEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryPlayEvent(u32 entryindex, PdmPlayEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets range fields which can then be used with the other pdmqry funcs, except for \ref pdmqryQueryAccountPlayEvent.
 * @param[out] total_entries Total entries.
 * @param[out] start_entryindex Start entry index.
 * @param[out] end_entryindex End entry index.
 */
Result pdmqryGetAvailablePlayEventRange(u32 *total_entries, u32 *start_entryindex, u32 *end_entryindex);

/**
 * @brief Gets a list of \ref PdmAccountEvent.
 * @param[in] entryindex Start entry index.
 * @param[out] events Output \ref PdmAccountEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryAccountEvent(u32 entryindex, PdmAccountEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets a list of \ref PdmAccountPlayEvent.
 * @note Only available with [4.0.0+].
 * @param[in] entryindex Start entry index.
 * @param[in] userID Account userID.
 * @param[out] events Output \ref PdmAccountPlayEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryAccountPlayEvent(u32 entryindex, u128 userID, PdmAccountPlayEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets range fields which can then be used with \ref pdmqryQueryAccountPlayEvent.
 * @param[in] userID Account userID.
 * @param[out] total_entries Total entries.
 * @param[out] start_entryindex Start entry index.
 * @param[out] end_entryindex End entry index.
 */
Result pdmqryGetAvailableAccountPlayEventRange(u128 userID, u32 *total_entries, u32 *start_entryindex, u32 *end_entryindex);

/**
 * @brief Gets a list of titles played by the specified user.
 * @note Only available with [6.0.0+].
 * @param[in] userID Account userID.
 * @param[out] titleIDs Output titleID array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryGetUserPlayedApplications(u128 userID, u64 *titleIDs, size_t count, u32 *total_out);

/**
 * @brief Gets an Event which is signaled when logging a new \ref PdmPlayEvent which would be available via \ref pdmqryQueryAccountEvent, where PdmPlayEvent::eventData::account::type is 0. 
 * @note Only available with [6.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] event_out Output Event with autoclear=false.
 */
Result pdmqryGetUserAccountEvent(Event* event_out);

/**
 * @brief Helper function which converts a Play timestamp from the Pdm*Event structs to POSIX.
 * @param[in] timestamp Input timestamp.
 */
static inline u64 pdmPlayTimestampToPosix(u32 timestamp) {
    return ((u64)timestamp) * 60 + 946598400;
}

