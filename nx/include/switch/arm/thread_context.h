/**
 * @file thread_context.h
 * @brief AArch64 register dump format and related definitions.
 * @author TuxSH
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"

/// Armv8 CPU register.
typedef union {
    u64 x; ///< 64-bit AArch64 register view.
    u32 w; ///< 32-bit AArch64 register view.
    u32 r; ///< AArch32 register view.
} CpuRegister;

/// Armv8 NEON register.
typedef union {
    u128    v; ///< 128-bit vector view.
    double  d; ///< 64-bit double-precision view.
    float   s; ///< 32-bit single-precision view.
} FpuRegister;

/// Armv8 register group. @ref svcGetThreadContext3 uses @ref RegisterGroup_All.
typedef enum {
    RegisterGroup_CpuGprs = BIT(0), ///< General-purpose CPU registers (x0..x28 or r0..r10,r12).
    RegisterGroup_CpuSprs = BIT(1), ///< Special-purpose CPU registers (fp, lr, sp, pc, PSTATE or cpsr, TPIDR_EL0).
    RegisterGroup_FpuGprs = BIT(2), ///< General-purpose NEON registers.
    RegisterGroup_FpuSprs = BIT(3), ///< Special-purpose NEON registers.

    RegisterGroup_CpuAll  = RegisterGroup_CpuGprs | RegisterGroup_CpuSprs, ///< All CPU registers.
    RegisterGroup_FpuAll  = RegisterGroup_FpuGprs | RegisterGroup_FpuSprs, ///< All NEON registers.
    RegisterGroup_All     = RegisterGroup_CpuAll  | RegisterGroup_FpuAll,  ///< All registers.
} RegisterGroup;

/// Thread context structure (register dump)
typedef struct {
    CpuRegister cpu_gprs[29];   ///< GPRs 0..28. Note: also contains AArch32 SPRs.
    u64 fp;                     ///< Frame pointer (x29) (AArch64). For AArch32, check r11.
    u64 lr;                     ///< Link register (x30) (AArch64). For AArch32, check r14.
    u64 sp;                     ///< Stack pointer (AArch64). For AArch32, check r13.
    CpuRegister pc;             ///< Program counter.
    u32         psr;            ///< PSTATE or cpsr.

    FpuRegister fpu_gprs[32];   ///< 32 general-purpose NEON registers.
    u32         fpcr;           ///< Floating-point control register.
    u32         fpsr;           ///< Floating-point status register.

    u64         tpidr;          ///< EL0 Read/Write Software Thread ID Register.
} ThreadContext;

/**
 * @brief Determines whether a thread context belong to an AArch64 process based on the PSR.
 * @param[in] ctx Thread context to which PSTATE/cspr has been dumped to.
 * @return true if and only if the thread context belongs to an AArch64 process.
 */
static inline bool threadContextIsAArch64(const ThreadContext *ctx)
{
    return (ctx->psr & 0x10) == 0;
}
