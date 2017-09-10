// Copyright 2017 plutoo
// Worst heap ever xD
#include <switch.h>

typedef struct HeapHeader HeapHeader;

typedef enum {
    USED = 0x55534544,
    FREE = 0x46524545
} HeapState;

struct HeapHeader {
    size_t State;
    size_t Size;
    HeapHeader* Next;
    HeapHeader* Prev;
};

static HeapHeader g_LastFree;

void heapInit(void* base, size_t size) {
    // Called by crt0.
    HeapHeader* hdr = (HeapHeader*) base;

    hdr->Next = &g_LastFree;
    hdr->Prev = &g_LastFree;
    hdr->Size = size - sizeof(HeapHeader);
    hdr->State = FREE;

    g_LastFree.Next = hdr;
    g_LastFree.Prev = hdr;
}

void heapSetup() {
    static u8 g_Heap[0x20000];
    heapInit(g_Heap, sizeof(g_Heap));
}

void* heapAllocPages(size_t size) {
    void* ptr = heapAlloc(size + 0x1000);

    if (ptr != NULL) {
        ptr = (void*) ((((uintptr_t) ptr) + 0xFFF) &~ 0xFFF);
    }

    return ptr;
}

void* heapAlloc(size_t size) {
    size = (size + 15) &~ 15;

    HeapHeader* hdr = &g_LastFree;

    while ((hdr = hdr->Next) != &g_LastFree) {
        if (hdr->Size >= size) {
            size_t rem = hdr->Size - size;

            if (rem < sizeof(HeapHeader)) {
                size = hdr->Size;
                rem = 0;
            }

            hdr->State = USED;
            hdr->Size = size;

            hdr->Prev->Next = hdr->Next;
            hdr->Next->Prev = hdr->Prev;

            if (rem != 0) {
                HeapHeader* rem_hdr = (HeapHeader*) (((uintptr_t)(hdr + 1)) + size);
                rem_hdr->State = FREE;
                rem_hdr->Size = rem - sizeof(HeapHeader);

                rem_hdr->Next = g_LastFree.Next;
                rem_hdr->Prev = &g_LastFree;
                g_LastFree.Next = rem_hdr;
            }

            return (void*) (((uintptr_t) hdr) + sizeof(HeapHeader));
        }
    }

    return NULL;
}

void heapFree(void* ptr) {
    HeapHeader* hdr = (HeapHeader*) (((uintptr_t) ptr) - sizeof(HeapHeader));
    hdr->State = FREE;

    hdr->Next = g_LastFree.Next;
    g_LastFree.Next = hdr;
    hdr->Prev = &g_LastFree;
}

