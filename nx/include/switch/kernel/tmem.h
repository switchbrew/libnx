/**
 * @file tmem.h
 * @brief Transfer memory handling
 * @author plutoo
 * @copyright libnx Authors
 * @remark Transfer memory differs from shared memory in the fact that the user process (as opposed to the kernel) allocates and owns its backing memory.
 */
#pragma once
#include "../types.h"
#include "../kernel/svc.h"

/// Transfer memory information structure.
typedef struct {
    Handle      handle;   ///< Kernel object handle.
    size_t      size;     ///< Size of the transfer memory object.
    Permission  perm;     ///< Permissions of the transfer memory object.
    void*       src_addr; ///< Address of the source backing memory.
    void*       map_addr; ///< Address to which the transfer memory object is mapped.
} TransferMemory;

/**
 * @brief Creates a transfer memory object.
 * @param t Transfer memory information structure that will be filled in.
 * @param size Size of the transfer memory object to create.
 * @param perm Permissions with which to protect the transfer memory in the local process.
 * @return Result code.
 */
Result tmemCreate(TransferMemory* t, size_t size, Permission perm);

/**
 * @brief Creates a transfer memory object from existing memory.
 * @param t Transfer memory information structure that will be filled in.
 * @param buf Pointer to a page-aligned buffer.
 * @param size Size of the transfer memory object to create.
 * @param perm Permissions with which to protect the transfer memory in the local process.
 * @return Result code.
 */
Result tmemCreateFromMemory(TransferMemory* t, void* buf, size_t size, Permission perm);

/**
 * @brief Loads a transfer memory object coming from a remote process.
 * @param t Transfer memory information structure which will be filled in.
 * @param handle Handle of the transfer memory object.
 * @param size Size of the transfer memory object that is being loaded.
 * @param perm Permissions which the transfer memory is expected to have in the process that owns the memory.
 * @warning This is a privileged operation; in normal circumstances applications shouldn't use this function.
 */
void tmemLoadRemote(TransferMemory* t, Handle handle, size_t size, Permission perm);

/**
 * @brief Maps a transfer memory object.
 * @param t Transfer memory information structure.
 * @return Result code.
 * @warning This is a privileged operation; in normal circumstances applications cannot use this function.
 */
Result tmemMap(TransferMemory* t);

/**
 * @brief Unmaps a transfer memory object.
 * @param t Transfer memory information structure.
 * @return Result code.
 * @warning This is a privileged operation; in normal circumstances applications cannot use this function.
 */
Result tmemUnmap(TransferMemory* t);

/**
 * @brief Retrieves the mapped address of a transfer memory object.
 * @param t Transfer memory information structure.
 * @return Mapped address of the transfer memory object.
 */
static inline void* tmemGetAddr(TransferMemory* t){
    return t->map_addr;
}

/**
 * @brief Frees up resources used by a transfer memory object, unmapping and closing handles, etc.
 * @param t Transfer memory information structure.
 * @return Result code.
 */
Result tmemClose(TransferMemory* t);
