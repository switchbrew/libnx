/**
 * @file fatal.h
 * @brief Fatal error (fatal:u) service IPC wrapper.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Type of thrown fatal error.
typedef enum {
    FatalPolicy_ErrorReportAndErrorScreen = 0,
    FatalPolicy_ErrorReport = 1,
    FatalPolicy_ErrorScreen = 2 ///< Only available with [3.0.0+]. If specified, FatalPolicy_ErrorReportAndErrorScreen will be used instead on pre-3.0.0.
} FatalPolicy;

/// Struct for fatal Cpu context, 64-bit.
typedef struct {
    union {
        u64 x[32];
        struct {
            u64 _x[29];
            u64 fp;
            u64 lr;
            u64 sp;
            u64 pc;
        };
    };
    u64 pstate;
    u64 afsr0;
    u64 afsr1;
    u64 esr;
    u64 far;

    u64 stack_trace[32];
    u64 start_address;      ///< Address of first NSO loaded (generally, process entrypoint).
    u64 register_set_flags; ///< Bitmask, bit i indicates GPR i has a value.
    u32 stack_trace_size;
} FatalAarch64Context;

/// Struct for fatal Cpu context, 32-bit.
typedef struct {
    union {
        u32 r[16];
        struct {
            u32 _r[11];
            u32 fp;
            u32 ip;
            u32 sp;
            u32 lr;
            u32 pc;
        };
    };
    u32 pstate;
    u32 afsr0;
    u32 afsr1;
    u32 esr;
    u32 far;

    u32 stack_trace[32];
    u32 stack_trace_size;
    u32 start_address;      ///< Address of first NSO loaded (generally, process entrypoint).
    u32 register_set_flags; ///< Bitmask, bit i indicates GPR i has a value.
} FatalAarch32Context;

typedef struct {
    union {
        FatalAarch64Context aarch64_ctx;
        FatalAarch32Context aarch32_ctx;
    };

    bool is_aarch32;
    u32 type;
} FatalCpuContext;

/**
 * @brief Triggers a system fatal error.
 * @param[in] err Result code to throw.
 * @note This function does not return.
 * @note This uses \ref fatalThrowWithPolicy with \ref FatalPolicy_ErrorScreen internally.
 */
void NX_NORETURN fatalThrow(Result err);

/**
 * @brief Triggers a system fatal error with a custom \ref FatalPolicy.
 * @param[in] err Result code to throw.
 * @param[in] type Type of fatal error to throw.
 * @note This function may not return, depending on \ref FatalPolicy.
 */
void fatalThrowWithPolicy(Result err, FatalPolicy type);

/**
 * @brief Triggers a system fatal error with a custom \ref FatalPolicy and \ref FatalCpuContext.
 * @param[in] err  Result code to throw.
 * @param[in] type Type of fatal error to throw.
 * @param[in] ctx  Cpu context for fatal error to throw.
 * @note This function may not return, depending on \ref FatalPolicy.
 */
void fatalThrowWithContext(Result err, FatalPolicy type, FatalCpuContext *ctx);
