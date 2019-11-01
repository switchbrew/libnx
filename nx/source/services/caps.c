#include "types.h"
#include "result.h"
#include "services/caps.h"
#include "runtime/hosversion.h"

u64 capsGetShimLibraryVersion(void) {
    u64 version=1; // [7.0.0-8.1.0]

    return version;
}

