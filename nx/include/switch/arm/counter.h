/**
 * @file counter.h
 * @brief AArch64 system counter-timer.
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/**
 * @brief Gets the current system tick.
 * @return The current system tick.
 */
static inline u64 armGetSystemTick(void) {
    u64 ret;
    __asm__ __volatile__ ("mrs %x[data], cntpct_el0" : [data] "=r" (ret));
    return ret;
}

/**
 * @brief Gets the system counter-timer frequency
 * @return The system counter-timer frequency, in Hz.
 */
static inline u64 armGetSystemTickFreq(void) {
    u64 ret;
    __asm__ ("mrs %x[data], cntfrq_el0" : [data] "=r" (ret));
    return ret;
}
