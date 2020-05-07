/**
 * @file diag.h
 * @brief Diagnostics utils (logging implementation, wrapping lm service)
 * @author XorTroll
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"

/// DiagLogSeverity
typedef enum {
    DiagLogSeverity_Trace = 0,
    DiagLogSeverity_Info = 1,
    DiagLogSeverity_Warn = 2,
    DiagLogSeverity_Error = 3,
    DiagLogSeverity_Fatal = 4
} DiagLogSeverity;

/// DiagSourceInfo
typedef struct {
    u32 line_number;            ///< Source line number.
    const char *file_name;      ///< Source file name.
    const char *function_name;  ///< Source function name.
} DiagSourceInfo;

/// DiagLogMetadata
typedef struct {
    DiagSourceInfo source_info; ///< Source info.
    DiagLogSeverity severity;   ///< Log severity.
    bool verbosity;             ///< Verbosity.
    const char *text_log;       ///< Log message.
} DiagLogMetadata;

/**
 * @brief Logs via lm.
 * @note Logging is stubbed in retail.
 * @param[in] metadata Log parameters.
 */
void diagLogImpl(const DiagLogMetadata *metadata);

// Helper macros.

#define DIAG_DETAILED_LOG(log_severity, log_verbosity, msg) ({ \
    const DiagLogMetadata __log_metadata = { \
        .source_info = { \
            .line_number = __LINE__, \
            .file_name = __FILE__, \
            .function_name = __func__, \
        }, \
        .severity = log_severity, \
        .verbosity = log_verbosity, \
        .text_log = msg, \
    }; \
    diagLogImpl(&__log_metadata); \
})

#define DIAG_DETAILED_LOGF(log_severity, log_verbosity, fmt, ...) ({ \
    char msg[0x400] = {}; \
    snprintf(msg, 0x400, fmt, ##__VA_ARGS__); \
    DIAG_DETAILED_LOG(log_severity, log_verbosity, msg); \
})

#define DIAG_DETAILED_VLOG(log_severity, log_verbosity, fmt, args) ({ \
    char msg[0x400] = {}; \
    vsnprintf(msg, 0x400, fmt, args); \
    DIAG_DETAILED_LOG(log_severity, log_verbosity, msg); \
})

#define DIAG_LOG(msg) DIAG_DETAILED_LOG(DiagLogSeverity_Info, false, msg)

#define DIAG_LOGF(fmt, ...) DIAG_DETAILED_LOGF(DiagLogSeverity_Info, false, fmt, ##__VA_ARGS__)

#define DIAG_VLOG(fmt, args) DIAG_DETAILED_VLOG(DiagLogSeverity_Info, false, fmt, args)
