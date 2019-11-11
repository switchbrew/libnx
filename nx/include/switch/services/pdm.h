/**
 * @file pdm.h
 * @brief PDM (pdm:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/acc.h"
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

/// AppletEvent.
/// Timestamp format, converted from PosixTime: total minutes since epoch UTC 1999/12/31 00:00:00.
/// See \ref pdmPlayTimestampToPosix.
typedef struct {
    u64 program_id;                   ///< ProgramId.
    u32 entry_index;                  ///< Entry index.
    u32 timestampUser;                ///< See PdmPlayEvent::timestampUser, with the above timestamp format.
    u32 timestampNetwork;             ///< See PdmPlayEvent::timestampNetwork, with the above timestamp format.
    u8 eventType;                     ///< \ref PdmAppletEventType
    u8 pad[3];                        ///< Padding.
} PdmAppletEvent;

/// PlayStatistics
typedef struct {
    u64 application_id;               ///< ApplicationId.

    u32 first_entry_index;            ///< Entry index for the first time the application was played.
    u32 first_timestampUser;          ///< See PdmAppletEvent::timestampUser. This is for the first time the application was played.
    u32 first_timestampNetwork;       ///< See PdmAppletEvent::timestampNetwork. This is for the first time the application was played.

    u32 last_entry_index;             ///< Entry index for the last time the application was played.
    u32 last_timestampUser;           ///< See PdmAppletEvent::timestampUser. This is for the last time the application was played.
    u32 last_timestampNetwork;        ///< See PdmAppletEvent::timestampNetwork. This is for the last time the application was played.

    u32 playtimeMinutes;              ///< Total play-time in minutes.
    u32 totalLaunches;                ///< Total times the application was launched.
} PdmPlayStatistics;

/// LastPlayTime.
/// This contains data from the last time the application was played.
typedef struct {
    u64 application_id;               ///< ApplicationId.
    u32 timestampUser;                ///< See PdmAppletEvent::timestampUser.
    u32 timestampNetwork;             ///< See PdmAppletEvent::timestampNetwork.
    u32 lastPlayedMinutes;            ///< Total minutes since the application was last played.
    u8 flag;                          ///< Flag indicating whether the above field is set.
    u8 pad[3];                        ///< Padding.
} PdmLastPlayTime;

/// PlayEvent.
/// This is the raw entry struct directly read from FS, without any entry filtering.
typedef struct {
    union {
        struct {
            u32 program_id[2];        ///< ProgramId.

            union {
                struct {
                    u32 version;      ///< Application version.
                } application;        ///< For AppletId == ::AppletId_application.

                struct {
                    u8 flag;          ///< Set to 0x1 by pdm:ntfy cmd8, indicating that the below field is set to an input param.
                    u8 mode;          ///< Input value from pdm:ntfy cmd8, see \ref LibAppletMode.
                    u8 pad[2];        ///< Padding.
                } applet;             ///< For AppletId != ::AppletId_application.

                u32 data;
            } unk_x8;

            u8 appletId;              ///< \ref AppletId
            u8 storageId;             ///< \ref NcmStorageId
            u8 logPolicy;             ///< \ref PdmPlayLogPolicy
            u8 eventType;             ///< \ref PdmAppletEventType
            u8 unused[0xc];           ///< Unused.
        } applet;

        struct {
            u32 uid[4];               ///< userId.
            u32 application_id[2];    ///< ApplicationId, see below.
            u8 type;                  ///< 0-1 to be listed by \ref pdmqryQueryAccountEvent, or 2 to include the above ApplicationId.
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
    } eventData;                      ///< ProgramId/ApplicationId/userId stored within here have the u32 low/high swapped in each u64.

    u8 playEventType;                 ///< \ref PdmPlayEventType. Controls which struct in the above eventData is used. ::PdmPlayEventType_Initialize doesn't use eventData.
    u8 pad[3];                        ///< Padding.

    u64 timestampUser;                ///< PosixTime timestamp from StandardUserSystemClock.
    u64 timestampNetwork;             ///< PosixTime timestamp from StandardNetworkSystemClock.
    u64 timestampSteady;              ///< Timestamp in seconds derived from StandardSteadyClock.
} PdmPlayEvent;

/// AccountEvent
typedef struct {
    AccountUid uid;                   ///< \ref AccountUid
    u32 entry_index;                  ///< Entry index.
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
    u32 application_id[2];            ///< ApplicationId, with the u32 low/high words swapped.
    u8 unk_xc[0xc];                   ///< Unknown.
    u64 timestamp0;                   ///< POSIX timestamp.
    u64 timestamp1;                   ///< POSIX timestamp.
} PdmAccountPlayEvent;

/// ApplicationPlayStatistics
typedef struct {
    u64 application_id;               ///< ApplicationId.
    u64 totalPlayTime;                ///< Total play-time in nanoseconds.
    u64 totalLaunches;                ///< Total times the application was launched.
} PdmApplicationPlayStatistics;

/// Initialize pdm:qry.
Result pdmqryInitialize(void);

/// Exit pdm:qry.
void pdmqryExit(void);

/// Gets the Service object for the actual pdm:qry service session.
Service* pdmqryGetServiceSession(void);

/**
 * @brief Gets a list of \ref PdmAppletEvent.
 * @param[in] entry_index Start entry index.
 * @param[out] events Output \ref PdmAppletEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryAppletEvent(s32 entry_index, PdmAppletEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets \ref PdmPlayStatistics for the specified ApplicationId.
 * @param[in] application_id ApplicationId
 * @param[out] stats \ref PdmPlayStatistics
 */
