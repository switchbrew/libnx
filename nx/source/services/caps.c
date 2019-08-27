#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "kernel/event.h"
#include "kernel/tmem.h"
#include "services/sm.h"
#include "services/caps.h"
#include "runtime/hosversion.h"

u64 capsGetShimLibraryVersion(void) {
    u64 version=1; // [7.0.0-8.1.0]

    return version;
}

