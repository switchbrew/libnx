#pragma once
#include "../switch/types.h"
#include "../switch/result.h"
#include "../switch/kernel/mutex.h"
#include "../switch/kernel/condvar.h"
#include "../switch/kernel/thread.h"

#define TSS_DTOR_ITERATIONS 1

typedef struct {
    Thread thr;
    int rc;
} __thrd_t;

typedef CondVar   cnd_t;
typedef __thrd_t* thrd_t;
typedef u32       tss_t;

#define _MTX_INITIALIZER_NP {mtx_plain, {0}}
typedef struct mtx_t {
    int type;
    union {
        Mutex mutex;
        RMutex rmutex;
    };
} mtx_t;

#define ONCE_FLAG_INIT {0,_MTX_INITIALIZER_NP,0}
typedef struct once_flag_t {
    int status;
    mtx_t mutex;
    cnd_t cond;
} once_flag;