Result pdmqryQueryPlayStatisticsByApplicationId(u64 application_id, PdmPlayStatistics *stats);

/**
 * @brief Gets \ref PdmPlayStatistics for the specified ApplicationId and account userId.
 * @param[in] application_id ApplicationId
 * @param[in] uid \ref AccountUid
 * @param[out] stats \ref PdmPlayStatistics
 */
Result pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(u64 application_id, AccountUid uid, PdmPlayStatistics *stats);

/**
 * @brief Gets \ref PdmLastPlayTime for the specified applications.
 * @param[out] playtimes Output \ref PdmLastPlayTime array.
 * @param[in] application_ids Input ApplicationIds array.
 * @param[in] count Total entries in the input/output arrays.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryLastPlayTime(PdmLastPlayTime *playtimes, const u64 *application_ids, s32 count, s32 *total_out);

/**
 * @brief Gets a list of \ref PdmPlayEvent.
 * @param[in] entry_index Start entry index.
 * @param[out] events Output \ref PdmPlayEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryPlayEvent(s32 entry_index, PdmPlayEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets range fields which can then be used with the other pdmqry funcs, except for \ref pdmqryQueryAccountPlayEvent.
 * @param[out] total_entries Total entries.
 * @param[out] start_entry_index Start entry index.
 * @param[out] end_entry_index End entry index.
 */
Result pdmqryGetAvailablePlayEventRange(s32 *total_entries, s32 *start_entry_index, s32 *end_entry_index);

/**
 * @brief Gets a list of \ref PdmAccountEvent.
 * @param[in] entry_index Start entry index.
 * @param[out] events Output \ref PdmAccountEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryAccountEvent(s32 entry_index, PdmAccountEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets a list of \ref PdmAccountPlayEvent.
 * @note Only available with [4.0.0+].
 * @param[in] entry_index Start entry index.
 * @param[in] uid \ref AccountUid
 * @param[out] events Output \ref PdmAccountPlayEvent array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryAccountPlayEvent(s32 entry_index, AccountUid uid, PdmAccountPlayEvent *events, s32 count, s32 *total_out);

/**
 * @brief Gets range fields which can then be used with \ref pdmqryQueryAccountPlayEvent.
 * @param[in] uid \ref AccountUid
 * @param[out] total_entries Total entries.
 * @param[out] start_entry_index Start entry index.
 * @param[out] end_entry_index End entry index.
 */
Result pdmqryGetAvailableAccountPlayEventRange(AccountUid uid, s32 *total_entries, s32 *start_entry_index, s32 *end_entry_index);

/**
 * @brief Gets a list of applications played by the specified user.
 * @note Only available with [6.0.0+].
 * @param[in] uid \ref AccountUid
 * @param[out] application_ids Output ApplicationIds array.
 * @param[in] count Max entries in the output array.
 * @param[out] total_out Total output entries.
 */
Result pdmqryQueryRecentlyPlayedApplication(AccountUid uid, u64 *application_ids, s32 count, s32 *total_out);

/**
 * @brief Gets an Event which is signaled when logging a new \ref PdmPlayEvent which would be available via \ref pdmqryQueryAccountEvent, where PdmPlayEvent::eventData::account::type is 0.
 * @note Only available with [6.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result pdmqryGetRecentlyPlayedApplicationUpdateEvent(Event* out_event);

/**
 * @brief Helper function which converts a Play timestamp from the Pdm*Event structs to POSIX.
 * @param[in] timestamp Input timestamp.
 */
static inline u64 pdmPlayTimestampToPosix(u32 timestamp) {
    return ((u64)timestamp) * 60 + 946598400;
}

