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

/// This is for \ref ThreadExceptionDump error_desc.
typedef enum {
    ThreadExceptionDesc_InstructionAbort = 0x100, ///< Instruction abort
    ThreadExceptionDesc_MisalignedPC     = 0x102, ///< Misaligned PC
    ThreadExceptionDesc_MisalignedSP     = 0x103, ///< Misaligned SP
    ThreadExceptionDesc_SError           = 0x106, ///< SError [not in 1.0.0?]
    ThreadExceptionDesc_BadSVC           = 0x301, ///< Bad SVC
    ThreadExceptionDesc_Trap             = 0x104, ///< Uncategorized, CP15RTTrap, CP15RRTTrap, CP14RTTrap, CP14RRTTrap, IllegalState, SystemRegisterTrap
    ThreadExceptionDesc_Other            = 0x101, ///< None of the above, EC <= 0x34 and not a breakpoint
} ThreadExceptionDesc;

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

/// Thread exception dump structure.
typedef struct {
    u32 error_desc;             ///< See \ref ThreadExceptionDesc.
    u32 pad[3];

    CpuRegister cpu_gprs[29];   ///< GPRs 0..28. Note: also contains AArch32 registers.
    CpuRegister fp;             ///< Frame pointer.
    CpuRegister lr;             ///< Link register.
    CpuRegister sp;             ///< Stack pointer.
    CpuRegister pc;             ///< Program counter (elr_el1).

    u64 padding;

    FpuRegister fpu_gprs[32];   ///< 32 general-purpose NEON registers.

    u32 pstate;                 ///< pstate & 0xFF0FFE20
    u32 afsr0;
    u32 afsr1;
    u32 esr;

    CpuRegister far;            ///< Fault Address Register.
} ThreadExceptionDump;

typedef struct {
    u64 cpu_gprs[9]; ///< GPRs 0..8.
    u64 lr;
    u64 sp;
    u64 elr_el1;
    u32 pstate;       ///< pstate & 0xFF0FFE20
    u32 afsr0;
    u32 afsr1;
    u32 esr;
    u64 far;
} ThreadExceptionFrameA64;

typedef struct {
    u32 cpu_gprs[8]; ///< GPRs 0..7.
    u32 sp;
    u32 lr;
    u32 elr_el1;
    u32 tpidr_el0;    ///< tpidr_el0 = 1
    u32 cpsr;         ///< cpsr & 0xFF0FFE20
    u32 afsr0;
    u32 afsr1;
    u32 esr;
    u32 far;
} ThreadExceptionFrameA32;

/**
 * @brief Determines whether a thread context belong to an AArch64 process based on the PSR.
 * @param[in] ctx Thread context to which PSTATE/cspr has been dumped to.
 * @return true if and only if the thread context belongs to an AArch64 process.
 */
static inline bool threadContextIsAArch64(const ThreadContext *ctx)
{
    return (ctx->psr & 0x10) == 0;
}

/**
 * @brief Determines whether a ThreadExceptionDump belongs to an AArch64 process based on the PSTATE.
 * @param[in] ctx ThreadExceptionDump.
 * @return true if and only if the ThreadExceptionDump belongs to an AArch64 process.
 */
static inline bool threadExceptionIsAArch64(const ThreadExceptionDump *ctx)
{
    return (ctx->pstate & 0x10) == 0;
}
