/**
 * @file error.h
 * @brief Wrapper for using the error LibraryApplet.
 * @author StuntHacks, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"
#include "../services/set.h"

/// Error type for ErrorCommonHeader.type.
typedef enum {
    ErrorType_Normal           = 0,  ///< Normal
    ErrorType_System           = 1,  ///< System
    ErrorType_Application      = 2,  ///< Application
    ErrorType_Eula             = 3,  ///< EULA
    ErrorType_Pctl             = 4,  ///< Parental Controls
    ErrorType_Record           = 5,  ///< Record
    ErrorType_SystemUpdateEula = 8,  ///< SystemUpdateEula
} ErrorType;

/// Stores error-codes which are displayed as XXXX-XXXX, low for the former and desc for the latter.
typedef struct {
    u32 low;             ///< The module portion of the error, normally this should be set to module + 2000.
    u32 desc;            ///< The error description.
} ErrorCode;

/// Error context.
typedef struct {
    u8 type;             ///< Type
    u8 pad[7];           ///< Padding
    u8 data[0x1f4];      ///< Data
    Result res;          ///< Result
} ErrorContext;

/// Common header for the start of the arg storage.
typedef struct {
    u8 type;             ///< Type, see \ref ErrorType.
    u8 jumpFlag;         ///< When clear, this indicates WithoutJump.
    u8 unk_x2[3];        ///< Unknown
    u8 contextFlag;      ///< When set with ::ErrorType_Normal, indicates that an additional storage is pushed for \ref ErrorResultBacktrace. [4.0.0+] Otherwise, when set indicates that an additional storage is pushed for \ref ErrorContext.
    u8 resultFlag;       ///< ErrorCommonArg: When clear, errorCode is used, otherwise the applet generates the error-code from res.
    u8 contextFlag2;     ///< Similar to contextFlag except for ErrorCommonArg, indicating \ref ErrorContext is used.
} ErrorCommonHeader;

/// Common error arg data.
typedef struct {
    ErrorCommonHeader hdr;            ///< Common header.
    ErrorCode errorCode;              ///< \ref ErrorCode
    Result res;                       ///< Result
} ErrorCommonArg;

/// Error arg data for certain errors with module PCTL.
typedef struct {
    ErrorCommonHeader hdr;            ///< Common header.
    Result res;                       ///< Result
} ErrorPctlArg;

/// ResultBacktrace
typedef struct {
    s32 count;                        ///< Total entries in the backtrace array.
    Result backtrace[0x20];           ///< Result backtrace.
} ErrorResultBacktrace;

/// Error arg data for EULA.
typedef struct {
    ErrorCommonHeader hdr;            ///< Common header.
    SetRegion regionCode;             ///< \ref SetRegion
} ErrorEulaArg;

/// Additional input storage data for \ref errorSystemUpdateEulaShow.
typedef struct {
    u8 data[0x20000];                ///< data
} ErrorEulaData;

/// Error arg data for Record.
typedef struct {
    ErrorCommonHeader hdr;            ///< Common header.
    ErrorCode errorCode;              ///< \ref ErrorCode
    u64 timestamp;                    ///< POSIX timestamp.
} ErrorRecordArg;

/// SystemErrorArg
typedef struct {
    ErrorCommonHeader hdr;            ///< Common header.
    ErrorCode errorCode;              ///< \ref ErrorCode
    u64 languageCode;                 ///< See set.h.
    char dialogMessage[0x800];        ///< UTF-8 Dialog message.
    char fullscreenMessage[0x800];    ///< UTF-8 Fullscreen message (displayed when the user clicks on "Details").
} ErrorSystemArg;

/// Error system config.
typedef struct {
    ErrorSystemArg arg;               ///< Arg data.
    ErrorContext ctx;                 ///< Optional error context.
} ErrorSystemConfig;

/// ApplicationErrorArg
typedef struct {
    ErrorCommonHeader hdr;            ///< Common header.
    u32 errorNumber;                  ///< Raw decimal error number which is displayed in the dialog.
    u64 languageCode;                 ///< See set.h.
    char dialogMessage[0x800];        ///< UTF-8 Dialog message.
    char fullscreenMessage[0x800];    ///< UTF-8 Fullscreen message (displayed when the user clicks on "Details").
} PACKED ErrorApplicationArg;

/// Error application config.
typedef struct {
    ErrorApplicationArg arg;          ///< Arg data.
} ErrorApplicationConfig;

/**
 * @brief Creates an \ref ErrorCode.
 * @param low  The module portion of the error, normally this should be set to module + 2000.
 * @param desc The error description.
 */
