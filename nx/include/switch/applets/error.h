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

typedef struct {
    char str[0x1f4];
    u8 unk_x1f4[0xc];
} PACKED ErrorContext;

/// Common header for the start of the arg storage.
typedef struct {
    u8 type;
    u8 jumpFlag;         ///< When clear, this indicates WithoutJump.
    u8 unk_x2[3];
    u8 contextFlag;
    u8 resultFlag;       ///< \ref ErrorCommonArg: When clear, errorCode is used, otherwise the applet generates the error-code from res.
    u8 contextFlag2;     ///< Similar to contextFlag except for ErrorCommonArg, indicating \ref ErrorContext is used.
} ErrorCommonHeader;

/// Error arg data for non-{System/Application}.
typedef struct {
    ErrorCommonHeader hdr;
    u64 errorCode;
    Result res;
} ErrorCommonArg;

/// Error arg data for certain errors with module PCTL.
typedef struct {
    ErrorCommonHeader hdr;
    Result res;
} ErrorPctlArg;

/// ResultBacktrace
typedef struct {
    s32 count;
    Result backtrace[0x20];
} ErrorResultBacktrace;

typedef struct {
    ErrorCommonHeader hdr;
    SetRegion regionCode;
} ErrorEulaArg;

typedef struct {
    u8 data[0x20000];
} ErrorEulaData;

typedef struct {
    ErrorCommonHeader hdr;
    u64 errorCode;
    u64 timestamp;                   ///< POSIX timestamp.
} ErrorRecordArg;

/// SystemErrorArg
typedef struct {
    ErrorCommonHeader hdr;
    u64 errorCode;
    u64 languageCode;
    char dialogMessage[0x800];        ///< UTF-8 Dialog message.
    char fullscreenMessage[0x800];    ///< UTF-8 Fullscreen message (displayed when the user clicks on "Details").
} ErrorSystemArg;

typedef struct {
    ErrorSystemArg arg;
    ErrorContext ctx;
} ErrorSystemConfig;

/// ApplicationErrorArg
typedef struct {
    ErrorCommonHeader hdr;
    u32 errorNumber;
    u64 languageCode;
    char dialogMessage[0x800];        ///< UTF-8 Dialog message.
    char fullscreenMessage[0x800];    ///< UTF-8 Fullscreen message (displayed when the user clicks on "Details").
} PACKED ErrorApplicationArg;

typedef struct {
    ErrorApplicationArg arg;
    ErrorContext ctx;
} ErrorApplicationConfig;

/**
 * @brief Launches the applet for displaying the specified Result.
 * @param res Result
 * @param jumpFlag Jump flag, normally this is true.
 * @param ctx \ref ErrorContext, unused when jumpFlag=false. Ignored on pre-4.0.0, since it's only available for [4.0.0+].
 * @note Sets the following fields: jumpFlag and contextFlag2. Uses type=0 normally.
 * @note For module=PCTL errors with desc 100-119 this sets \ref ErrorCommonHeader type=4, in which case the applet will display the following dialog (without the report logging mentioned below): "This software is restricted by Parental Controls".
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorResultShow(Result res, bool jumpFlag, ErrorContext* ctx);

/**
 * @brief Launches the applet for displaying the specified ErrorCode.
 * @param low  The module portion of the error, normally this should be set to module + 2000.
 * @param desc The error description.
 * @param jumpFlag Jump flag, normally this is true.
 * @param ctx \ref ErrorContext, unused when jumpFlag=false. Ignored on pre-4.0.0, since it's only available for [4.0.0+].
 * @note Sets the following fields: jumpFlag and contextFlag2. type=0 and resultFlag=1.
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorCodeShow(u32 low, u32 desc, bool jumpFlag, ErrorContext* ctx);

/**
 * @brief Creates an ErrorResultBacktrace struct.
 * @param backtrace \ref ErrorResultBacktrace struct.
 * @param count Total number of entries.
 * @param entries Input array of Result.
 */
Result errorResultBacktraceCreate(ErrorResultBacktrace* backtrace, s32 count, Result* entries);

/**
 * @brief Closes an ErrorResultBacktrace struct.
 * @param backtrace \ref ErrorResultBacktrace struct.
 */
void errorResultBacktraceClose(ErrorResultBacktrace* backtrace);

/**
 * @brief Launches the applet for \ref ErrorResultBacktrace.
 * @param backtrace ErrorResultBacktrace struct.
 * @param res Result
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorResultBacktraceShow(Result res, ErrorResultBacktrace* backtrace);

/**
 * @brief Launches the applet for displaying the EULA.
 * @param RegionCode \ref SetRegion
 */
Result errorEulaShow(SetRegion RegionCode);

