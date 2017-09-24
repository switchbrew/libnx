// Copyright 2017 plutoo
typedef struct {
    u32 Tag;
} Mutex;

typedef struct {
    u32    Owner;
    Mutex  Lock;
    size_t Count;
} RMutex;

void mutexLock(Mutex* m);
void mutexUnlock(Mutex* m);

void rmutexLock(RMutex* m);
void rmutexUnlock(RMutex* m);