static inline ErrorCode errorCodeCreate(u32 low, u32 desc) {
    return (ErrorCode){low, desc};
}

/**
 * @brief Creates an \ref ErrorCode with the input Result. Wrapper for \ref errorCodeCreate.
 * @param res Input Result.
 */
static inline ErrorCode errorCodeCreateResult(Result res) {
    return errorCodeCreate(2000 + R_MODULE(res), R_DESCRIPTION(res));
}

/**
 * @brief Creates an invalid \ref ErrorCode.
 */
static inline ErrorCode errorCodeCreateInvalid(void) {
    return (ErrorCode){0};
}

/**
 * @brief Checks whether the input ErrorCode is valid.
 * @param errorCode \ref ErrorCode
 */
static inline bool errorCodeIsValid(ErrorCode errorCode) {
    return errorCode.low!=0;
}

/**
 * @brief Launches the applet for displaying the specified Result.
 * @param res Result
 * @param jumpFlag Jump flag, normally this is true.
 * @param ctx Optional \ref ErrorContext, can be NULL. Unused when jumpFlag=false. Ignored on pre-4.0.0, since it's only available for [4.0.0+].
 * @note Sets the following fields: jumpFlag and contextFlag2. Uses ::ErrorType_Normal normally.
 * @note For module=PCTL errors with desc 100-119 this sets uses ::ErrorType_Pctl, in which case the applet will display the following special dialog: "This software is restricted by Parental Controls".
 * @note If the input Result is 0xC8A2, the applet will display a special dialog regarding the current application requiring a software update, with buttons "Later" and "Restart".
 * @note [3.0.0+] If the input Result is 0xCAA2, the applet will display a special dialog related to DLC version.
 * @warning This applet creates an error report that is logged in the system, when not handling the above special dialogs. Proceed at your own risk!
 */
Result errorResultShow(Result res, bool jumpFlag, const ErrorContext* ctx);

/**
 * @brief Launches the applet for displaying the specified ErrorCode.
 * @param errorCode \ref ErrorCode
 * @param jumpFlag Jump flag, normally this is true.
 * @param ctx Optional \ref ErrorContext, can be NULL. Unused when jumpFlag=false. Ignored on pre-4.0.0, since it's only available for [4.0.0+].
 * @note Sets the following fields: jumpFlag and contextFlag2. resultFlag=1. Uses ::ErrorType_Normal.
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorCodeShow(ErrorCode errorCode, bool jumpFlag, const ErrorContext* ctx);

/**
 * @brief Creates an ErrorResultBacktrace struct.
 * @param backtrace \ref ErrorResultBacktrace struct.
 * @param count Total number of entries.
 * @param entries Input array of Result.
 */
Result errorResultBacktraceCreate(ErrorResultBacktrace* backtrace, s32 count, const Result* entries);

/**
 * @brief Launches the applet for \ref ErrorResultBacktrace.
 * @param backtrace ErrorResultBacktrace struct.
 * @param res Result
 * @note Sets the following fields: jumpFlag=1, contextFlag=1, and uses ::ErrorType_Normal.
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorResultBacktraceShow(Result res, const ErrorResultBacktrace* backtrace);

/**
 * @brief Launches the applet for displaying the EULA.
 * @param RegionCode \ref SetRegion
 * @note Sets the following fields: jumpFlag=1, regionCode, and uses ::ErrorType_Eula.
 */
Result errorEulaShow(SetRegion RegionCode);

/**
 * @brief Launches the applet for displaying the system-update EULA.
 * @param RegionCode \ref SetRegion
 * @param eula EULA data. Address must be 0x1000-byte aligned.
 * @note Sets the following fields: jumpFlag=1, regionCode, and uses ::ErrorType_SystemUpdateEula.
 */
Result errorSystemUpdateEulaShow(SetRegion RegionCode, const ErrorEulaData* eula);

/**
 * @brief Launches the applet for displaying an error full-screen, using the specified ErrorCode and timestamp.
 * @param errorCode \ref ErrorCode
 * @param timestamp POSIX timestamp.
 * @note Sets the following fields: jumpFlag=1, errorCode, timestamp, and uses ::ErrorType_Record.
 * @note The applet does not log an error report for this. error*RecordShow is used by qlaunch for displaying previously logged error reports.
 */