/**
 * @brief Launches the applet for displaying the system-update EULA.
 * @param RegionCode \ref SetRegion
 * @param eula EULA data. Address must be 0x1000-byte aligned.
 */
Result errorSystemUpdateEulaShow(SetRegion RegionCode, ErrorEulaData* eula);

/**
 * @brief Launches the applet for displaying an error full-screen, using the specified Result and timestamp.
 * @param res Result
 * @param timestamp POSIX timestamp.
 * @note Wrapper for \ref errorCodeRecordShow.
 * @note The applet does not log an error report for this.
 */
Result errorResultRecordShow(Result res, u64 timestamp);

/**
 * @brief Launches the applet for displaying an error full-screen, using the specified ErrorCode and timestamp.
 * @param low  The module portion of the error, normally this should be set to module + 2000.
 * @param desc The error description.
 * @note The applet does not log an error report for this.
 */
Result errorCodeRecordShow(u32 low, u32 desc, u64 timestamp);

/**
 * @brief Creates an ErrorSystemConfig struct.
 * @param c ErrorSystemConfig struct.
 * @param dialog_message UTF-8 dialog message.
 * @param fullscreen_message UTF-8 fullscreen message, displayed when the user clicks on "Details". Optional, can be NULL (which disables displaying Details).
 * @note Sets the following fields: type=1 and {strings}. The rest are cleared.
 * @note On pre-5.0.0 this will initialize languageCode by using: setInitialize(), setMakeLanguageCode(SetLanguage_ENUS, ...), and setExit(). This is needed since an empty languageCode wasn't supported until [5.0.0+] (which would also use SetLanguage_ENUS).
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorSystemCreate(ErrorSystemConfig* c, const char* dialog_message, const char* fullscreen_message);

/**
 * @brief Closes an ErrorSystemConfig struct.
 * @param c ErrorSystemConfig struct.
 */
void errorSystemClose(ErrorSystemConfig* c);

/**
 * @brief Launches the applet with the specified config.
 * @param c ErrorSystemConfig struct.
 */
Result errorSystemShow(ErrorSystemConfig* c);

/**
 * @brief Sets the error code.
 * @param c    ErrorSystemConfig struct.
 * @param low  The module portion of the error, normally this should be set to module + 2000.
 * @param desc The error description.
 */
void errorSystemSetCode(ErrorSystemConfig* c, u32 low, u32 desc);

/**
 * @brief Sets the error code, using the input Result. Wrapper for \ref errorSystemSetCode.
 * @param c    ErrorSystemConfig struct.
 * @param res  The Result to set.
 */
void errorSystemSetResult(ErrorSystemConfig* c, Result res);

/**
 * @brief Sets the LanguageCode.
 * @param c            ErrorSystemConfig struct.
 * @param LanguageCode LanguageCode, see set.h.
 */
void errorSystemSetLanguageCode(ErrorSystemConfig* c, u64 LanguageCode);

/**
 * @brief Sets the ErrorContext.
 * @note Only available on [4.0.0+], on older versions this will return without setting the context.
 * @param c   ErrorSystemConfig struct.
 * @param ctx ErrorContext, NULL to clear it.
 */
void errorSystemSetContext(ErrorSystemConfig* c, ErrorContext* ctx);

/**
 * @brief Creates an ErrorApplicationConfig struct.
 * @param c ErrorApplicationConfig struct.
 * @param dialog_message UTF-8 dialog message.
 * @param fullscreen_message UTF-8 fullscreen message, displayed when the user clicks on "Details". Optional, can be NULL (which disables displaying Details).
 * @note Sets the following fields: type=2, unk_x1=1, and {strings}. The rest are cleared.
 * @note On pre-5.0.0 this will initialize languageCode by using: setInitialize(), setMakeLanguageCode(SetLanguage_ENUS, ...), and setExit(). This is needed since an empty languageCode wasn't supported until [5.0.0+] (which would also use SetLanguage_ENUS).
 * @warning This applet creates an error report that is logged in the system. Proceed at your own risk!
 */
Result errorApplicationCreate(ErrorApplicationConfig* c, const char* dialog_message, const char* fullscreen_message);

/**
 * @brief Closes an ErrorApplicationConfig struct.
 * @param c ErrorApplicationConfig struct.
 */
void errorApplicationClose(ErrorApplicationConfig* c);

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
void errorApplicationSetNumber(ErrorApplicationConfig* c, u32 errorNumber);

/**
 * @brief Sets the LanguageCode.
 * @param c            ErrorApplicationConfig struct.
 * @param LanguageCode LanguageCode, see set.h.
 */
void errorApplicationSetLanguageCode(ErrorApplicationConfig* c, u64 LanguageCode);

