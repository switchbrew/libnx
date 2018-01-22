#pragma once
#include <switch/types.h>

typedef struct {
    Handle     handle;
    void*      stack_mem;
    void*      stack_mirror;
    size_t     stack_sz;
} Thread;

Result threadCreate(
    Thread* t, ThreadFunc entry, void* arg, size_t stack_sz, int prio,
    int cpuid);

Result threadStart(Thread* t);
Result threadWaitForExit(Thread* t);
Result threadClose(Thread* t);

Result threadPause(Thread* t);
Result threadResume(Thread* t);
