/**
 * @file shmem.h
 * @brief Shared memory object handling
 * @author plutoo
 * @copyright libnx Authors
 * @remark Shared memory differs from transfer memory in the fact that the kernel (as opposed to the user process) allocates and owns its backing memory.
 */
#pragma once
#include "../types.h"

/// Shared memory information structure.
typedef struct {
    Handle      handle;   ///< Kernel object handle.
    size_t      size;     ///< Size of the shared memory object.
    Permission  perm;     ///< Permissions.
    void*       map_addr; ///< Address to which the shared memory object is mapped.
} SharedMemory;

/**
 * @brief Creates a shared memory object.
 * @param s Shared memory information structure which will be filled in.
 * @param size Size of the shared memory object to create.
 * @param local_perm Permissions with which the shared memory object will be mapped in the local process.
 * @param remote_perm Permissions with which the shared memory object will be mapped in the remote process (can be Perm_DontCare).
 * @return Result code.
 * @warning This is a privileged operation; in normal circumstances applications cannot use this function.
 */
Result shmemCreate(SharedMemory* s, size_t size, Permission local_perm, Permission remote_perm);

/**
 * @brief Loads a shared memory object coming from a remote process.
 * @param s Shared memory information structure which will be filled in.
 * @param handle Handle of the shared memory object.
 * @param size Size of the shared memory object that is being loaded.
 * @param perm Permissions with which the shared memory object will be mapped in the local process.
 */
void shmemLoadRemote(SharedMemory* s, Handle handle, size_t size, Permission perm);

/**
 * @brief Maps a shared memory object.
 * @param s Shared memory information structure.
 * @return Result code.
 */
Result shmemMap(SharedMemory* s);

/**
 * @brief Unmaps a shared memory object.
 * @param s Shared memory information structure.
 * @return Result code.
 */
Result shmemUnmap(SharedMemory* s);

/**
 * @brief Retrieves the mapped address of a shared memory object.
 * @param s Shared memory information structure.
 * @return Mapped address of the shared memory object.
 */
static inline void* shmemGetAddr(SharedMemory* s) {
    return s->map_addr;
}

/**
 * @brief Frees up resources used by a shared memory object, unmapping and closing handles, etc.
 * @param s Shared memory information structure.
 * @return Result code.
 */
Result shmemClose(SharedMemory* s);
