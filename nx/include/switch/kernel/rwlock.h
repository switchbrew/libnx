// Copyright 2018 plutoo
#pragma once
#include <switch/kernel/mutex.h>

typedef struct {
    RMutex r;
    RMutex g;
    u64 b;
} RwLock;

void rwlockReadLock(RwLock* r);
void rwlockReadUnlock(RwLock* r);

void rwlockWriteLock(RwLock* r);
void rwlockWriteUnlock(RwLock* r);
