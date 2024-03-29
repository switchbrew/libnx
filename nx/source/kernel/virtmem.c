#include "types.h"
#include "result.h"
#include "kernel/mutex.h"
#include "kernel/svc.h"
#include "kernel/virtmem.h"
#include "kernel/random.h"
#include "runtime/diag.h"
#include "../runtime/alloc.h"

#define SEQUENTIAL_GUARD_REGION_SIZE 0x1000
#define RANDOM_MAX_ATTEMPTS 0x200

typedef struct {
    uintptr_t start;
    uintptr_t end;
} MemRegion;

struct VirtmemReservation {
    VirtmemReservation *next;
    VirtmemReservation *prev;
    MemRegion region;
};

static Mutex g_VirtmemMutex;

static MemRegion g_AliasRegion;
static MemRegion g_HeapRegion;
static MemRegion g_AslrRegion;
static MemRegion g_StackRegion;

static VirtmemReservation *g_Reservations;

static bool g_IsLegacyKernel;

uintptr_t __attribute__((weak)) __libnx_virtmem_rng(void) {
    return (uintptr_t)randomGet64();
}

static Result _memregionInitWithInfo(MemRegion* r, InfoType id0_addr, InfoType id0_sz) {
    u64 base;
    Result rc = svcGetInfo(&base, id0_addr, CUR_PROCESS_HANDLE, 0);

    if (R_SUCCEEDED(rc)) {
        u64 size;
        rc = svcGetInfo(&size, id0_sz, CUR_PROCESS_HANDLE, 0);

        if (R_SUCCEEDED(rc)) {
            r->start = base;
            r->end   = base + size;
        }
    }

    return rc;
}

static void _memregionInitHardcoded(MemRegion* r, uintptr_t start, uintptr_t end) {
    r->start = start;
    r->end   = end;
}

NX_INLINE bool _memregionIsInside(MemRegion* r, uintptr_t start, uintptr_t end) {
    return start >= r->start && end <= r->end;
}

NX_INLINE bool _memregionOverlaps(MemRegion* r, uintptr_t start, uintptr_t end) {
    return start < r->end && r->start < end;
}

NX_INLINE bool _memregionIsMapped(uintptr_t start, uintptr_t end, uintptr_t guard, uintptr_t* out_end) {
    // Adjust start/end by the desired guard size.
    start -= guard;
    end += guard;

    // Query memory properties.
    MemoryInfo meminfo;
    u32 pageinfo;
    Result rc = svcQueryMemory(&meminfo, &pageinfo, start);
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadQueryMemory));

    // Return true if there's anything mapped.
    uintptr_t memend = meminfo.addr + meminfo.size;
    if (meminfo.type != MemType_Unmapped || end > memend) {
        if (out_end) *out_end = memend + guard;
        return true;
    }

    return false;
}

NX_INLINE bool _memregionIsReserved(uintptr_t start, uintptr_t end, uintptr_t guard, uintptr_t* out_end) {
    // Adjust start/end by the desired guard size.
    start -= guard;
    end += guard;

    // Go through each reservation and check if any of them overlap the desired address range.
    for (VirtmemReservation *rv = g_Reservations; rv; rv = rv->next) {
        if (_memregionOverlaps(&rv->region, start, end)) {
            if (out_end) *out_end = rv->region.end + guard;
            return true;
        }
    }

    return false;
}

static void* _memregionFindRandom(MemRegion* r, size_t size, size_t guard_size) {
    // Page align the sizes.
    size = (size + 0xFFF) &~ 0xFFF;
    guard_size = (guard_size + 0xFFF) &~ 0xFFF;

    // Ensure the requested size isn't greater than the memory region itself...
    uintptr_t region_size = r->end - r->start;
    if (size > region_size)
        return NULL;

    // Main allocation loop.
    uintptr_t aslr_max_page_offset = (region_size - size) >> 12;
    for (unsigned i = 0; i < RANDOM_MAX_ATTEMPTS; i ++) {
        // Calculate a random memory range outside reserved areas.
        uintptr_t cur_addr;
        for (;;) {
            uintptr_t page_offset = __libnx_virtmem_rng() % (aslr_max_page_offset + 1);
            cur_addr = (uintptr_t)r->start + (page_offset << 12);

            // Avoid mapping within the alias region.
            if (_memregionOverlaps(&g_AliasRegion, cur_addr, cur_addr + size))
                continue;

            // Avoid mapping within the heap region.
            if (_memregionOverlaps(&g_HeapRegion, cur_addr, cur_addr + size))
                continue;

            // Found it.
            break;
        }

        // Check that there isn't anything mapped at the desired memory range.
        if (_memregionIsMapped(cur_addr, cur_addr + size, guard_size, NULL))
            continue;

        // Check that the desired memory range doesn't overlap any reservations.
        if (_memregionIsReserved(cur_addr, cur_addr + size, guard_size, NULL))
            continue;

        // We found a suitable address!
        return (void*)cur_addr;
    }

    return NULL;
}

