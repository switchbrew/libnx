/**
 * @file error.h
 * @brief Wrapper for using the error LibraryApplet.
 * @author StuntHacks, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

typedef struct {
    u8 unk_x0[0x200];
} ErrorContext;

/// Common header for the start of the arg storage.
typedef struct {
    u8 type;
    u8 unk_x1;
    u8 unk_x2[3];
    u8 contextFlag;
    u8 resultFlag;       ///< \ref ErrorCommonArg: When clear, errorCode is used, otherwise the applet generates the error-code from res.
    u8 unk_x7;
} ErrorCommonHeader;

/// Error arg data for non-{System/Application}.
typedef struct {
    ErrorCommonHeader hdr;
    u64 errorCode;
    Result res;
} ErrorCommonArg;

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

/**
 * @brief Creates an ErrorSystemConfig struct.
 * @param c ErrorSystemConfig struct.
 * @param dialog_message UTF-8 dialog message.
 * @param fullscreen_message UTF-8 fullscreen message, displayed when the user clicks on "Details".
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
 * @brief Launches with the specified config.
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
 * @note Only available on [4.0.0+], on older versions \ref errorSystemShow will skip pushing the storage for this.
 * @param c   ErrorSystemConfig struct.
 * @param ctx ErrorContext, NULL to clear it.
 */
void errorSystemSetContext(ErrorSystemConfig* c, ErrorContext* ctx);

