// Copyright 2018 plutoo

typedef struct {
    RMutex r;
    RMutex g;
    u64 b;
} RwLock;

void rwlockReadLock(RwLock* r);
void rwlockReadUnlock(RwLock* r);

void rwlockWriteLock(RwLock* r);
void rwlockWriteUnlock(RwLock* r);
