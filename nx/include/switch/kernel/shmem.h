typedef struct {
    Handle      MemHandle;
    size_t      Size;
    Permission  Perm;
    void*       MappedAddr;
} SharedMemory;

Result shmemCreate(SharedMemory* s, size_t size, Permission local_perm, Permission remote_perm);
void shmemLoadRemote(SharedMemory* t, Handle handle, size_t size, Permission perm);

Result shmemMap(SharedMemory* t);
Result shmemUnmap(SharedMemory* t);
void*  shmemGetAddr(SharedMemory* t);

Result shmemClose(SharedMemory* t);