Result errorCodeRecordShow(ErrorCode errorCode, u64 timestamp);

/**
 * @brief Launches the applet for displaying an error full-screen, using the specified Result and timestamp.
 * @param res Result
 * @param timestamp POSIX timestamp.
 * @note Wrapper for \ref errorCodeRecordShow, see \ref errorCodeRecordShow notes.
 */
static inline Result errorResultRecordShow(Result res, u64 timestamp) {
    return errorCodeRecordShow(errorCodeCreateResult(res), timestamp);
}

/**
 * @brief Creates an ErrorSystemConfig struct.
 * @param c ErrorSystemConfig struct.
 * @param dialog_message UTF-8 dialog message.
 * @param fullscreen_message UTF-8 fullscreen message, displayed when the user clicks on "Details". Optional, can be NULL (which disables displaying Details).
 * @note Sets the following fields: {strings}, and uses ::ErrorType_System. The rest are cleared.
 * @note On pre-5.0.0 this will initialize languageCode by using: setInitialize(), setMakeLanguageCode(SetLanguage_ENUS, ...), and setExit(). This is needed since an empty languageCode wasn't supported until [5.0.0+] (which would also use SetLanguage_ENUS).
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorSystemCreate(ErrorSystemConfig* c, const char* dialog_message, const char* fullscreen_message);

/**
 * @brief Launches the applet with the specified config.
 * @param c ErrorSystemConfig struct.
 */
Result errorSystemShow(ErrorSystemConfig* c);

/**
 * @brief Sets the error code.
 * @param c    ErrorSystemConfig struct.
 * @param errorCode \ref ErrorCode
 */
static inline void errorSystemSetCode(ErrorSystemConfig* c, ErrorCode errorCode) {
    c->arg.errorCode = errorCode;
}

/**
 * @brief Sets the error code, using the input Result. Wrapper for \ref errorSystemSetCode.
 * @param c    ErrorSystemConfig struct.
 * @param res  The Result to set.
 */
static inline void errorSystemSetResult(ErrorSystemConfig* c, Result res) {
    errorSystemSetCode(c, errorCodeCreateResult(res));
}

/**
 * @brief Sets the LanguageCode.
 * @param c            ErrorSystemConfig struct.
 * @param LanguageCode LanguageCode, see set.h.
 */
static inline void errorSystemSetLanguageCode(ErrorSystemConfig* c, u64 LanguageCode) {
    c->arg.languageCode = LanguageCode;
}

/**
 * @brief Sets the ErrorContext.
 * @note Only available on [4.0.0+], on older versions this will return without setting the context.
 * @param c   ErrorSystemConfig struct.
 * @param ctx ErrorContext, NULL to clear it.
 */
void errorSystemSetContext(ErrorSystemConfig* c, const ErrorContext* ctx);

/**
 * @brief Creates an ErrorApplicationConfig struct.
 * @param c ErrorApplicationConfig struct.
 * @param dialog_message UTF-8 dialog message.
 * @param fullscreen_message UTF-8 fullscreen message, displayed when the user clicks on "Details". Optional, can be NULL (which disables displaying Details).
 * @note Sets the following fields: jumpFlag=1, {strings}, and uses ::ErrorType_Application. The rest are cleared.
 * @note On pre-5.0.0 this will initialize languageCode by using: setInitialize(), setMakeLanguageCode(SetLanguage_ENUS, ...), and setExit(). This is needed since an empty languageCode wasn't supported until [5.0.0+] (which would also use SetLanguage_ENUS).
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorApplicationCreate(ErrorApplicationConfig* c, const char* dialog_message, const char* fullscreen_message);

/**
 * @brief Launches the applet with the specified config.
 * @param c ErrorApplicationConfig struct.
 */
Result errorApplicationShow(ErrorApplicationConfig* c);

/**
 * @brief Sets the error code number.
 * @param c           ErrorApplicationConfig struct.
 * @param errorNumber Error code number. Raw decimal error number which is displayed in the dialog.
 */
static inline void errorApplicationSetNumber(ErrorApplicationConfig* c, u32 errorNumber) {
    c->arg.errorNumber = errorNumber;
}

/**
 * @brief Sets the LanguageCode.
 * @param c            ErrorApplicationConfig struct.
 * @param LanguageCode LanguageCode, see set.h.
 */
static inline void errorApplicationSetLanguageCode(ErrorApplicationConfig* c, u64 LanguageCode) {
    c->arg.languageCode = LanguageCode;
}

