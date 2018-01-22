#pragma once
#include <switch/types.h>

typedef struct {
    Handle      handle;
    size_t      size;
    Permission  perm;
    void*       map_addr;
} SharedMemory;

Result shmemCreate(SharedMemory* s, size_t size, Permission local_perm, Permission remote_perm);
void shmemLoadRemote(SharedMemory* t, Handle handle, size_t size, Permission perm);

Result shmemMap(SharedMemory* t);
Result shmemUnmap(SharedMemory* t);
void*  shmemGetAddr(SharedMemory* t);

Result shmemClose(SharedMemory* t);
