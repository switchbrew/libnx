/**
 * @file jit.h
 * @brief Just-in-time compilation support.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "virtmem.h"

/// JIT implementation type.
typedef enum {
    JitType_SetProcessMemoryPermission, ///< JIT supported using svcSetProcessMemoryPermission
    JitType_CodeMemory,                 ///< JIT supported using [4.0.0+] CodeMemory syscalls
} JitType;

/// JIT buffer object.
typedef struct {
    JitType type;
    size_t  size;
    void*   src_addr;
    void*   rx_addr;
    void*   rw_addr;
    bool    is_executable;
    union {
        Handle              handle;
        VirtmemReservation* rv;
    };
} Jit;

/**
 * @brief Creates a JIT buffer.
 * @param j JIT buffer.
 * @param size Size of the JIT buffer.
 * @return Result code.
 */
Result jitCreate(Jit* j, size_t size);

/**
 * @brief Transition a JIT buffer to have writable permission.
 * @param j JIT buffer.
 * @return Result code.
 */
Result jitTransitionToWritable(Jit* j);

/**
 * @brief Transition a JIT buffer to have executable permission.
 * @param j JIT buffer.
 * @return Result code.
 */
Result jitTransitionToExecutable(Jit* j);

/**
 * @brief Destroys a JIT buffer.
 * @param j JIT buffer.
 * @return Result code.
 */
Result jitClose(Jit* j);

/**
 * @brief Gets the address of the writable memory alias of a JIT buffer.
 * @param j JIT buffer.
 * @return Pointer to alias of the JIT buffer that can be written to.
 */
NX_CONSTEXPR void* jitGetRwAddr(Jit* j) {
    return j->rw_addr;
}

/**
 * @brief Gets the address of the executable memory alias of a JIT buffer.
 * @param j JIT buffer.
 * @return Pointer to alias of the JIT buffer that can be executed.
 */
NX_CONSTEXPR void* jitGetRxAddr(Jit* j) {
    return j->rx_addr;
}