void virtmemSetup(void) {
    Result rc;

    // Retrieve memory region information for the reserved alias region.
    rc = _memregionInitWithInfo(&g_AliasRegion, InfoType_AliasRegionAddress, InfoType_AliasRegionSize);
    if (R_FAILED(rc)) {
        // Wat.
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_WeirdKernel));
    }

    // Account for the alias region extra size.
    u64 alias_extra_size;
    rc = svcGetInfo(&alias_extra_size, InfoType_AliasRegionExtraSize, CUR_PROCESS_HANDLE, 0);
    if (R_SUCCEEDED(rc)) {
        g_AliasRegion.end -= alias_extra_size;
    }

    // Retrieve memory region information for the reserved heap region.
    rc = _memregionInitWithInfo(&g_HeapRegion, InfoType_HeapRegionAddress, InfoType_HeapRegionSize);
    if (R_FAILED(rc)) {
        // Wat.
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadGetInfo_Heap));
    }

    // Retrieve memory region information for the aslr/stack regions if available [2.0.0+]
    rc = _memregionInitWithInfo(&g_AslrRegion, InfoType_AslrRegionAddress, InfoType_AslrRegionSize);
    if (R_SUCCEEDED(rc)) {
        rc = _memregionInitWithInfo(&g_StackRegion, InfoType_StackRegionAddress, InfoType_StackRegionSize);
        if (R_FAILED(rc))
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadGetInfo_Stack));
    }
    else {
        // [1.0.0] doesn't expose aslr/stack region information so we have to do this dirty hack to detect it.
        // Forgive me.
        g_IsLegacyKernel = true;
        rc = svcUnmapMemory((void*)0xFFFFFFFFFFFFE000UL, (void*)0xFFFFFE000UL, 0x1000);
        if (R_VALUE(rc) == KERNELRESULT(InvalidMemoryState)) {
            // Invalid src-address error means that a valid 36-bit address was rejected.
            // Thus we are 32-bit.
            _memregionInitHardcoded(&g_AslrRegion, 0x200000ull, 0x100000000ull);
            _memregionInitHardcoded(&g_StackRegion, 0x200000ull, 0x40000000ull);
        }
        else if (R_VALUE(rc) == KERNELRESULT(InvalidMemoryRange)) {
            // Invalid dst-address error means our 36-bit src-address was valid.
            // Thus we are 36-bit.
            _memregionInitHardcoded(&g_AslrRegion, 0x8000000ull, 0x1000000000ull);
            _memregionInitHardcoded(&g_StackRegion, 0x8000000ull, 0x80000000ull);
        }
        else {
            // Wat.
            diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_WeirdKernel));
        }
    }
}

void virtmemLock(void) {
    mutexLock(&g_VirtmemMutex);
}

void virtmemUnlock(void) {
    mutexUnlock(&g_VirtmemMutex);
}

void* virtmemFindAslr(size_t size, size_t guard_size) {
    if (!mutexIsLockedByCurrentThread(&g_VirtmemMutex)) return NULL;
    return _memregionFindRandom(&g_AslrRegion, size, guard_size);
}

void* virtmemFindStack(size_t size, size_t guard_size) {
    if (!mutexIsLockedByCurrentThread(&g_VirtmemMutex)) return NULL;
    return _memregionFindRandom(&g_StackRegion, size, guard_size);
}

void* virtmemFindCodeMemory(size_t size, size_t guard_size) {
    if (!mutexIsLockedByCurrentThread(&g_VirtmemMutex)) return NULL;
    // [1.0.0] requires CodeMemory to be mapped within the stack region.
    return _memregionFindRandom(g_IsLegacyKernel ? &g_StackRegion : &g_AslrRegion, size, guard_size);
}

VirtmemReservation* virtmemAddReservation(void* mem, size_t size) {
    if (!mutexIsLockedByCurrentThread(&g_VirtmemMutex)) return NULL;
    VirtmemReservation* rv = (VirtmemReservation*)__libnx_alloc(sizeof(VirtmemReservation));
    if (rv) {
        rv->region.start = (uintptr_t)mem;
        rv->region.end   = rv->region.start + size;
        rv->next         = g_Reservations;
        rv->prev         = NULL;
        g_Reservations   = rv;
        if (rv->next)
            rv->next->prev = rv;
    }
    return rv;
}

void virtmemRemoveReservation(VirtmemReservation* rv) {
    if (!mutexIsLockedByCurrentThread(&g_VirtmemMutex)) return;
    if (rv->next)
        rv->next->prev = rv->prev;
    if (rv->prev)
        rv->prev->next = rv->next;
    else
        g_Reservations = rv->next;
    __libnx_free(rv);
}
